// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "CharacterType.h"
#include "NiagaraSystem.h"

class UHierarchicalInstancedStaticMeshComponent;
class UPauseMenuWidget;

#include "BaseController.generated.h"

/**
 * 
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API ABaseController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

	void OnRightClick();

	void MoveToMouse();

	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSubclassOf<class UUserWidget> CharacterSelectWidgetClass;

	UPROPERTY()
	UUserWidget* CharacterSelectWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY()
	UUserWidget* LobbyWidget;

	UFUNCTION(Server, Reliable)
	void ServerSelectCharacter(ECharacterType NewCharacter);

	UFUNCTION(BlueprintCallable)
	void SelectCharacter(ECharacterType NewCharacter);

	FTimerHandle LobbyRefreshHandle;

	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bNewReady);

	UFUNCTION(Server, Reliable)
	void ServerStartGame();

	public:
	UFUNCTION(Server, Reliable)
	void ServerStartMoveToLocation(FVector TargetLocation);

	UFUNCTION(Client, Reliable)
	void ClientShowYouDied();

	UFUNCTION(Client, Reliable)
	void ClientEnableRetry();

	UFUNCTION(Server, Reliable)
	void ServerRequestRetry();

	void ShowDeathScreen(bool bShouldEnableRetry);
	// Called by a newly spawned pawn after retry travel to remove every
	// death-screen input/UI restriction left on the local controller.
	void ClearDeathRestrictions();

	UFUNCTION(Client, Reliable)
	void ClientShowGameClear();

	void TogglePauseMenu();
	void ResumePausedGame();
	void ExitPausedGame();

	bool IsRetryAvailable() const { return bRetryAvailable; }
	bool IsGameClearVisible() const { return bGameClearVisible; }

	protected:

	UPROPERTY(EditAnywhere, Category = "FX")
	UNiagaraSystem* ClickFX;

	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> TreeComponents;

	void UpdateTreeTransparency();

	TMap<UHierarchicalInstancedStaticMeshComponent*, TSet<int32>> FadedTrees;

	FTimerHandle TreeTransparencyTimer;

	FVector LastTreeTraceStart = FVector::ZeroVector;
	FVector LastTreeTraceEnd = FVector::ZeroVector;

	bool bHasLastTreeTracePosition = false;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Death")
	TSubclassOf<UUserWidget> DeathScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Game Clear")
	TSubclassOf<UUserWidget> GameClearWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Pause")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> DeathScreenWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> GameClearWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget = nullptr;

	bool bRetryAvailable = false;
	bool bGameClearVisible = false;
	bool bPauseMenuVisible = false;

	FTimerHandle GameClearTimerHandle;


public:

	ABaseController();

	UFUNCTION(BlueprintCallable)
	void SelectNextCharacter();

	UFUNCTION(BlueprintCallable)
	void ToggleReady();

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Death")
	void RequestRetry();

	UFUNCTION(BlueprintCallable, Category = "Game Clear")
	void ExitGame();

	UFUNCTION(Server, Reliable)
	void ServerBackToTitle();

	UFUNCTION(BlueprintCallable)
	void BackToTitle();
};
