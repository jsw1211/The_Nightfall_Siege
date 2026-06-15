// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "CharacterType.h"
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

	void RotateCharacterToCursor();

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
};
