// Copyright Epic Games, Inc. All Rights Reserved.

#include "The_Nightfall_SiegeGameMode.h"
#include "TheNightfallSiegeInstance.h"
#include "CharacterType.h"
#include "BaseLobbyGameState.h"
#include "BasePlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Lantern.h"
#include "BaseCharacter.h"

AThe_Nightfall_SiegeGameMode::AThe_Nightfall_SiegeGameMode()
{
	// stub
}

void AThe_Nightfall_SiegeGameMode::BeginPlay()
{
    Super::BeginPlay();

    // TopDown 맵에서만 실행
    if (!GetWorld()->GetMapName().Contains(TEXT("Lvl_TopDown")))
    {
        return;
    }

    bool bSomeoneHasLantern = false;

    TArray<AActor*> Players;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ABaseCharacter::StaticClass(),
        Players);

    for (AActor* Actor : Players)
    {
        ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);

        if (!Player)
        {
            continue;
        }

        if (Player->bHasLantern)
        {
            bSomeoneHasLantern = true;
            break;
        }
    }

    if (!bSomeoneHasLantern)
    {
        return;
    }

    TArray<AActor*> Lanterns;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ALantern::StaticClass(),
        Lanterns);

    for (AActor* Actor : Lanterns)
    {
        Actor->Destroy();
    }

    UE_LOG(LogTemp, Warning, TEXT("World Lantern Removed"));
}

UClass* AThe_Nightfall_SiegeGameMode::GetDefaultPawnClassForController_Implementation(
    AController* InController)
{
    if (!InController)
    {
        return PaladinClass;
    }

    ABasePlayerState* PS =
        InController->GetPlayerState<ABasePlayerState>();

    if (!PS)
    {
        return PaladinClass;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Selected Character : %d"),
        (int32)PS->SelectedCharacter);

    switch (PS->SelectedCharacter)
    {
    case ECharacterType::Paladin:
        return PaladinClass;

    case ECharacterType::Warrior:
        return WarriorClass;

    case ECharacterType::Archer:
        return ArcherClass;

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

