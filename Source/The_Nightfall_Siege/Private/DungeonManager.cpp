// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonManager.h"
#include "DungeonPrism.h"

// Sets default values
ADungeonManager::ADungeonManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AliveMonsterCount = 0;
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
}

void ADungeonManager::OnMonsterDead()
{
	AliveMonsterCount--;

	// ¸ó½ºÅÍ Àü¸ê
	if (AliveMonsterCount <= 0)
	{
		if (DungeonPrism)
		{
			DungeonPrism->ActivatePrism();
		}
	}
}
