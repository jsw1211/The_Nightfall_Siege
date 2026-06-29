// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BaseLobbyGameState.generated.h"

/**
 * 
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API ABaseLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	ABaseLobbyGameState();

protected:

	virtual void BeginPlay() override;

};
