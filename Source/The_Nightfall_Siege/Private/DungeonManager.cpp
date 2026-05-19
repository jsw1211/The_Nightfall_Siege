// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonManager.h"
#include "DungeonPrism.h"

// Sets default values
ADungeonManager::ADungeonManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AliveMonsterCount = 10;
}

// Called when the game starts or when spawned
void ADungeonManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADungeonManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADungeonManager::RegisterMonster()
{
	AliveMonsterCount++;

	UE_LOG(LogTemp, Warning, TEXT("Monster Registered: %d"), AliveMonsterCount);
}

void ADungeonManager::OnMonsterDead()
{
	AliveMonsterCount--;

	UE_LOG(LogTemp, Warning, TEXT("Monster Dead Left: %d"), AliveMonsterCount);

	// ¸ó½ºÅÍ Àü¸ê
	if (AliveMonsterCount <= 0)
	{
		if (DungeonPrism)
		{
			UE_LOG(LogTemp, Warning, TEXT("Activate Prism"));

			DungeonPrism->ActivatePrism();
		}
	}
}
