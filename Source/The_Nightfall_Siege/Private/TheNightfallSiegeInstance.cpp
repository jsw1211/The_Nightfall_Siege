// Fill out your copyright notice in the Description page of Project Settings.


#include "TheNightfallSiegeInstance.h"

void UTheNightfallSiegeInstance::Init()
{
    Super::Init();

    SkillLevels.Add(ESkillType::Q, 1);
    SkillLevels.Add(ESkillType::W, 1);
    SkillLevels.Add(ESkillType::E, 1);
    SkillLevels.Add(ESkillType::R, 1);

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
    UE_LOG(LogTemp, Warning, TEXT("CurrentDungeon : %s"), *CurrentDungeon.ToString());

    RemainingDungeons.Remove(CurrentDungeon);

    UE_LOG(LogTemp, Warning, TEXT("Remaining : %d"), RemainingDungeons.Num());

    ClearedDungeonCount++;

    return ClearedDungeonCount >= 1;
}