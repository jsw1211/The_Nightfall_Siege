	// Fill out your copyright notice in the Description page of Project Settings.


	#include "DungeonManager.h"
	#include "Altar.h"
	#include "Monster.h"
	#include "BaseCharacter.h"
	#include "BasePlayerState.h"
	#include "Kismet/GameplayStatics.h"
	#include "DrawDebugHelpers.h"

	// Sets default values
	ADungeonManager::ADungeonManager()
	{
 		// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
		PrimaryActorTick.bCanEverTick = false;

		bReplicates = true;

		AliveMonsterCount = 0;
		TotalMonsterCount = 0;

		MonstersPerAltar = 5;
	}

	// Called when the game starts or when spawned
	void ADungeonManager::BeginPlay()
	{
		Super::BeginPlay();
	
		if (!HasAuthority())
		{
			return;
		}

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

	bool ADungeonManager::OnMonsterDead()
	{
		AliveMonsterCount = FMath::Max(0, AliveMonsterCount - 1);
		UpdatePlayerMonsterProgress(true);

		UE_LOG(LogTemp, Warning, TEXT("Monster Dead Left: %d"), AliveMonsterCount);

		return AliveMonsterCount <= 0;
	}

void ADungeonManager::UpdatePlayerMonsterProgress(bool bMonsterWasKilled)
{
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

	// Monster count is one party-wide value.  Updating every PlayerState here
	// would increment the shared counter once per connected player.
	for (AActor* Actor : Players)
	{
		if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
		{
			if (ABasePlayerState* PlayerState = Player->GetPlayerState<ABasePlayerState>())
			{
					if (bMonsterWasKilled)
					{
						PlayerState->NotifyDungeonMonsterKilled();
					}
					else
				{
					PlayerState->SetDungeonMonsterTotal(TotalMonsterCount);
				}
				return;
			}
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
				float Angle = FMath::FRandRange(0.f, 360.f);

				float Distance = FMath::FRandRange(300.f, 800.f);

				FVector Offset;

				Offset.X = FMath::Cos(FMath::DegreesToRadians(Angle)) * Distance;
				Offset.Y = FMath::Sin(FMath::DegreesToRadians(Angle)) * Distance;
				Offset.Z = 50.f;

				FVector SpawnLocation =
					Altar->GetActorLocation() + Offset;

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
					++TotalMonsterCount;

					// 제단 연결
					SpawnedMonster->OwnerAltar = Altar;

					// 제단에도 몬스터 등록
					Altar->RegisterMonster(SpawnedMonster);
				}
			}
		}

		// The total is based on successful spawns, keeping the quest accurate
		// if a spawn location is blocked or a class fails to load.
		UpdatePlayerMonsterProgress(false);
	}
