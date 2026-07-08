// Copyright Epic Games, Inc. All Rights Reserved.

#include "The_Nightfall_SiegeGameMode.h"
#include "TheNightfallSiegeInstance.h"
#include "CharacterType.h"
#include "BaseLobbyGameState.h"
#include "BasePlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
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
    if (!GetWorld()->GetMapName().Contains(TEXT("Village_Forest")))
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

AActor* AThe_Nightfall_SiegeGameMode::ChoosePlayerStart_Implementation(
    AController* Player)
{
    if (AActor* Start = Super::ChoosePlayerStart_Implementation(Player))
    {
        return Start;
    }

    // If every PlayerStart is occupied, still return a valid start. The pawn
    // spawn path below offsets additional players so they do not overlap.
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("All PlayerStarts occupied; using %s as fallback"),
            *GetNameSafe(*It));
        return *It;
    }

    UE_LOG(LogTemp, Error, TEXT("No PlayerStart exists in this map"));
    return nullptr;
}

APawn* AThe_Nightfall_SiegeGameMode::SpawnDefaultPawnAtTransform_Implementation(
    AController* NewPlayer,
    const FTransform& SpawnTransform)
{
    UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);

    if (!PawnClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Cannot restart player: default pawn class is null"));
        return nullptr;
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = GetInstigator();
    SpawnInfo.ObjectFlags |= RF_Transient;
    SpawnInfo.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    int32 ExistingPlayerCount = 0;
    for (TActorIterator<ABaseCharacter> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->GetController())
        {
            ++ExistingPlayerCount;
        }
    }

    FTransform SafeSpawnTransform = SpawnTransform;
    SafeSpawnTransform.AddToTranslation(
        FVector(0.f, ExistingPlayerCount * 250.f, 0.f));

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(
        PawnClass,
        SafeSpawnTransform,
        SpawnInfo);

    if (!SpawnedPawn)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Failed to spawn pawn %s for controller %s after travel"),
            *GetNameSafe(PawnClass),
            *GetNameSafe(NewPlayer));
    }

    return SpawnedPawn;
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

