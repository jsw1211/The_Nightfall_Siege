// Fill out your copyright notice in the Description page of Project Settings.


#include "ForestManager.h"
#include "DungeonPortal.h"
#include "TheNightfallSiegeInstance.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AForestManager::AForestManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Keep the runtime-spawned portal Blueprint as a hard reference.  The
	// fallback in SpawnDungeonPortal uses a string path, which the cooker cannot
	// discover reliably when the placed ForestManager Blueprint has no class set.
	static ConstructorHelpers::FClassFinder<ADungeonPortal> DungeonPortalClass(
		TEXT("/Game/BP/BP_DungeonPortal"));
	if (DungeonPortalClass.Succeeded())
	{
		PortalClass = DungeonPortalClass.Class;
	}
}

// Called when the game starts or when spawned
void AForestManager::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnDungeonPortal();
}

// Called every frame
void AForestManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AForestManager::SpawnDungeonPortal()
{
	if (!HasAuthority())
	{
		return;
	}

	UTheNightfallSiegeInstance* GI =
		Cast<UTheNightfallSiegeInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	const bool bRetryingDungeon = GI->ConsumeDungeonRetry();
	if (!bRetryingDungeon && GI->SelectNextDungeon().IsNone())
	{
		return;
	}

	FVector SpawnLocation;
	if (!GI->SelectNextDungeonPortalLocation(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("No unused dungeon portal locations remain."));
		return;
	}

	// The placed BP_ForestManager may have been saved before PortalClass was
	// assigned.  Fall back to the visible dungeon portal Blueprint so deleting
	// a previously placed portal never prevents the runtime portal from spawning.
	TSubclassOf<ADungeonPortal> PortalClassToSpawn = PortalClass;
	if (!PortalClassToSpawn)
	{
		PortalClassToSpawn = LoadClass<ADungeonPortal>(
			nullptr,
			TEXT("/Game/BP/BP_DungeonPortal.BP_DungeonPortal_C"));
	}

	if (!PortalClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load BP_DungeonPortal for runtime spawning."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADungeonPortal* SpawnedPortal = GetWorld()->SpawnActor<ADungeonPortal>(
		PortalClassToSpawn,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("Dungeon portal spawned: %s at %s for %s"),
		*GetNameSafe(SpawnedPortal), *SpawnLocation.ToString(), *GI->CurrentDungeon.ToString());
}
