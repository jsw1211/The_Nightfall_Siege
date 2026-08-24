// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterType.h"
#include "ShopInventoryTypes.h"
#include "SkillUpgradeData.h"
#include "BasePlayerState.generated.h"

UENUM(BlueprintType)
enum class EQuestStage : uint8
{
    NotAccepted, FindDungeonPortal, ClearDungeon, CollectPrism, ReturnToVillage,
    SpendSkillPoint,
    FindBossPortal, DefeatBoss, Completed
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ABasePlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ABasePlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacter, BlueprintReadOnly)
	ECharacterType SelectedCharacter = ECharacterType::Archer;

	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bHasLantern = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bLanternEquipped = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bHasPrism = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bPrismEquipped = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 Coin = 0;

	// Gold is committed only after a dungeon is cleared.  This checkpoint is
	// captured just before entering a dungeon and restored when that attempt
	// is abandoned through the party retry flow.
	UPROPERTY()
	int32 DungeonEntryCoin = 0;

	UPROPERTY()
	bool bHasDungeonCoinCheckpoint = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 PotionCount = 0;

	// The pawn is recreated on every dungeon/boss travel. Keep the complete
	// shop inventory and its quick-slot selection on the persistent PlayerState.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	TArray<FShopInventoryItem> PurchasedItems;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 Slot4PurchasedItemIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_SkillProgress, BlueprintReadWrite)
	int32 SkillPoints = 0;

	UPROPERTY(ReplicatedUsing = OnRep_SkillProgress, BlueprintReadOnly, Category = "Skill")
	int32 QSkillLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_SkillProgress, BlueprintReadOnly, Category = "Skill")
	int32 WSkillLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_SkillProgress, BlueprintReadOnly, Category = "Skill")
	int32 ESkillLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_SkillProgress, BlueprintReadOnly, Category = "Skill")
	int32 RSkillLevel = 1;

	void CopySkillLevelsTo(TMap<ESkillType, int32>& OutSkillLevels) const;
	void SetSkillLevelsFrom(const TMap<ESkillType, int32>& InSkillLevels);

	UFUNCTION()
	void OnRep_SkillProgress();

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    EQuestStage QuestStage = EQuestStage::NotAccepted;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    int32 ClearedDungeonCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    int32 DungeonMonsterKillCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    int32 DungeonMonsterTotalCount = 0;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void AcceptMainQuest();
    void NotifyDungeonEntered();
    void NotifyDungeonCleared();
    void SetDungeonMonsterTotal(int32 TotalCount);
    void NotifyDungeonMonsterKilled();
    void NotifySkillPointSpent();
    void NotifyPrismCollected();
    void NotifyReturnedToVillage();
    void NotifyBossPortalEntered();
    void NotifyBossDefeated();

    // Quest objectives are shared by the whole listen-server party.  Call
    // this on the server after directly assigning a quest field.
    void SyncQuestProgressToParty();
    void CopyQuestProgressFrom(const ABasePlayerState& Source);

    UFUNCTION(BlueprintPure, Category = "Quest")
    FText GetQuestObjectiveText() const;

	// Permanent shop upgrades survive pawn replacement and seamless map travel.
	UPROPERTY(ReplicatedUsing = OnRep_PersistentStats, BlueprintReadWrite)
	bool bHasShopStatBonuses = false;

	UPROPERTY(ReplicatedUsing = OnRep_PersistentStats, BlueprintReadWrite)
	float SavedMaxHP = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_PersistentStats, BlueprintReadWrite)
	float SavedAttackPower = 0.f;

	UFUNCTION()
	void OnRep_PersistentStats();

	UFUNCTION()
	void OnRep_SelectedCharacter();

	UFUNCTION(BlueprintCallable)
	void SetReady(bool bNewReady);

	UFUNCTION(BlueprintCallable)
	bool IsReady() const;

protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void CopyProperties(APlayerState* PlayerState) override;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bReady = false;
};
