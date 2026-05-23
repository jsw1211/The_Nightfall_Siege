// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillUpgradeData.generated.h"

UENUM(BlueprintType)
enum class ESkillType : uint8
{
	Q,
	W,
	E,
	R
};

UENUM(BlueprintType)
enum class EUpgradeType : uint8
{
	Damage,
	Cooldown,
	Defense,
	Heal,
	AttackSpeed,
	Range
};

USTRUCT(BlueprintType)
struct FSkillUpgradeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESkillType SkillType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUpgradeType UpgradeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;
};
