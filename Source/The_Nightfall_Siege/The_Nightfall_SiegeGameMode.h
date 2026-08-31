// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "The_Nightfall_SiegeGameMode.generated.h"

/**
 *  Simple Game Mode for a top-down perspective game
 *  Sets the default gameplay framework classes
 *  Check the Blueprint derived class for the set values
 */
UCLASS(abstract)
class AThe_Nightfall_SiegeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AThe_Nightfall_SiegeGameMode();

	virtual void BeginPlay() override;

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(
		AController* NewPlayer,
		const FTransform& SpawnTransform) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TSubclassOf<APawn> PaladinClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TSubclassOf<APawn> ArcherClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TSubclassOf<APawn> WarriorClass;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	void HandlePlayerDeath(class ABaseCharacter* DeadCharacter);
	void RequestPartyRetry(class ABaseController* RequestingController);
	void HandleBossDefeated();

	/** Returns false after another party member has already started title travel. */
	bool TryBeginReturnToTitle();
	void ResetGameStateForNewRun();

private:
	bool bPartyRetryAvailable = false;
	bool bGameClearAnnounced = false;
	bool bReturningToTitle = false;
};



