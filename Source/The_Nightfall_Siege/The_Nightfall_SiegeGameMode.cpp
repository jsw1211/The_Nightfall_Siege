// Copyright Epic Games, Inc. All Rights Reserved.

#include "The_Nightfall_SiegeGameMode.h"
#include "TheNightfallSiegeInstance.h"
#include "CharacterType.h"

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
