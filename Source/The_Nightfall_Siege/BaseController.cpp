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

void ABaseController::SetupInputComponent()
{
    Super::SetupInputComponent();

	InputComponent->BindAction("RightClick", IE_Pressed, this, &ABaseController::MoveToMouse);
}

void ABaseController::OnRightClick()
{
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

    GetWorld()->ServerTravel(TEXT("/Game/TopDown/Lvl_TopDown?listen"));
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

    MyPawn->SetActorRotation(TargetRotation);

    UAIBlueprintHelperLibrary::SimpleMoveToLocation(
        this,
        TargetLocation);
}

