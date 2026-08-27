// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterType.h"
#include "SkillUpgradeData.h"
#include "TheNightfallSiegeInstance.generated.h"

/**
 * 
 */

UCLASS()
class THE_NIGHTFALL_SIEGE_API UTheNightfallSiegeInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    UTheNightfallSiegeInstance();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECharacterType SelectedCharacter = ECharacterType::Archer;

	UPROPERTY(BlueprintReadWrite)
	TArray<FName> RemainingDungeons;

	// Each village portal consumes one of these locations for the current raid.
	// A location is never selected twice until StartRaid resets the raid.
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> RemainingDungeonPortalLocations;

	// All valid village portal positions.  Unlike the remaining list, this is
	// retained so the boss portal can use any of the six dungeon positions.
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> DungeonPortalLocations;

	UPROPERTY(BlueprintReadWrite)
	FName CurrentDungeon;

	UPROPERTY(BlueprintReadWrite)
	int32 ClearedDungeonCount;

	void StartRaid();

	/**
	 * Keeps the server-authoritative health captured immediately before
	 * ServerTravel. This is independent of PlayerState replacement/copy order.
	 */
	void SaveTravelHealth(int32 PlayerId, float CurrentHP);

	/** Returns and removes the pending health for the newly spawned pawn. */
	bool ConsumeTravelHealth(int32 PlayerId, float& OutCurrentHP);

	FName SelectNextDungeon();

	bool SelectNextDungeonPortalLocation(FVector& OutLocation);

	bool SelectBossPortalLocation(FVector& OutLocation) const;

	bool ClearCurrentDungeon();

	void BeginRetry(bool bWasBossEncounter);
	bool ConsumeDungeonRetry();

	virtual void Init() override;
	virtual void Shutdown() override;

	/** Opens the WBP_IP-style address dialog from the main-menu Join Server button. */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ShowIPJoinDialog();

	UPROPERTY(BlueprintReadWrite)
	bool bHasLantern = false;

	UPROPERTY(BlueprintReadWrite)
	bool bLanternEquipped = false;

	UPROPERTY(BlueprintReadWrite)
	bool bHasPrism = false;

	UPROPERTY(BlueprintReadWrite)
	bool bPrismEquipped = false;

	UPROPERTY(BlueprintReadWrite)
	int32 SkillPoints = 0;

	UPROPERTY(BlueprintReadWrite)
	TMap<ESkillType, int32> SkillLevels;

	UPROPERTY(BlueprintReadWrite)
	bool bBossPortalSpawned = false;

	UPROPERTY(BlueprintReadOnly)
	bool bRetryingCurrentDungeon = false;

	UPROPERTY(BlueprintReadOnly)
	bool bRetryingBoss = false;

	UPROPERTY(BlueprintReadWrite)
	bool bIsHost = false;

	UPROPERTY()
	bool bWorldLanternDestroyed = false;

private:
	TMap<int32, float> PendingTravelHealthByPlayerId;

	void HandlePostLoadMap(UWorld* LoadedWorld);
	void BindMainMenuJoinButton();
	void HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	UFUNCTION()
	void OnMainMenuJoinClicked();

	UPROPERTY(Transient)
	TObjectPtr<class UIPJoinWidget> IPJoinWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Network|UI")
	TSubclassOf<class UIPJoinWidget> IPJoinWidgetClass;
};
