// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterType.h"
#include "BasePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API ABasePlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ABasePlayerState();

	UPROPERTY(Replicated, BlueprintReadOnly)
	ECharacterType SelectedCharacter = ECharacterType::Paladin;

protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bReady = false;
};
