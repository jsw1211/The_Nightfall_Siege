// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterType.h"
#include "SkillUpgradeData.h"
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

	UPROPERTY(BlueprintReadWrite)
	TArray<FName> RemainingDungeons;

	UPROPERTY(BlueprintReadWrite)
	FName CurrentDungeon;

	UPROPERTY(BlueprintReadWrite)
	int32 ClearedDungeonCount;

	void StartRaid();

	FName SelectNextDungeon();

	bool ClearCurrentDungeon();

	virtual void Init() override;

	UPROPERTY(BlueprintReadWrite)
	bool bHasLantern = false;

	UPROPERTY(BlueprintReadWrite)
	bool bLanternEquipped = false;

	UPROPERTY(BlueprintReadWrite)
	bool bHasPrism = false;

	UPROPERTY(BlueprintReadWrite)
	bool bPrismEquipped = false;

	UPROPERTY(BlueprintReadWrite)
	int32 SkillPoints = 5;

	UPROPERTY(BlueprintReadWrite)
	TMap<ESkillType, int32> SkillLevels;

	UPROPERTY(BlueprintReadWrite)
	bool bBossPortalSpawned = false;
};
