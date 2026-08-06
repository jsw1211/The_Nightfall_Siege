// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterType.h"
#include "BasePlayerState.generated.h"

UENUM(BlueprintType)
enum class EQuestStage : uint8
{
    NotAccepted, FindDungeonPortal, ClearDungeon, SpendSkillPoint,
    CollectPrism, FindBossPortal, DefeatBoss, Completed
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ABasePlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ABasePlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacter, BlueprintReadOnly)
	ECharacterType SelectedCharacter = ECharacterType::Paladin;

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

	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 SkillPoints = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    EQuestStage QuestStage = EQuestStage::NotAccepted;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
    int32 ClearedDungeonCount = 0;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void AcceptMainQuest();
    void NotifyDungeonEntered();
    void NotifyDungeonCleared();
    void NotifySkillPointSpent();
    void NotifyPrismCollected();
    void NotifyBossPortalEntered();
    void NotifyBossDefeated();

    UFUNCTION(BlueprintPure, Category = "Quest")
    FText GetQuestObjectiveText() const;

	// Permanent shop upgrades survive pawn replacement and seamless map travel.
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bHasShopStatBonuses = false;

	UPROPERTY(Replicated, BlueprintReadWrite)
	float SavedMaxHP = 0.f;

	UPROPERTY(Replicated, BlueprintReadWrite)
	float SavedAttackPower = 0.f;

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
