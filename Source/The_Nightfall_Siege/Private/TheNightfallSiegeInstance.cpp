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
}

void UTheNightfallSiegeInstance::StartRaid()
{
    RemainingDungeons.Empty();

    // Each raid gets a random dungeon order.  A retry keeps CurrentDungeon,
    // so only the failed entry in that order is attempted again.
    RemainingDungeons = { "LV_Dungeon1", "LV_Dungeon2", "LV_Dungeon3" };

    DungeonPortalLocations =
    {
        FVector(27900.f, 28190.f, 870.f),
        FVector(-440.f, 13730.f, 0.f),
        FVector(26900.f, 14330.f, 0.f),
        FVector(31450.f, 1070.f, 2290.f),
        FVector(19370.f, 6540.f, 1980.f),
        FVector(6360.f, 25440.f, 0.f)
    };

    RemainingDungeonPortalLocations = DungeonPortalLocations;

    ClearedDungeonCount = 0;
    CurrentDungeon = NAME_None;
    bBossPortalSpawned = false;
    bRetryingCurrentDungeon = false;
    bRetryingBoss = false;
}

void UTheNightfallSiegeInstance::BeginRetry(bool bWasBossEncounter)
{
    bRetryingBoss = bWasBossEncounter;
    bRetryingCurrentDungeon = !bWasBossEncounter && !CurrentDungeon.IsNone();
}

bool UTheNightfallSiegeInstance::ConsumeDungeonRetry()
{
    const bool bRetry = bRetryingCurrentDungeon;
    bRetryingCurrentDungeon = false;
    return bRetry;
}

FName UTheNightfallSiegeInstance::SelectNextDungeon()
{
    if (RemainingDungeons.Num() <= 0)
    {
        return NAME_None;
    }

    const int32 RandomIndex = FMath::RandRange(0, RemainingDungeons.Num() - 1);
    CurrentDungeon = RemainingDungeons[RandomIndex];

    return CurrentDungeon;
}

bool UTheNightfallSiegeInstance::SelectNextDungeonPortalLocation(FVector& OutLocation)
{
    if (RemainingDungeonPortalLocations.IsEmpty())
    {
        return false;
    }

    const int32 RandomIndex = FMath::RandRange(0, RemainingDungeonPortalLocations.Num() - 1);
    OutLocation = RemainingDungeonPortalLocations[RandomIndex];
    RemainingDungeonPortalLocations.RemoveAtSwap(RandomIndex);
    return true;
}

bool UTheNightfallSiegeInstance::SelectBossPortalLocation(FVector& OutLocation) const
{
    if (DungeonPortalLocations.IsEmpty())
    {
        return false;
    }

    const int32 RandomIndex = FMath::RandRange(0, DungeonPortalLocations.Num() - 1);
    OutLocation = DungeonPortalLocations[RandomIndex];
    return true;
}

bool UTheNightfallSiegeInstance::ClearCurrentDungeon()
{
    UE_LOG(LogTemp, Warning, TEXT("CurrentDungeon : %s"), *CurrentDungeon.ToString());

    if (CurrentDungeon.IsNone() || !RemainingDungeons.RemoveSingle(CurrentDungeon))
    {
        return ClearedDungeonCount >= 3;
    }

    UE_LOG(LogTemp, Warning, TEXT("Remaining : %d"), RemainingDungeons.Num());

    ClearedDungeonCount++;

    return ClearedDungeonCount >= 3;
}
