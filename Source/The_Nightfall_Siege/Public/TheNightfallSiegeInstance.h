// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterType.h"
#include "TheNightfallSiegeInstance.generated.h"

/**
 * 
 */

UCLASS()
class THE_NIGHTFALL_SIEGE_API UTheNightfallSiegeInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECharacterType SelectedCharacter = ECharacterType::Paladin;
};
