// Fill out your copyright notice in the Description page of Project Settings.


#include "RaidGameInstance.h"

void URaidGameInstance::Init()
{
	Super::Init();

	StartRaid();
}

void URaidGameInstance::StartRaid()
{
	RemainingDungeons.Empty();

	RemainingDungeons.Add("DragonLevelSample");
	RemainingDungeons.Add("DragonLevelSample");
	RemainingDungeons.Add("DragonLevelSample");

	ClearedDungeonCount = 0;
}

FName URaidGameInstance::SelectNextDungeon()
{
	if (RemainingDungeons.Num() <= 0)
	{
		return NAME_None;
	}

	int32 RandomIndex =
		FMath::RandRange(0, RemainingDungeons.Num() - 1);

	CurrentDungeon = RemainingDungeons[RandomIndex];

	return CurrentDungeon;
}

bool URaidGameInstance::ClearCurrentDungeon()
{
	RemainingDungeons.Remove(CurrentDungeon);

	ClearedDungeonCount++;

	return ClearedDungeonCount >= 3;
}