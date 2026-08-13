// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseController.h"
#include "BaseCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "BasePlayerState.h"
#include "LobbyWidget.h"
#include "TimerManager.h"
#include "The_Nightfall_SiegeGameMode.h"
#include "TheNightfallSiegeInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "DeathScreenWidget.h"
#include "The_Nightfall_SiegeGameMode.h"
#include "Kismet/KismetSystemLibrary.h"

void ABaseController::SetupInputComponent()
{
    Super::SetupInputComponent();

	InputComponent->BindAction("RightClick", IE_Pressed, this, &ABaseController::MoveToMouse);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ABaseController::HandleOverlayClick);
}

void ABaseController::OnRightClick()
{
    ABaseCharacter* MyCharacter =
        Cast<ABaseCharacter>(GetPawn());

    if (!MyCharacter || MyCharacter->bIsDead)
    {
        return;
    }

    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        RotateCharacterToCursor();

        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
    }
}

void ABaseController::MoveToMouse()
{
    ABaseCharacter* MyCharacter =
        Cast<ABaseCharacter>(GetPawn());

    if (!MyCharacter)
    {
        return;
    }

    if (MyCharacter->bIsDead)
    {
        return;
    }

    if (MyCharacter->bIsUsingSkill)
    {
        return;
    }

    if (MyCharacter->bIsAttacking)
    {
        return;
    }

    FHitResult Hit;

    GetHitResultUnderCursor(
        ECC_Visibility,
        false,
        Hit);

    if (Hit.bBlockingHit)
    {
        FVector Direction =
            Hit.Location - MyCharacter->GetActorLocation();

        Direction.Z = 0.f;

        FRotator TargetRotation =
            Direction.Rotation();

        MyCharacter->SetActorRotation(TargetRotation);

        if (ClickFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                ClickFX,
                Hit.Location,
                FRotator::ZeroRotator);
        }

        ServerMoveToLocation(
            Hit.Location,
            TargetRotation);
    }
}

void ABaseController::RotateCharacterToCursor()
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        return;

    APawn* BasePawn = GetPawn();
    if (!BasePawn) return;

    if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(BasePawn))
    {
        if (BaseCharacter->bIsDead)
        {
            return;
        }
    }

    FVector Direction = Hit.Location - BasePawn->GetActorLocation();
    Direction.Z = 0.f;

    FRotator TargetRotation = Direction.Rotation();

    BasePawn->SetActorRotation(TargetRotation);
}

void ABaseController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    FString MapName = GetWorld()->GetMapName();

    UE_LOG(LogTemp, Warning, TEXT("%s"), *MapName);

    if (MapName.Contains(TEXT("Lvl_Lobby")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Lobby Controller"));

        if (LobbyWidgetClass)
        {
            LobbyWidget =
                CreateWidget<UUserWidget>(
                    this,
                    LobbyWidgetClass);

            if (LobbyWidget)
            {
                LobbyWidget->AddToViewport();

                ULobbyWidget* Lobby =
                    Cast<ULobbyWidget>(LobbyWidget);

                if (Lobby)
                {
                    GetWorld()->GetTimerManager().SetTimer(
                        LobbyRefreshHandle,
                        Lobby,
                        &ULobbyWidget::RefreshLobby,
                        1.0f,
                        true);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("%s"), *MapName);

    bShowMouseCursor = true;

    DefaultMouseCursor = EMouseCursor::Default;

    SetIgnoreLookInput(true);

    TreeComponents.Empty();

    TArray<UHierarchicalInstancedStaticMeshComponent*> Components;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        Components.Reset();

        It->GetComponents<UHierarchicalInstancedStaticMeshComponent>(Components);

        for (UHierarchicalInstancedStaticMeshComponent* Comp : Components)
        {
            if (!Comp || !Comp->GetStaticMesh())
                continue;

            const FString MeshName = Comp->GetStaticMesh()->GetName();

            if (MeshName == TEXT("tree_test") ||
                MeshName == TEXT("realistic_tree"))
            {
                TreeComponents.Add(Comp);
            }
        }
    }
}

void ABaseController::SelectNextCharacter()
{
    UE_LOG(LogTemp, Error, TEXT("===== SelectNextCharacter Called ====="));

    ABasePlayerState* PS = Cast<ABasePlayerState>(PlayerState);

    if (!PS)
    {
        return;
    }

    ECharacterType NewCharacter;

    switch (PS->SelectedCharacter)
    {
    case ECharacterType::Paladin:
        NewCharacter = ECharacterType::Warrior;
        break;

    case ECharacterType::Warrior:
        NewCharacter = ECharacterType::Archer;
        break;

    default:
        NewCharacter = ECharacterType::Paladin;
        break;
    }

    ServerSelectCharacter(NewCharacter);
}

void ABaseController::ToggleReady()
{
    ABasePlayerState* PS = Cast<ABasePlayerState>(PlayerState);

    if (!PS)
    {
        return;
    }

    ServerSetReady(!PS->IsReady());
}

void ABaseController::ServerSetReady_Implementation(bool bNewReady)
{
    ABasePlayerState* PS = Cast<ABasePlayerState>(PlayerState);

    if (!PS)
    {
        return;
    }

    PS->SetReady(bNewReady);

    UE_LOG(LogTemp, Warning,
        TEXT("Ready : %s"),
        bNewReady ? TEXT("True") : TEXT("False"));
}

void ABaseController::StartGame()
{
    ServerStartGame();
}

void ABaseController::ServerStartGame_Implementation()
{
    UE_LOG(LogTemp, Error, TEXT("ServerTravel Start"));

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ABaseController* PC = Cast<ABaseController>(*It);

        if (!PC)
            continue;

        ABasePlayerState* PS = PC->GetPlayerState<ABasePlayerState>();

        if (!PS)
            continue;

        UE_LOG(LogTemp, Error,
            TEXT("Travel Character = %d"),
            (int32)PS->SelectedCharacter);
    }

    GetWorld()->ServerTravel(TEXT("/Game/Map/Village+Forest/Village_Forest?listen"));
}

void ABaseController::ServerSelectCharacter_Implementation(ECharacterType NewCharacter)
{
    UE_LOG(LogTemp, Error, TEXT("===== ServerSelectCharacter Called ====="));

    ABasePlayerState* PS = Cast<ABasePlayerState>(PlayerState);

    if (!PS)
    {
        return;
    }

    PS->SelectedCharacter = NewCharacter;

    UE_LOG(LogTemp, Error,
        TEXT("Character = %d"),
        (int32)PS->SelectedCharacter);
}

void ABaseController::SelectCharacter(ECharacterType NewCharacter)
{
    ServerSelectCharacter(NewCharacter);
}

void ABaseController::ServerMoveToLocation_Implementation(
    FVector TargetLocation,
    FRotator TargetRotation)
{
    APawn* MyPawn = GetPawn();

    if (!MyPawn)
    {
        return;
    }

    ABaseCharacter* MyCharacter =
        Cast<ABaseCharacter>(MyPawn);

    if (!MyCharacter || MyCharacter->bIsDead)
    {
        StopMovement();
        return;
    }

    MyPawn->SetActorRotation(TargetRotation);

    UAIBlueprintHelperLibrary::SimpleMoveToLocation(
        this,
        TargetLocation);
}

ABaseController::ABaseController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ABaseController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsLocalController())
    {
        UpdateTreeTransparency();
    }
}

void ABaseController::UpdateTreeTransparency()
{
    APawn* MyPawn = GetPawn();

    if (!MyPawn)
    {
        return;
    }

    FVector Start = PlayerCameraManager->GetCameraLocation();
    FVector End = MyPawn->GetActorLocation();

    // 이전 프레임에 투명했던 나무 복원
    for (auto& Pair : FadedTrees)
    {
        for (int32 Index : Pair.Value)
        {
            Pair.Key->SetCustomDataValue(
                Index,
                0,
                0.0f,
                true);
        }
    }

    FadedTrees.Empty();

    TArray<FHitResult> Hits;

    TArray<AActor*> IgnoreActors;

    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        Start,
        End,
        350.0f,                         // ← 반지름 (조절 가능)
        UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1),
        false,
        IgnoreActors,
        EDrawDebugTrace::None,
        Hits,
        true);


    for (const FHitResult& Hit : Hits)
    {

        UHierarchicalInstancedStaticMeshComponent* HISM =
            Cast<UHierarchicalInstancedStaticMeshComponent>(Hit.Component.Get());

        if (!HISM)
        {
            continue;
        }

        int32 Index = Hit.Item;

        if (Index == INDEX_NONE)
        {
            continue;
        }

        HISM->SetCustomDataValue(
            Index,
            0,
            1.0f,
            true);

        FadedTrees.FindOrAdd(HISM).Add(Index);

    }
}

void ABaseController::ShowDeathScreen(bool bShouldEnableRetry)
{
    if (!IsLocalController()) return;

    if (!DeathScreenWidget)
    {
        DeathScreenWidget = CreateWidget<UDeathScreenWidget>(this, UDeathScreenWidget::StaticClass());
        if (DeathScreenWidget) DeathScreenWidget->AddToViewport(1000);
    }

    bRetryAvailable = bShouldEnableRetry;
    if (DeathScreenWidget) DeathScreenWidget->SetRetryAvailable(bShouldEnableRetry);

    if (bShouldEnableRetry)
    {
        bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        if (DeathScreenWidget)
        {
            InputMode.SetWidgetToFocus(DeathScreenWidget->TakeWidget());
        }
        SetInputMode(InputMode);
    }
}

void ABaseController::ClearDeathRestrictions()
{
    bRetryAvailable = false;
    bGameClearVisible = false;

    if (DeathScreenWidget)
    {
        DeathScreenWidget->RemoveFromParent();
        DeathScreenWidget = nullptr;
    }

    bShowMouseCursor = true;
    SetInputMode(FInputModeGameOnly());
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
}

void ABaseController::HandleOverlayClick()
{
    if (bRetryAvailable || bGameClearVisible)
    {
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        GetViewportSize(ViewportX, ViewportY);
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (!GetMousePosition(MouseX, MouseY)) return;
        const bool bOverCenterButton = MouseX >= ViewportX * 0.5f - 150.f
            && MouseX <= ViewportX * 0.5f + 150.f
            && MouseY >= ViewportY * 0.5f - 10.f
            && MouseY <= ViewportY * 0.5f + 65.f;

        if (bOverCenterButton)
        {
            if (bGameClearVisible) ExitGame();
            else RequestRetry();
        }
    }
}

void ABaseController::ClientShowYouDied_Implementation()
{
    ShowDeathScreen(false);
}

void ABaseController::ClientEnableRetry_Implementation()
{
    ShowDeathScreen(true);
}

void ABaseController::ClientShowGameClear_Implementation()
{
    bGameClearVisible = true;
    bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    SetInputMode(InputMode);
}

void ABaseController::RequestRetry()
{
    ServerRequestRetry();
}

void ABaseController::ServerRequestRetry_Implementation()
{
    if (AThe_Nightfall_SiegeGameMode* GameMode = GetWorld()->GetAuthGameMode<AThe_Nightfall_SiegeGameMode>())
    {
        GameMode->RequestPartyRetry(this);
    }
}

void ABaseController::ExitGame()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
