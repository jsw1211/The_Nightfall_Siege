#if WITH_DEV_AUTOMATION_TESTS

#include "BasePlayerState.h"
#include "Misc/AutomationTest.h"
#include "TheNightfallSiegeInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameInstanceNewRunResetTest,
    "TheNightfallSiege.NewRun.GameInstanceReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameInstanceNewRunResetTest::RunTest(const FString& Parameters)
{
    UTheNightfallSiegeInstance* GameInstance = NewObject<UTheNightfallSiegeInstance>();
    if (!TestNotNull(TEXT("Game instance should be created"), GameInstance))
    {
        return false;
    }

    GameInstance->SelectedCharacter = ECharacterType::Warrior;
    GameInstance->RemainingDungeons.Empty();
    GameInstance->DungeonPortalLocations.Empty();
    GameInstance->CurrentDungeon = TEXT("LV_Dungeon3");
    GameInstance->ClearedDungeonCount = 3;
    GameInstance->bHasLantern = true;
    GameInstance->bLanternEquipped = true;
    GameInstance->bHasPrism = true;
    GameInstance->bPrismEquipped = true;
    GameInstance->SkillPoints = 12;
    GameInstance->SkillLevels.FindOrAdd(ESkillType::Q) = 4;
    GameInstance->SkillLevels.FindOrAdd(ESkillType::W) = 4;
    GameInstance->SkillLevels.FindOrAdd(ESkillType::E) = 4;
    GameInstance->SkillLevels.FindOrAdd(ESkillType::R) = 4;
    GameInstance->bBossPortalSpawned = true;
    GameInstance->bRetryingCurrentDungeon = true;
    GameInstance->bRetryingBoss = true;
    GameInstance->bIsHost = true;
    GameInstance->bWorldLanternDestroyed = true;
    GameInstance->SetPlayerNickname(TEXT("OldPlayer"));
    GameInstance->SaveTravelHealth(7, 1.f);

    GameInstance->ResetForNewRun();
    GameInstance->ResetForNewRun();

    TestTrue(TEXT("Character should reset to Archer"),
        GameInstance->SelectedCharacter == ECharacterType::Archer);
    TestEqual(TEXT("All three dungeons should be available"),
        GameInstance->RemainingDungeons.Num(), 3);
    TestEqual(TEXT("All six portal locations should be configured"),
        GameInstance->DungeonPortalLocations.Num(), 6);
    TestTrue(TEXT("Current dungeon should be empty"), GameInstance->CurrentDungeon.IsNone());
    TestEqual(TEXT("Cleared dungeon count should reset"), GameInstance->ClearedDungeonCount, 0);
    TestFalse(TEXT("Lantern ownership should reset"), GameInstance->bHasLantern);
    TestFalse(TEXT("Lantern equipment should reset"), GameInstance->bLanternEquipped);
    TestFalse(TEXT("Prism ownership should reset"), GameInstance->bHasPrism);
    TestFalse(TEXT("Prism equipment should reset"), GameInstance->bPrismEquipped);
    TestEqual(TEXT("Skill points should reset"), GameInstance->SkillPoints, 0);
    TestEqual(TEXT("Exactly four skill levels should be configured"),
        GameInstance->SkillLevels.Num(), 4);
    TestEqual(TEXT("Q skill should reset"), GameInstance->SkillLevels.FindRef(ESkillType::Q), 1);
    TestEqual(TEXT("W skill should reset"), GameInstance->SkillLevels.FindRef(ESkillType::W), 1);
    TestEqual(TEXT("E skill should reset"), GameInstance->SkillLevels.FindRef(ESkillType::E), 1);
    TestEqual(TEXT("R skill should reset"), GameInstance->SkillLevels.FindRef(ESkillType::R), 1);
    TestFalse(TEXT("Boss portal state should reset"), GameInstance->bBossPortalSpawned);
    TestFalse(TEXT("Dungeon retry state should reset"), GameInstance->bRetryingCurrentDungeon);
    TestFalse(TEXT("Boss retry state should reset"), GameInstance->bRetryingBoss);
    TestFalse(TEXT("Host state should reset"), GameInstance->bIsHost);
    TestFalse(TEXT("World lantern state should reset"), GameInstance->bWorldLanternDestroyed);
    TestTrue(TEXT("Nickname should reset"), GameInstance->GetPlayerNickname().IsEmpty());

    float TravelHealth = -1.f;
    TestFalse(TEXT("Pending travel health should be cleared"),
        GameInstance->ConsumeTravelHealth(7, TravelHealth));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStateNewRunResetTest,
    "TheNightfallSiege.NewRun.PlayerStateReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerStateNewRunResetTest::RunTest(const FString& Parameters)
{
    ABasePlayerState* PlayerState = NewObject<ABasePlayerState>();
    if (!TestNotNull(TEXT("Player state should be created"), PlayerState))
    {
        return false;
    }

    PlayerState->SelectedCharacter = ECharacterType::Paladin;
    PlayerState->SetReady(true);
    PlayerState->bHasLantern = true;
    PlayerState->bLanternEquipped = true;
    PlayerState->bHasPrism = true;
    PlayerState->bPrismEquipped = true;
    PlayerState->bPrismCleansePressed = true;
    PlayerState->Coin = 999;
    PlayerState->DungeonEntryCoin = 500;
    PlayerState->bHasDungeonCoinCheckpoint = true;
    PlayerState->PotionCount = 8;
    PlayerState->PurchasedItems.AddDefaulted();
    PlayerState->Slot4PurchasedItemIndex = 0;
    PlayerState->SkillPoints = 9;
    PlayerState->QSkillLevel = 4;
    PlayerState->WSkillLevel = 4;
    PlayerState->ESkillLevel = 4;
    PlayerState->RSkillLevel = 4;
    PlayerState->QuestStage = EQuestStage::Completed;
    PlayerState->ClearedDungeonCount = 3;
    PlayerState->DungeonMonsterKillCount = 20;
    PlayerState->DungeonMonsterTotalCount = 20;
    PlayerState->bHasShopStatBonuses = true;
    PlayerState->SavedMaxHP = 1000.f;
    PlayerState->SavedCurrentHP = 10.f;
    PlayerState->SavedAttackPower = 100.f;
    PlayerState->SetScore(50.f);

    PlayerState->ResetForNewRun();

    TestTrue(TEXT("Character should reset to Archer"),
        PlayerState->SelectedCharacter == ECharacterType::Archer);
    TestFalse(TEXT("Ready state should reset"), PlayerState->IsReady());
    TestFalse(TEXT("Lantern ownership should reset"), PlayerState->bHasLantern);
    TestFalse(TEXT("Lantern equipment should reset"), PlayerState->bLanternEquipped);
    TestFalse(TEXT("Prism ownership should reset"), PlayerState->bHasPrism);
    TestFalse(TEXT("Prism equipment should reset"), PlayerState->bPrismEquipped);
    TestFalse(TEXT("Prism cleanse input should reset"), PlayerState->bPrismCleansePressed);
    TestEqual(TEXT("Gold should reset"), PlayerState->Coin, 0);
    TestEqual(TEXT("Dungeon gold checkpoint should reset"), PlayerState->DungeonEntryCoin, 0);
    TestFalse(TEXT("Dungeon checkpoint flag should reset"),
        PlayerState->bHasDungeonCoinCheckpoint);
    TestEqual(TEXT("Potion count should reset"), PlayerState->PotionCount, 0);
    TestTrue(TEXT("Purchased items should reset"), PlayerState->PurchasedItems.IsEmpty());
    TestEqual(TEXT("Quick slot should reset"), PlayerState->Slot4PurchasedItemIndex, INDEX_NONE);
    TestEqual(TEXT("Skill points should reset"), PlayerState->SkillPoints, 0);
    TestEqual(TEXT("Q skill should reset"), PlayerState->QSkillLevel, 1);
    TestEqual(TEXT("W skill should reset"), PlayerState->WSkillLevel, 1);
    TestEqual(TEXT("E skill should reset"), PlayerState->ESkillLevel, 1);
    TestEqual(TEXT("R skill should reset"), PlayerState->RSkillLevel, 1);
    TestTrue(TEXT("Quest should reset"), PlayerState->QuestStage == EQuestStage::NotAccepted);
    TestEqual(TEXT("Quest clear count should reset"), PlayerState->ClearedDungeonCount, 0);
    TestEqual(TEXT("Quest kill count should reset"), PlayerState->DungeonMonsterKillCount, 0);
    TestEqual(TEXT("Quest monster total should reset"), PlayerState->DungeonMonsterTotalCount, 0);
    TestFalse(TEXT("Shop stat bonus flag should reset"), PlayerState->bHasShopStatBonuses);
    TestEqual(TEXT("Saved max HP should reset"), PlayerState->SavedMaxHP, 0.f);
    TestEqual(TEXT("Saved current HP should reset"), PlayerState->SavedCurrentHP, -1.f);
    TestEqual(TEXT("Saved attack should reset"), PlayerState->SavedAttackPower, 0.f);
    TestEqual(TEXT("Score should reset"), PlayerState->GetScore(), 0.f);
    return true;
}

#endif
