// Fill out your copyright notice in the Description page of Project Settings.


#include "TheNightfallSiegeInstance.h"

void UTheNightfallSiegeInstance::Init()
{
    Super::Init();

    StartRaid();

    SelectNextDungeon();
}

void UTheNightfallSiegeInstance::StartRaid()
{
    RemainingDungeons.Empty();

    RemainingDungeons.Add("LV_Dungeon1");

    ClearedDungeonCount = 0;
}

FName UTheNightfallSiegeInstance::SelectNextDungeon()
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

bool UTheNightfallSiegeInstance::ClearCurrentDungeon()
{
    RemainingDungeons.Remove(CurrentDungeon);

    ClearedDungeonCount++;

    return ClearedDungeonCount >= 3;
}