// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"



ABasePlayerState::ABasePlayerState()
{
    bReplicates = true;
}

void ABasePlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABasePlayerState, SelectedCharacter);
    DOREPLIFETIME(ABasePlayerState, bReady);

    DOREPLIFETIME(ABasePlayerState, bHasLantern);
    DOREPLIFETIME(ABasePlayerState, bLanternEquipped);

    DOREPLIFETIME(ABasePlayerState, bHasPrism);
    DOREPLIFETIME(ABasePlayerState, bPrismEquipped);

    DOREPLIFETIME(ABasePlayerState, Coin);
    DOREPLIFETIME(ABasePlayerState, PotionCount);
    DOREPLIFETIME(ABasePlayerState, SkillPoints);
    DOREPLIFETIME(ABasePlayerState, QuestStage);
    DOREPLIFETIME(ABasePlayerState, ClearedDungeonCount);
    DOREPLIFETIME(ABasePlayerState, DungeonMonsterKillCount);
    DOREPLIFETIME(ABasePlayerState, DungeonMonsterTotalCount);
    DOREPLIFETIME(ABasePlayerState, bHasShopStatBonuses);
    DOREPLIFETIME(ABasePlayerState, SavedMaxHP);
    DOREPLIFETIME(ABasePlayerState, SavedAttackPower);
}

void ABasePlayerState::OnRep_SelectedCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("Rep Character Changed"));
}

void ABasePlayerState::AcceptMainQuest()
{
    if (QuestStage == EQuestStage::NotAccepted)
    {
        QuestStage = EQuestStage::FindDungeonPortal;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifyDungeonEntered()
{
    if (QuestStage == EQuestStage::FindDungeonPortal)
    {
        QuestStage = EQuestStage::ClearDungeon;
    }

    // Retrying a failed dungeon keeps the quest in ClearDungeon.  Reset the
    // previous attempt's UI progress here as well, before the new dungeon
    // manager publishes its freshly spawned monster total.
    if (QuestStage == EQuestStage::ClearDungeon)
    {
        DungeonMonsterKillCount = 0;
        DungeonMonsterTotalCount = 0;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifyDungeonCleared()
{
    if (QuestStage == EQuestStage::ClearDungeon)
    {
        QuestStage = EQuestStage::CollectPrism;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::SetDungeonMonsterTotal(int32 TotalCount)
{
    DungeonMonsterTotalCount = FMath::Max(0, TotalCount);
    DungeonMonsterKillCount = FMath::Clamp(DungeonMonsterKillCount, 0, DungeonMonsterTotalCount);
    SyncQuestProgressToParty();
}

void ABasePlayerState::NotifyDungeonMonsterKilled()
{
    if (QuestStage == EQuestStage::ClearDungeon && DungeonMonsterTotalCount > 0)
    {
        DungeonMonsterKillCount = FMath::Min(DungeonMonsterKillCount + 1, DungeonMonsterTotalCount);
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifySkillPointSpent()
{
    if (QuestStage == EQuestStage::SpendSkillPoint)
    {
        QuestStage = ClearedDungeonCount >= 3
            ? EQuestStage::FindBossPortal
            : EQuestStage::FindDungeonPortal;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifyPrismCollected()
{
    if (QuestStage != EQuestStage::CollectPrism) return;
    ++ClearedDungeonCount;
    QuestStage = EQuestStage::ReturnToVillage;
    SyncQuestProgressToParty();
}

void ABasePlayerState::NotifyReturnedToVillage()
{
    if (QuestStage == EQuestStage::ReturnToVillage)
    {
        QuestStage = EQuestStage::SpendSkillPoint;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifyBossPortalEntered()
{
    if (QuestStage == EQuestStage::FindBossPortal)
    {
        QuestStage = EQuestStage::DefeatBoss;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::NotifyBossDefeated()
{
    if (QuestStage == EQuestStage::DefeatBoss)
    {
        QuestStage = EQuestStage::Completed;
        SyncQuestProgressToParty();
    }
}

void ABasePlayerState::CopyQuestProgressFrom(const ABasePlayerState& Source)
{
    QuestStage = Source.QuestStage;
    ClearedDungeonCount = Source.ClearedDungeonCount;
    DungeonMonsterKillCount = Source.DungeonMonsterKillCount;
    DungeonMonsterTotalCount = Source.DungeonMonsterTotalCount;
}

void ABasePlayerState::SyncQuestProgressToParty()
{
    if (!HasAuthority())
    {
        return;
    }

    AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
    if (!GameState)
    {
        return;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (ABasePlayerState* PartyMember = Cast<ABasePlayerState>(PlayerState))
        {
            PartyMember->CopyQuestProgressFrom(*this);
            PartyMember->ForceNetUpdate();
        }
    }
}

FText ABasePlayerState::GetQuestObjectiveText() const
{
    switch (QuestStage)
    {
    case EQuestStage::NotAccepted: return FText::FromString(TEXT("F를 눌러 마을 촌장에게 퀘스트를 받으세요."));
    case EQuestStage::FindDungeonPortal: return FText::FromString(FString::Printf(TEXT("던전 포탈을 찾으세요. (%d/3 완료)"), ClearedDungeonCount));
    case EQuestStage::ClearDungeon: return FText::FromString(TEXT("던전의 몬스터를 모두 처치하세요."));
    case EQuestStage::CollectPrism: return FText::FromString(TEXT("던전 프리즘을 획득하세요."));
    case EQuestStage::ReturnToVillage: return FText::FromString(TEXT("마을로 돌아가 스킬 트리를 강화하세요."));
    case EQuestStage::SpendSkillPoint: return FText::FromString(TEXT("스킬 포인트를 사용해 스킬을 강화하세요."));
    case EQuestStage::FindBossPortal: return FText::FromString(TEXT("보스가 등장했습니다. 보스 포탈을 찾으세요."));
    case EQuestStage::DefeatBoss: return FText::FromString(TEXT("보스를 처치하세요."));
    case EQuestStage::Completed: return FText::FromString(TEXT("메인 퀘스트를 완료했습니다!"));
    default: return FText::GetEmpty();
    }
}

void ABasePlayerState::SetReady(bool bNewReady)
{
    bReady = bNewReady;
}

bool ABasePlayerState::IsReady() const
{
    return bReady;
}

void ABasePlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    UE_LOG(LogTemp, Error,
        TEXT("===== CopyProperties ====="));

    UE_LOG(LogTemp, Error,
        TEXT("Copy Character = %d"),
        (int32)SelectedCharacter);

    ABasePlayerState* NewPS =
        Cast<ABasePlayerState>(PlayerState);

    if (!NewPS)
    {
        return;
    }

    NewPS->SelectedCharacter = SelectedCharacter;
    NewPS->bReady = bReady;

    NewPS->bHasLantern = bHasLantern;
    NewPS->bLanternEquipped = bLanternEquipped;

    NewPS->bHasPrism = bHasPrism;
    NewPS->bPrismEquipped = bPrismEquipped;

    NewPS->Coin = Coin;
	NewPS->DungeonEntryCoin = DungeonEntryCoin;
	NewPS->bHasDungeonCoinCheckpoint = bHasDungeonCoinCheckpoint;
    NewPS->PotionCount = PotionCount;
    NewPS->SkillPoints = SkillPoints;
    NewPS->QuestStage = QuestStage;
    NewPS->ClearedDungeonCount = ClearedDungeonCount;
    NewPS->DungeonMonsterKillCount = DungeonMonsterKillCount;
    NewPS->DungeonMonsterTotalCount = DungeonMonsterTotalCount;
    NewPS->bHasShopStatBonuses = bHasShopStatBonuses;
    NewPS->SavedMaxHP = SavedMaxHP;
    NewPS->SavedAttackPower = SavedAttackPower;

    UE_LOG(LogTemp, Warning,
        TEXT("Copy Lantern : %d"),
        bHasLantern);

    UE_LOG(LogTemp, Warning,
        TEXT("Copy Prism : %d"),
        bHasPrism);

}

