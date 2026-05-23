// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonManager.h"
#include "DungeonPrism.h"
#include "Altar.h"
#include "Monster.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ADungeonManager::ADungeonManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AliveMonsterCount = 0;

	MonstersPerAltar = 5;
}

// Called when the game starts or when spawned
void ADungeonManager::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnMonsters();
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

	// 몬스터 전멸
	if (AliveMonsterCount <= 0)
	{
		if (DungeonPrism)
		{
			UE_LOG(LogTemp, Warning, TEXT("Activate Prism"));

			DungeonPrism->ActivatePrism();
		}
	}
}

void ADungeonManager::SpawnMonsters()
{
	TArray<AActor*> FoundAltars;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AAltar::StaticClass(),
		FoundAltars
	);

	for (AActor* Actor : FoundAltars)
	{
		AAltar* Altar = Cast<AAltar>(Actor);

		if (!Altar) continue;

		// 제단당 5마리 생성
		for (int32 i = 0; i < MonstersPerAltar; i++)
		{
			FVector SpawnLocation =
				Altar->GetActorLocation() +
				FVector(
					FMath::RandRange(-300.f, 300.f),
					FMath::RandRange(-300.f, 300.f),
					50.f
				);

			FRotator SpawnRotation = FRotator::ZeroRotator;

			AMonster* SpawnedMonster =
				GetWorld()->SpawnActor<AMonster>(
					MonsterClass,
					SpawnLocation,
					SpawnRotation
				);

			if (SpawnedMonster)
			{
				// 살아있는 몬스터 등록
				RegisterMonster();

				// 제단 연결
				SpawnedMonster->OwnerAltar = Altar;

				// 제단에도 몬스터 등록
				Altar->RegisterMonster(SpawnedMonster);
			}
		}
	}
}
