// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RaidGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API URaidGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

	// 전체 던전
	UPROPERTY(BlueprintReadWrite)
	TArray<FName> RemainingDungeons;

	// 현재 던전
	UPROPERTY(BlueprintReadWrite)
	FName CurrentDungeon;

	// 클리어 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 ClearedDungeonCount;

	// 레이드 시작
	void StartRaid();

	// 다음 던전 선택
	FName SelectNextDungeon();

	// 현재 던전 클리어
	bool ClearCurrentDungeon();
};
