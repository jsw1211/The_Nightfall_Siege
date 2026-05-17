// Fill out your copyright notice in the Description page of Project Settings.


#include "ForestManager.h"
#include "DungeonPortal.h"
#include "RaidGameInstance.h"

// Sets default values
AForestManager::AForestManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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
	URaidGameInstance* GI =
		Cast<URaidGameInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	FName NextDungeon =
		GI->SelectNextDungeon();

	if (PortalSpawnPoints.Num() <= 0)
	{
		return;
	}

	int32 RandomIndex =
		FMath::RandRange(
			0,
			PortalSpawnPoints.Num() - 1);

	FVector SpawnLocation =
		PortalSpawnPoints[RandomIndex]->GetActorLocation();

	GetWorld()->SpawnActor<ADungeonPortal>(
		PortalClass,
		SpawnLocation,
		FRotator::ZeroRotator);
}
