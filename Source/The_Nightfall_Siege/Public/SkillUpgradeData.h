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
	ESkillType SkillType = ESkillType::Q;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUpgradeType UpgradeType = EUpgradeType::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;
};
