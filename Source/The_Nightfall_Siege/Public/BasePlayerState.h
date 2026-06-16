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

	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacter, BlueprintReadOnly)
	ECharacterType SelectedCharacter = ECharacterType::Paladin;

	UFUNCTION()
	void OnRep_SelectedCharacter();

	UFUNCTION(BlueprintCallable)
	void SetReady(bool bNewReady);

	UFUNCTION(BlueprintCallable)
	bool IsReady() const;

protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void CopyProperties(APlayerState* PlayerState) override;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bReady = false;
};
