// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseController.h"
#include "BaseCharacter.h"
#include "PauseMenuWidget.h"
#include "PausedOverlayWidget.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "BasePlayerState.h"
#include "LobbyWidget.h"
#include "TimerManager.h"
#include "The_Nightfall_SiegeGameMode.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "TheNightfallSiegeInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "The_Nightfall_SiegeGameMode.h"
#include "Kismet/KismetSystemLibrary.h"

void ABaseController::SetupInputComponent()
{
    Super::SetupInputComponent();

	InputComponent->BindAction("RightClick", IE_Pressed, this, &ABaseController::MoveToMouse);
	FInputKeyBinding& PauseBinding = InputComponent->BindKey(
		EKeys::Escape, IE_Pressed, this, &ABaseController::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused = true;
}

void ABaseController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsLocalController())
    {
        RefreshGlobalPausePresentation();
    }
}

void ABaseController::OnRightClick()
{
    MoveToMouse();
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
        if (ClickFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                ClickFX,
                Hit.Location,
                FRotator::ZeroRotator);
        }

        // Start path following on the owning machine. CharacterMovement then
        // sends its predicted moves to the server through Unreal's normal
        // movement RPCs. Running SimpleMove only in ServerMoveToLocation made
        // a remote player's autonomous proxy wait for replicated corrections,
        // which made the camera and pawn look as if they were updating at a
        // much lower frame rate than the listen-server host.
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);

        // A remote client's path-following request is local state and is not
        // created automatically on the authoritative controller. Start the
        // same path on the server so it validates movement toward the same
        // destination instead of continuously correcting the client back to
        // its starting point. The listen-server host already has authority,
        // so it must not start the path twice.
        if (!HasAuthority())
        {
            ServerStartMoveToLocation(Hit.Location);
        }
    }
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

    // The controller is first created in the lobby and persists through the
    // travel to the village.  Gameplay input is applied by the new pawn's
    // BeginPlay after travel so it cannot be overwritten by the lobby UI.
    if (!MapName.Contains(TEXT("Lvl_Lobby")))
    {
        // Gameplay must own keyboard focus.  GameAndUI leaves focus with the
        // previous lobby widget, which makes the first click/held movement key
        // appear to be dropped after travelling to this map.
        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(false);
        SetInputMode(InputMode);
        SetIgnoreMoveInput(false);
    }

    SetIgnoreLookInput(true);

    TreeComponents.Empty();

    

    if (IsLocalController())
    {
        GetWorld()->GetTimerManager().SetTimer(
            TreeTransparencyTimer,
            this,
            &ABaseController::UpdateTreeTransparency,
            0.15f,
            true
        );
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
	// Lobby pawns can still hold the default Archer's 300 HP. Character
	// selection starts a new run, so gameplay must initialize from the chosen
	// class maximum instead of carrying that lobby preview value forward.
	PS->SavedCurrentHP = -1.f;
	PS->ForceNetUpdate();

    UE_LOG(LogTemp, Error,
        TEXT("Character = %d"),
        (int32)PS->SelectedCharacter);
}

void ABaseController::SelectCharacter(ECharacterType NewCharacter)
{
    ServerSelectCharacter(NewCharacter);
}

void ABaseController::ServerStartMoveToLocation_Implementation(
    FVector TargetLocation)
{
    ABaseCharacter* MyCharacter = Cast<ABaseCharacter>(GetPawn());
    if (!MyCharacter || MyCharacter->bIsDead ||
        MyCharacter->bIsUsingSkill || MyCharacter->bIsAttacking)
    {
        StopMovement();
        return;
    }

    UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, TargetLocation);
}

ABaseController::ABaseController()
{
    PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FClassFinder<UUserWidget> DeathScreenWidgetBP(TEXT("/Game/BP/WBP_DeathScreen"));
    static ConstructorHelpers::FClassFinder<UUserWidget> GameClearWidgetBP(TEXT("/Game/BP/WBP_EndingCredits"));
    static ConstructorHelpers::FClassFinder<UPauseMenuWidget> PauseMenuWidgetBP(TEXT("/Game/BP/WBP_PauseMenuDesigner"));

    if (DeathScreenWidgetBP.Succeeded())
    {
        DeathScreenWidgetClass = DeathScreenWidgetBP.Class;
    }

    if (GameClearWidgetBP.Succeeded())
    {
        GameClearWidgetClass = GameClearWidgetBP.Class;
    }

    if (PauseMenuWidgetBP.Succeeded())
    {
        PauseMenuWidgetClass = PauseMenuWidgetBP.Class;
    }
}



void ABaseController::UpdateTreeTransparency()
{
    if (!IsLocalController())
    {
        return;
    }

    APawn* MyPawn = GetPawn();

    if (!MyPawn || !PlayerCameraManager)
    {
        return;
    }

    const FVector Start = PlayerCameraManager->GetCameraLocation();
    const FVector End = MyPawn->GetActorLocation();

    // 카메라나 캐릭터가 충분히 움직이지 않았다면
    // 같은 Trace를 다시 할 필요가 없다.
    constexpr float PositionThreshold = 30.0f;

    if (bHasLastTreeTracePosition &&
        FVector::DistSquared(Start, LastTreeTraceStart) <=
        FMath::Square(PositionThreshold) &&
        FVector::DistSquared(End, LastTreeTraceEnd) <=
        FMath::Square(PositionThreshold))
    {
        return;
    }

    LastTreeTraceStart = Start;
    LastTreeTraceEnd = End;
    bHasLastTreeTracePosition = true;

    TArray<FHitResult> Hits;
    TArray<AActor*> IgnoreActors;

    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        Start,
        End,
        350.0f,
        UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1),
        false,
        IgnoreActors,
        EDrawDebugTrace::None,
        Hits,
        true
    );

    TMap<UHierarchicalInstancedStaticMeshComponent*, TSet<int32>> NewFadedTrees;
    TSet<UHierarchicalInstancedStaticMeshComponent*> ComponentsNeedingRenderUpdate;

    // 현재 카메라와 캐릭터 사이에 있는 나무 수집
    for (const FHitResult& Hit : Hits)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM =
            Cast<UHierarchicalInstancedStaticMeshComponent>(
                Hit.Component.Get());

        if (!HISM)
        {
            continue;
        }

        const int32 Index = Hit.Item;

        if (Index == INDEX_NONE)
        {
            continue;
        }

        NewFadedTrees.FindOrAdd(HISM).Add(Index);
    }

    // 더 이상 가리지 않는 나무만 원래 상태로 복구
    for (auto& Pair : FadedTrees)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM = Pair.Key;

        if (!HISM)
        {
            continue;
        }

        const TSet<int32>* NewIndices = NewFadedTrees.Find(HISM);

        for (const int32 Index : Pair.Value)
        {
            if (!NewIndices || !NewIndices->Contains(Index))
            {
                HISM->SetCustomDataValue(
                    Index,
                    0,
                    0.0f,
                    false
                );
                ComponentsNeedingRenderUpdate.Add(HISM);
            }
        }
    }

    // 새롭게 가려진 나무만 투명화
    for (auto& Pair : NewFadedTrees)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM = Pair.Key;

        if (!HISM)
        {
            continue;
        }

        const TSet<int32>* OldIndices = FadedTrees.Find(HISM);

        for (const int32 Index : Pair.Value)
        {
            if (!OldIndices || !OldIndices->Contains(Index))
            {
                HISM->SetCustomDataValue(
                    Index,
                    0,
                    1.0f,
                    false
                );
                ComponentsNeedingRenderUpdate.Add(HISM);
            }
        }
    }

    FadedTrees = MoveTemp(NewFadedTrees);

    // Updating the render state per foliage instance can stall the game
    // thread. Batch all changed instances into one render-state refresh per
    // HISM component so click-to-move remains responsive.
    for (UHierarchicalInstancedStaticMeshComponent* HISM : ComponentsNeedingRenderUpdate)
    {
        if (HISM)
        {
            HISM->MarkRenderStateDirty();
        }
    }
}

void ABaseController::ShowDeathScreen(bool bShouldEnableRetry)
{
    if (!IsLocalController()) return;

    if (!DeathScreenWidget && DeathScreenWidgetClass)
    {
        DeathScreenWidget = CreateWidget<UUserWidget>(this, DeathScreenWidgetClass);
        if (DeathScreenWidget) DeathScreenWidget->AddToViewport(1000);
    }

    bRetryAvailable = bShouldEnableRetry;
    if (DeathScreenWidget)
    {
        if (UTextBlock* TitleText = Cast<UTextBlock>(DeathScreenWidget->GetWidgetFromName(TEXT("YouDiedText"))))
        {
            TitleText->SetText(FText::FromString(TEXT("YOU DIED")));
            TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.05f, 0.05f)));
        }

        if (UButton* RetryButton = Cast<UButton>(DeathScreenWidget->GetWidgetFromName(TEXT("RetryButton"))))
        {
            RetryButton->OnClicked.RemoveDynamic(this, &ABaseController::RequestRetry);
            RetryButton->OnClicked.AddDynamic(this, &ABaseController::RequestRetry);
            RetryButton->SetVisibility(bShouldEnableRetry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            RetryButton->SetIsEnabled(bShouldEnableRetry);
        }

        if (UTextBlock* RetryText = Cast<UTextBlock>(DeathScreenWidget->GetWidgetFromName(TEXT("RetryText"))))
        {
            RetryText->SetText(FText::FromString(TEXT("RETRY")));
        }
    }

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

    if (GameClearWidget)
    {
        GameClearWidget->RemoveFromParent();
        GameClearWidget = nullptr;
    }

    bShowMouseCursor = true;
    SetInputMode(FInputModeGameOnly());
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
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
    if (!IsLocalController())
    {
        return;
    }

    // 사망 화면 제거
    bRetryAvailable = false;

    if (DeathScreenWidget)
    {
        DeathScreenWidget->RemoveFromParent();
        DeathScreenWidget = nullptr;
    }

    // 이미 엔딩 화면 처리 중이면 중복 방지
    if (bGameClearVisible)
    {
        return;
    }

    // 엔딩 처리 시작 상태로 변경
    bGameClearVisible = true;
    bShowMouseCursor = true;

    // 보스 처치 후 5초 뒤 엔딩 WBP 표시
    GetWorld()->GetTimerManager().SetTimer(
        GameClearTimerHandle,
        [this]()
        {
            if (!IsValid(this) || !IsLocalController())
            {
                return;
            }

            // 엔딩 크레딧 WBP 생성
            if (!GameClearWidget && GameClearWidgetClass)
            {
                GameClearWidget =
                    CreateWidget<UUserWidget>(this, GameClearWidgetClass);

                if (GameClearWidget)
                {
                    GameClearWidget->AddToViewport(1100);
                }
            }

            // 엔딩 크레딧 동안 UI 입력
            FInputModeUIOnly InputMode;

            if (GameClearWidget)
            {
                InputMode.SetWidgetToFocus(
                    GameClearWidget->TakeWidget());
            }

            SetInputMode(InputMode);
        },
        5.0f,
        false
    );
}

void ABaseController::TogglePauseMenu()
{
	if (!IsLocalController())
    {
        return;
    }

    if (UGameplayStatics::IsGamePaused(this))
    {
        const AWorldSettings* WorldSettings = GetWorldSettings();
        if (WorldSettings &&
            WorldSettings->GetPauserPlayerState() == PlayerState)
        {
            ResumePausedGame();
        }
        return;
    }

    ServerRequestGlobalPause(true);
}

void ABaseController::ServerRequestGlobalPause_Implementation(bool bShouldPause)
{
    AWorldSettings* WorldSettings = GetWorldSettings();
    if (!WorldSettings || !PlayerState)
    {
        return;
    }

    APlayerState* CurrentPauser = WorldSettings->GetPauserPlayerState();

    if (bShouldPause)
    {
        // Use the authoritative version of the requesting controller so a
        // remote client's PlayerState, rather than the listen host, owns the
        // shared pause.
        FCanUnpause CanUnpauseDelegate;
        CanUnpauseDelegate.BindUObject(
            this, &ABaseController::CanClearGlobalPause);

        if (CurrentPauser || !SetPause(true, CanUnpauseDelegate))
        {
            return;
        }

        WorldSettings->ForceNetUpdate();
        return;
    }

    // Players viewing the read-only Paused notice cannot resume somebody
    // else's pause.
    if (CurrentPauser != PlayerState || !SetPause(false))
    {
        return;
    }

    // SetPause forces a WorldSettings update only when pausing. Push the
    // cleared PauserPlayerState immediately on resume as well.
    WorldSettings->ForceNetUpdate();
}

bool ABaseController::CanClearGlobalPause() const
{
    // The server RPC performs the ownership check. Binding this delegate to
    // the requesting controller also lets GameMode remove the pause cleanly
    // if that controller disconnects while the world is stopped.
    return true;
}

void ABaseController::RefreshGlobalPausePresentation()
{
    const AWorldSettings* WorldSettings = GetWorldSettings();
    APlayerState* Pauser = WorldSettings
        ? WorldSettings->GetPauserPlayerState()
        : nullptr;

    const bool bIsPaused = Pauser != nullptr;
    const bool bIsPauseOwner = bIsPaused && Pauser == PlayerState;

    if (bIsPaused)
    {
        if (!bGlobalPausePresentationActive ||
            bPauseMenuVisible != bIsPauseOwner)
        {
            ShowGlobalPausePresentation(bIsPauseOwner);
        }
    }
    else if (bGlobalPausePresentationActive)
    {
        HideGlobalPausePresentation();
    }
}

void ABaseController::ShowGlobalPausePresentation(bool bIsPauseOwner)
{
    const bool bWasPauseMenuVisible = bPauseMenuVisible;

    if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
    {
        PauseMenuWidget->RemoveFromParent();
    }
    if (PausedOverlayWidget && PausedOverlayWidget->IsInViewport())
    {
        PausedOverlayWidget->RemoveFromParent();
    }

    bPauseMenuVisible = false;
    bGlobalPausePresentationActive = true;

    if (bIsPauseOwner)
    {
        if (!PauseMenuWidget)
        {
            TSubclassOf<UPauseMenuWidget> WidgetClass = PauseMenuWidgetClass;
            if (!WidgetClass)
            {
                WidgetClass = UPauseMenuWidget::StaticClass();
            }
            PauseMenuWidget = CreateWidget<UPauseMenuWidget>(this, WidgetClass);
        }

        if (PauseMenuWidget)
        {
            PauseMenuWidget->AddToViewport(2000);
            bPauseMenuVisible = true;

            bShowMouseCursor = true;
            // Keep the legacy Escape binding available while Slate owns the
            // menu focus. Other gameplay bindings do not execute while the
            // world is paused because only Escape opts into paused input.
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(
                EMouseLockMode::DoNotLock);
            InputMode.SetHideCursorDuringCapture(false);
            SetInputMode(InputMode);
        }
        return;
    }

    if (bWasPauseMenuVisible)
    {
        RestoreGameplayInputAfterPause();
    }

    if (!PausedOverlayWidget)
    {
        PausedOverlayWidget = CreateWidget<UPausedOverlayWidget>(
            this, UPausedOverlayWidget::StaticClass());
    }

    if (PausedOverlayWidget)
    {
        PausedOverlayWidget->AddToViewport(2000);
    }
}

void ABaseController::HideGlobalPausePresentation()
{
    const bool bWasPauseMenuVisible = bPauseMenuVisible;

    if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
    {
        PauseMenuWidget->RemoveFromParent();
    }
    if (PausedOverlayWidget && PausedOverlayWidget->IsInViewport())
    {
        PausedOverlayWidget->RemoveFromParent();
    }

    bPauseMenuVisible = false;
    bGlobalPausePresentationActive = false;

    if (bWasPauseMenuVisible)
    {
        RestoreGameplayInputAfterPause();
    }
}

void ABaseController::RestoreGameplayInputAfterPause()
{
    bShowMouseCursor = true;
    FInputModeGameOnly InputMode;
    InputMode.SetConsumeCaptureMouseDown(false);
    SetInputMode(InputMode);
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
}

void ABaseController::ResumePausedGame()
{
    if (!IsLocalController() || !bPauseMenuVisible)
    {
        return;
    }

    ServerRequestGlobalPause(false);
}

void ABaseController::ExitPausedGame()
{
    if (bPauseMenuVisible)
    {
        // Wait for the server to release the shared pause before closing the
        // requesting client. Quitting immediately can drop the unpause RPC.
        ServerExitPausedGame();
        return;
    }
    ExitGame();
}

void ABaseController::ServerExitPausedGame_Implementation()
{
    AWorldSettings* WorldSettings = GetWorldSettings();
    if (!WorldSettings ||
        WorldSettings->GetPauserPlayerState() != PlayerState ||
        !SetPause(false))
    {
        return;
    }

    WorldSettings->ForceNetUpdate();
    ClientQuitAfterPauseReleased();
}

void ABaseController::ClientQuitAfterPauseReleased_Implementation()
{
    ExitGame();
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

void ABaseController::BackToTitle()
{
    ServerBackToTitle();
}

void ABaseController::ServerBackToTitle_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }

    AThe_Nightfall_SiegeGameMode* GameMode =
        GetWorld()->GetAuthGameMode<AThe_Nightfall_SiegeGameMode>();

    if (!GameMode || !GameMode->TryBeginReturnToTitle())
    {
        return;
    }

    // 모든 플레이어의 진행 상태 초기화
    if (GameMode->GameState)
    {
        for (APlayerState* ExistingPlayerState : GameMode->GameState->PlayerArray)
        {
            ABasePlayerState* PS =
                Cast<ABasePlayerState>(ExistingPlayerState);

            if (!PS)
            {
                continue;
            }

            PS->ResetForNewRun();
        }
    }

    // Reset the listen-server process immediately. Every remote process also
    // runs the same reset from GameInstance::HandlePostLoadMap on the title map.
    if (UTheNightfallSiegeInstance* GI =
        GetGameInstance<UTheNightfallSiegeInstance>())
    {
        GI->ResetForNewRun();
    }

    // GameMode 자체의 게임 종료 상태 초기화
    GameMode->ResetGameStateForNewRun();

    // 타이틀 화면으로 이동
    TArray<ABaseController*> ControllersToTravel;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ABaseController* Controller = Cast<ABaseController>(*It))
        {
            ControllersToTravel.Add(Controller);
        }
    }

    for (ABaseController* Controller : ControllersToTravel)
    {
        Controller->ClientTravel(
            TEXT("/Game/Level/Lvl_MainMenu"),
            ETravelType::TRAVEL_Absolute);
    }
}
