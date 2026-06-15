// Copyright Epic Games, Inc. All Rights Reserved.

#include "The_Nightfall_SiegeGameMode.h"
#include "TheNightfallSiegeInstance.h"
#include "CharacterType.h"
#include "BaseLobbyGameState.h"

AThe_Nightfall_SiegeGameMode::AThe_Nightfall_SiegeGameMode()
{
	// stub
}

UClass* AThe_Nightfall_SiegeGameMode::GetDefaultPawnClassForController_Implementation(
	AController* InController)
{
	UTheNightfallSiegeInstance* GI =
		Cast<UTheNightfallSiegeInstance>(GetGameInstance());

	if (!GI)
	{
		return PaladinClass;
	}

	switch (GI->SelectedCharacter)
	{
	case ECharacterType::Paladin:
		return PaladinClass;

	case ECharacterType::Archer:
		return ArcherClass;

	case ECharacterType::Warrior:
		return WarriorClass;

	default:
		return PaladinClass;
	}
}

void AThe_Nightfall_SiegeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("Player Joined"));

	AGameStateBase* GS = GetGameState<AGameStateBase>();

	if (GS)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Players : %d"),
			GS->PlayerArray.Num());
	}
}

