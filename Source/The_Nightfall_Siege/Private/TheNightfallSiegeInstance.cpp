// Fill out your copyright notice in the Description page of Project Settings.


#include "TheNightfallSiegeInstance.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "IPJoinWidget.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UTheNightfallSiegeInstance::UTheNightfallSiegeInstance()
{
    static ConstructorHelpers::FClassFinder<UIPJoinWidget> IPWidgetBlueprint(TEXT("/Game/BP/WBP_IP"));
    if (IPWidgetBlueprint.Succeeded())
    {
        IPJoinWidgetClass = IPWidgetBlueprint.Class;
    }
}

void UTheNightfallSiegeInstance::Init()
{
    Super::Init();

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTheNightfallSiegeInstance::HandlePostLoadMap);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().AddUObject(this, &UTheNightfallSiegeInstance::HandleNetworkFailure);
    }

    SkillLevels.Add(ESkillType::Q, 1);
    SkillLevels.Add(ESkillType::W, 1);
    SkillLevels.Add(ESkillType::E, 1);
    SkillLevels.Add(ESkillType::R, 1);

    StartRaid();
}

void UTheNightfallSiegeInstance::Shutdown()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
    }

    Super::Shutdown();
}

void UTheNightfallSiegeInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!LoadedWorld || !LoadedWorld->GetMapName().Contains(TEXT("Lvl_MainMenu")))
    {
        return;
    }

    // The game mode creates WBP_Title after the map callback, so bind on the
    // next tick after its Blueprint Construct event has installed its handlers.
    LoadedWorld->GetTimerManager().SetTimerForNextTick(this, &UTheNightfallSiegeInstance::BindMainMenuJoinButton);
}

void UTheNightfallSiegeInstance::BindMainMenuJoinButton()
{
    TArray<UUserWidget*> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), false);

    for (UUserWidget* Widget : Widgets)
    {
        if (!Widget || !Widget->GetClass()->GetName().Contains(TEXT("WBP_Title")))
        {
            continue;
        }

        if (UButton* JoinButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Btn_Join"))))
        {
            // WBP_Title previously bound this button to "open 127.0.0.1".
            // Replacing that binding prevents a second, hard-coded travel.
            JoinButton->OnClicked.Clear();
            JoinButton->OnClicked.AddDynamic(this, &UTheNightfallSiegeInstance::OnMainMenuJoinClicked);
        }

        if (UButton* HostButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Btn_Host"))))
        {
            // Opening a listen server now requires the same nickname step as
            // joining one, so the host never appears under the PC name.
            HostButton->OnClicked.Clear();
            HostButton->OnClicked.AddDynamic(this, &UTheNightfallSiegeInstance::OnMainMenuHostClicked);
        }

        return;
    }
}

void UTheNightfallSiegeInstance::OnMainMenuJoinClicked()
{
    ShowIPJoinDialog();
}

void UTheNightfallSiegeInstance::OnMainMenuHostClicked()
{
    ShowHostDialog();
}

void UTheNightfallSiegeInstance::ShowIPJoinDialog()
{
    ShowConnectionDialog(false);
}

void UTheNightfallSiegeInstance::ShowHostDialog()
{
    ShowConnectionDialog(true);
}

void UTheNightfallSiegeInstance::ShowConnectionDialog(bool bHostMode)
{
    if (IPJoinWidget && IPJoinWidget->IsInViewport())
    {
        return;
    }

    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        TSubclassOf<UIPJoinWidget> WidgetClass = IPJoinWidgetClass;
        if (!WidgetClass)
        {
            WidgetClass = UIPJoinWidget::StaticClass();
        }
        IPJoinWidget = CreateWidget<UIPJoinWidget>(PlayerController, WidgetClass);
        if (IPJoinWidget)
        {
            IPJoinWidget->AddToViewport(1000);
            IPJoinWidget->SetHostMode(bHostMode);
            PlayerController->bShowMouseCursor = true;
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(IPJoinWidget->TakeWidget());
            PlayerController->SetInputMode(InputMode);
        }
    }
}

void UTheNightfallSiegeInstance::HandleNetworkFailure(UWorld*, UNetDriver*, ENetworkFailure::Type, const FString&)
{
    if (IPJoinWidget && IPJoinWidget->IsConnecting())
    {
        IPJoinWidget->ShowConnectionError();
    }
}

void UTheNightfallSiegeInstance::StartRaid()
{
    PendingTravelHealthByPlayerId.Empty();
    RemainingDungeons.Empty();

    // Each raid gets a random dungeon order.  A retry keeps CurrentDungeon,
    // so only the failed entry in that order is attempted again.
    RemainingDungeons = { "LV_Dungeon1", "LV_Dungeon2", "LV_Dungeon3" };

    DungeonPortalLocations =
    {
        FVector(27900.f, 28190.f, 870.f),
        FVector(-440.f, 13730.f, 0.f),
        FVector(26900.f, 14330.f, 0.f),
        FVector(31450.f, 1070.f, 2290.f),
        FVector(19370.f, 6540.f, 1980.f),
        FVector(6360.f, 25440.f, 0.f)
    };

    ClearedDungeonCount = 0;
    CurrentDungeon = NAME_None;
    bBossPortalSpawned = false;
    bRetryingCurrentDungeon = false;
    bRetryingBoss = false;
}

void UTheNightfallSiegeInstance::SaveTravelHealth(int32 PlayerId, float CurrentHP)
{
    PendingTravelHealthByPlayerId.FindOrAdd(PlayerId) = FMath::Max(0.f, CurrentHP);
}

bool UTheNightfallSiegeInstance::ConsumeTravelHealth(int32 PlayerId, float& OutCurrentHP)
{
    if (const float* SavedHealth = PendingTravelHealthByPlayerId.Find(PlayerId))
    {
        OutCurrentHP = *SavedHealth;
        PendingTravelHealthByPlayerId.Remove(PlayerId);
        return true;
    }

    return false;
}

void UTheNightfallSiegeInstance::BeginRetry(bool bWasBossEncounter)
{
    bRetryingBoss = bWasBossEncounter;
    bRetryingCurrentDungeon = !bWasBossEncounter && !CurrentDungeon.IsNone();
}

bool UTheNightfallSiegeInstance::ConsumeDungeonRetry()
{
    const bool bRetry = bRetryingCurrentDungeon;
    bRetryingCurrentDungeon = false;
    return bRetry;
}

FName UTheNightfallSiegeInstance::SelectNextDungeon()
{
    if (RemainingDungeons.Num() <= 0)
    {
        return NAME_None;
    }

    const int32 RandomIndex = FMath::RandRange(0, RemainingDungeons.Num() - 1);
    CurrentDungeon = RemainingDungeons[RandomIndex];

    return CurrentDungeon;
}

bool UTheNightfallSiegeInstance::SelectNextDungeonPortalLocation(FVector& OutLocation) const
{
    if (DungeonPortalLocations.IsEmpty())
    {
        return false;
    }

    const int32 RandomIndex = FMath::RandRange(0, DungeonPortalLocations.Num() - 1);
    OutLocation = DungeonPortalLocations[RandomIndex];
    return true;
}

bool UTheNightfallSiegeInstance::SelectBossPortalLocation(FVector& OutLocation) const
{
    if (DungeonPortalLocations.IsEmpty())
    {
        return false;
    }

    const int32 RandomIndex = FMath::RandRange(0, DungeonPortalLocations.Num() - 1);
    OutLocation = DungeonPortalLocations[RandomIndex];
    return true;
}

bool UTheNightfallSiegeInstance::ClearCurrentDungeon()
{
    UE_LOG(LogTemp, Warning, TEXT("CurrentDungeon : %s"), *CurrentDungeon.ToString());

    if (CurrentDungeon.IsNone() || !RemainingDungeons.RemoveSingle(CurrentDungeon))
    {
        return ClearedDungeonCount >= 3;
    }

    UE_LOG(LogTemp, Warning, TEXT("Remaining : %d"), RemainingDungeons.Num());

    ClearedDungeonCount++;

    return ClearedDungeonCount >= 3;
}
