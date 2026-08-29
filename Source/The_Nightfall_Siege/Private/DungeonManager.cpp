	// Fill out your copyright notice in the Description page of Project Settings.


	#include "DungeonManager.h"
	#include "Altar.h"
	#include "Monster.h"
	#include "BaseCharacter.h"
	#include "BasePlayerState.h"
	#include "DungeonPrism.h"
	#include "TheNightfallSiegeInstance.h"
	#include "Components/CapsuleComponent.h"
	#include "GameFramework/CharacterMovementComponent.h"
	#include "Kismet/GameplayStatics.h"
	#include "NavigationSystem.h"
	#include "DrawDebugHelpers.h"

	namespace
	{
		constexpr float MinAltarSpawnDistance = 300.f;
		constexpr float MaxAltarSpawnDistance = 800.f;
		constexpr float SpawnWallClearance = 10.f;
		constexpr float SpawnGroundClearance = 5.f;
		constexpr float MaxGroundSnapDistance = 100.f;
		constexpr float NavProjectionHorizontalTolerance = 75.f;
		constexpr float PostSpawnNavHorizontalTolerance = 25.f;
		constexpr float GroundSupportEdgeInset = 5.f;
		constexpr int32 MaxSpawnAttemptsPerMonster = 40;
		constexpr int32 GroundSupportSampleCount = 8;

		bool HasCapsuleGroundSupport(
			UWorld* World,
			const AAltar* Altar,
			const AActor* ActorToIgnore,
			const FVector& CenterGroundLocation,
			float CapsuleRadius,
			float WalkableFloorZ,
			float MaxStepHeight)
		{
			FCollisionQueryParams GroundQuery(
				SCENE_QUERY_STAT(MonsterSpawnFootprintGround),
				false);
			GroundQuery.AddIgnoredActor(Altar);
			if (ActorToIgnore)
			{
				GroundQuery.AddIgnoredActor(ActorToIgnore);
			}

			const float SupportRadius = FMath::Max(
				CapsuleRadius - GroundSupportEdgeInset,
				CapsuleRadius * 0.5f);

			for (int32 SampleIndex = 0;
				SampleIndex < GroundSupportSampleCount;
				++SampleIndex)
			{
				const float Angle = 2.f * UE_PI *
					static_cast<float>(SampleIndex) /
					static_cast<float>(GroundSupportSampleCount);
				const FVector SampleOffset(
					FMath::Cos(Angle) * SupportRadius,
					FMath::Sin(Angle) * SupportRadius,
					0.f);
				const FVector SampleLocation = CenterGroundLocation + SampleOffset;
				FHitResult SupportHit;
				if (!World->LineTraceSingleByChannel(
						SupportHit,
						SampleLocation + FVector(0.f, 0.f, MaxGroundSnapDistance * 0.5f),
						SampleLocation - FVector(0.f, 0.f, MaxGroundSnapDistance),
						ECC_Visibility,
						GroundQuery) ||
					SupportHit.ImpactNormal.Z < WalkableFloorZ ||
					FMath::Abs(
						SupportHit.ImpactPoint.Z - CenterGroundLocation.Z) >
						MaxStepHeight + SpawnGroundClearance)
				{
					return false;
				}
			}

			return true;
		}

		bool IsSpawnedMonsterLocationSafe(
			UWorld* World,
			const AAltar* Altar,
			const AMonster* Monster)
		{
			if (!World || !Altar || !Monster)
			{
				return false;
			}

			const UCapsuleComponent* Capsule = Monster->GetCapsuleComponent();
			UNavigationSystemV1* NavigationSystem =
				FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Capsule || !NavigationSystem)
			{
				return false;
			}

			const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
			const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			const float ExclusionRadius = Altar->MonsterExclusionCapsule
				? Altar->MonsterExclusionCapsule->GetScaledCapsuleRadius()
				: 0.f;
			const float MinimumSafeDistance = FMath::Max(
				MinAltarSpawnDistance,
				ExclusionRadius + CapsuleRadius + SpawnWallClearance);
			const FVector MonsterLocation = Monster->GetActorLocation();
			const float DistanceFromAltar = FVector::Dist2D(
				Altar->GetActorLocation(),
				MonsterLocation);
			if (DistanceFromAltar < MinimumSafeDistance ||
				DistanceFromAltar > MaxAltarSpawnDistance)
			{
				return false;
			}

			FNavAgentProperties AgentProperties = Monster->GetNavAgentPropertiesRef();
			AgentProperties.AgentRadius = FMath::Max(
				AgentProperties.AgentRadius,
				CapsuleRadius);
			AgentProperties.AgentHeight = FMath::Max(
				AgentProperties.AgentHeight,
				CapsuleHalfHeight * 2.f);
			ANavigationData* NavData = NavigationSystem->GetNavDataForProps(
				AgentProperties,
				MonsterLocation);
			FNavLocation ProjectedLocation;
			if (!NavData ||
				!NavigationSystem->ProjectPointToNavigation(
					MonsterLocation,
					ProjectedLocation,
					FVector(
						PostSpawnNavHorizontalTolerance,
						PostSpawnNavHorizontalTolerance,
						CapsuleHalfHeight + MaxGroundSnapDistance),
					NavData) ||
				FVector::Dist2D(MonsterLocation, ProjectedLocation.Location) >
					PostSpawnNavHorizontalTolerance)
			{
				return false;
			}

			const UCharacterMovementComponent* Movement =
				Monster->GetCharacterMovement();
			const float WalkableFloorZ = Movement
				? Movement->GetWalkableFloorZ()
				: 0.7f;
			const float MaxStepHeight = Movement
				? Movement->MaxStepHeight
				: 45.f;
			FCollisionQueryParams GroundQuery(
				SCENE_QUERY_STAT(MonsterPostSpawnGround),
				false);
			GroundQuery.AddIgnoredActor(Altar);
			GroundQuery.AddIgnoredActor(Monster);
			FHitResult GroundHit;
			if (!World->LineTraceSingleByChannel(
					GroundHit,
					MonsterLocation,
					MonsterLocation - FVector(
						0.f,
						0.f,
						CapsuleHalfHeight + MaxGroundSnapDistance),
					ECC_Visibility,
					GroundQuery) ||
				GroundHit.ImpactNormal.Z < WalkableFloorZ)
			{
				return false;
			}

			const float GroundSeparation =
				MonsterLocation.Z - CapsuleHalfHeight - GroundHit.ImpactPoint.Z;
			if (GroundSeparation < -SpawnGroundClearance ||
				GroundSeparation > MaxStepHeight + SpawnGroundClearance ||
				!HasCapsuleGroundSupport(
					World,
					Altar,
					Monster,
					GroundHit.ImpactPoint,
					CapsuleRadius,
					WalkableFloorZ,
					MaxStepHeight))
			{
				return false;
			}

			FCollisionQueryParams PathQuery(
				SCENE_QUERY_STAT(MonsterPostSpawnPath),
				false);
			PathQuery.AddIgnoredActor(Altar);
			PathQuery.AddIgnoredActor(Monster);
			const FVector PathStart = Altar->GetActorLocation() +
				FVector(0.f, 0.f, CapsuleHalfHeight + SpawnGroundClearance);
			return !World->LineTraceTestByChannel(
				PathStart,
				MonsterLocation,
				ECC_Visibility,
				PathQuery);
		}

		bool FindSafeMonsterSpawnLocation(
			UWorld* World,
			const AAltar* Altar,
			const AMonster* MonsterDefault,
			FVector& OutSpawnLocation)
		{
			if (!World || !Altar || !MonsterDefault)
			{
				return false;
			}

			const UCapsuleComponent* Capsule = MonsterDefault->GetCapsuleComponent();
			UNavigationSystemV1* NavigationSystem =
				FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!NavigationSystem || !Capsule)
			{
				return false;
			}

			const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
			const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			const float ExclusionRadius = Altar->MonsterExclusionCapsule
				? Altar->MonsterExclusionCapsule->GetScaledCapsuleRadius()
				: 0.f;
			const float MinimumSafeDistance = FMath::Max(
				MinAltarSpawnDistance,
				ExclusionRadius + CapsuleRadius + SpawnWallClearance);

			FNavAgentProperties AgentProperties =
				MonsterDefault->GetNavAgentPropertiesRef();
			AgentProperties.AgentRadius = FMath::Max(
				AgentProperties.AgentRadius,
				CapsuleRadius);
			AgentProperties.AgentHeight = FMath::Max(
				AgentProperties.AgentHeight,
				CapsuleHalfHeight * 2.f);
			ANavigationData* NavData = NavigationSystem->GetNavDataForProps(
				AgentProperties,
				Altar->GetActorLocation());
			if (!NavData)
			{
				return false;
			}

			// Project a point from the annulus itself. Projecting the altar center
			// can select the small, disconnected nav island on top of its platform.
			const float AngleRadians = FMath::FRandRange(0.f, 2.f * UE_PI);
			const float DesiredDistance = FMath::Sqrt(FMath::FRandRange(
				FMath::Square(MinimumSafeDistance),
				FMath::Square(MaxAltarSpawnDistance)));
			const FVector DesiredLocation = Altar->GetActorLocation() + FVector(
				FMath::Cos(AngleRadians) * DesiredDistance,
				FMath::Sin(AngleRadians) * DesiredDistance,
				0.f);
			FNavLocation NavLocation;
			const FVector ProjectionExtent(
				NavProjectionHorizontalTolerance,
				NavProjectionHorizontalTolerance,
				CapsuleHalfHeight + MaxGroundSnapDistance);
			if (!NavigationSystem->ProjectPointToNavigation(
					DesiredLocation,
					NavLocation,
					ProjectionExtent,
					NavData))
			{
				return false;
			}

			const float DistanceFromAltar = FVector::Dist2D(
				Altar->GetActorLocation(),
				NavLocation.Location);
			if (DistanceFromAltar < MinimumSafeDistance ||
				DistanceFromAltar > MaxAltarSpawnDistance)
			{
				return false;
			}

			const UCharacterMovementComponent* Movement =
				MonsterDefault->GetCharacterMovement();

			FHitResult GroundHit;
			FCollisionQueryParams GroundQuery(
				SCENE_QUERY_STAT(MonsterSpawnGround),
				false);
			GroundQuery.AddIgnoredActor(Altar);

			const FVector TraceStart = NavLocation.Location +
				FVector(0.f, 0.f, MaxGroundSnapDistance * 0.5f);
			const FVector TraceEnd = NavLocation.Location -
				FVector(0.f, 0.f, MaxGroundSnapDistance);
			if (!World->LineTraceSingleByChannel(
					GroundHit,
					TraceStart,
					TraceEnd,
					ECC_Visibility,
					GroundQuery))
			{
				return false;
			}

			const float WalkableFloorZ = Movement
				? Movement->GetWalkableFloorZ()
				: 0.7f;
			if (GroundHit.ImpactNormal.Z < WalkableFloorZ ||
				FMath::Abs(GroundHit.ImpactPoint.Z - NavLocation.Location.Z) >
					MaxGroundSnapDistance)
			{
				return false;
			}

			const float MaxStepHeight = Movement
				? Movement->MaxStepHeight
				: 45.f;
			if (!HasCapsuleGroundSupport(
					World,
					Altar,
					nullptr,
					GroundHit.ImpactPoint,
					CapsuleRadius,
					WalkableFloorZ,
					MaxStepHeight))
			{
				return false;
			}

			const FVector CandidateLocation = GroundHit.ImpactPoint +
				FVector(0.f, 0.f, CapsuleHalfHeight + SpawnGroundClearance);
			FCollisionQueryParams PathQuery(
				SCENE_QUERY_STAT(MonsterSpawnPathClearance),
				false);
			PathQuery.AddIgnoredActor(Altar);
			const FVector PathStart = Altar->GetActorLocation() +
				FVector(0.f, 0.f, CapsuleHalfHeight + SpawnGroundClearance);
			if (World->LineTraceTestByChannel(
					PathStart,
					CandidateLocation,
					ECC_Visibility,
					PathQuery))
			{
				return false;
			}

			OutSpawnLocation = CandidateLocation;
			return true;
		}
	}

	// Sets default values
	ADungeonManager::ADungeonManager()
	{
 		// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
		PrimaryActorTick.bCanEverTick = false;

		bReplicates = true;

		AliveMonsterCount = 0;
		TotalMonsterCount = 0;

		MinMonstersPerAltar = 6;
		MaxMonstersPerAltar = 8;
		MonstersPerAltar_DEPRECATED = 5;
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
		if (!MonsterClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Dungeon monster class is not configured"));
			UpdatePlayerMonsterProgress(false);
			return;
		}

		const AMonster* MonsterDefault = MonsterClass->GetDefaultObject<AMonster>();
		TArray<AActor*> FoundAltars;
		int32 MinimumCount = FMath::Max(1, MinMonstersPerAltar);
		int32 MaximumCount = FMath::Max(MinimumCount, MaxMonstersPerAltar);
		int32 DungeonEntryNumber = 0;

		// The dungeon map is selected randomly, but the monster range follows the
		// party's progress through the raid. ClearedDungeonCount is unchanged by a
		// retry, so retrying a dungeon also keeps the same spawn range.
		if (const UTheNightfallSiegeInstance* GameInstance =
			Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
		{
			DungeonEntryNumber = FMath::Clamp(
				GameInstance->ClearedDungeonCount + 1,
				1,
				3);
			switch (DungeonEntryNumber)
			{
			case 1:
				MinimumCount = 3;
				MaximumCount = 4;
				break;
			case 2:
				MinimumCount = 4;
				MaximumCount = 6;
				break;
			default:
				MinimumCount = 6;
				MaximumCount = 8;
				break;
			}
		}

		if (DungeonEntryNumber > 0)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("Dungeon entry %d uses %d-%d monsters per altar"),
				DungeonEntryNumber,
				MinimumCount,
				MaximumCount);
		}

		UGameplayStatics::GetAllActorsOfClass(
			GetWorld(),
			AAltar::StaticClass(),
			FoundAltars
		);

		for (AActor* Actor : FoundAltars)
		{
			AAltar* Altar = Cast<AAltar>(Actor);

			if (!Altar) continue;

			const int32 TargetMonsterCount = FMath::RandRange(
				MinimumCount,
				MaximumCount);

			for (int32 i = 0; i < TargetMonsterCount; ++i)
			{
				AMonster* SpawnedMonster = nullptr;
				for (int32 Attempt = 0;
					Attempt < MaxSpawnAttemptsPerMonster && !SpawnedMonster;
					++Attempt)
				{
					FVector SpawnLocation;
					if (!FindSafeMonsterSpawnLocation(
							GetWorld(),
							Altar,
							MonsterDefault,
							SpawnLocation))
					{
						continue;
					}

					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride =
						ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
					SpawnedMonster = GetWorld()->SpawnActor<AMonster>(
						MonsterClass,
						SpawnLocation,
						FRotator::ZeroRotator,
						SpawnParams);
					if (SpawnedMonster &&
						!IsSpawnedMonsterLocationSafe(
							GetWorld(),
							Altar,
							SpawnedMonster))
					{
						SpawnedMonster->Destroy();
						SpawnedMonster = nullptr;
					}
				}

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
				else
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("No safe spawn location found for altar %s (%d/%d)"),
						*GetNameSafe(Altar),
						i + 1,
						TargetMonsterCount);
				}
			}
		}

		// The total is based on successful spawns, keeping the quest accurate
		// if a spawn location is blocked or a class fails to load.
		UpdatePlayerMonsterProgress(false);
	}

int32 ADungeonManager::DebugClearDungeon(ABaseCharacter* RewardPlayer)
{
	if (!HasAuthority() || !RewardPlayer || AliveMonsterCount <= 0)
	{
		return 0;
	}

	TArray<AActor*> FoundMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonster::StaticClass(), FoundMonsters);

	TArray<AMonster*> LivingMonsters;
	TSubclassOf<ADungeonPrism> PrismClass;
	for (AActor* Actor : FoundMonsters)
	{
		AMonster* Monster = Cast<AMonster>(Actor);
		if (!Monster || Monster->bIsDead)
		{
			continue;
		}

		LivingMonsters.Add(Monster);
		if (!PrismClass && Monster->PrismClass)
		{
			PrismClass = Monster->PrismClass;
		}
	}

	if (LivingMonsters.IsEmpty())
	{
		return 0;
	}

	bDebugClearInProgress = true;
	for (AMonster* Monster : LivingMonsters)
	{
		Monster->TakeMonsterDamage(Monster->CurrentHP);
	}
	bDebugClearInProgress = false;

	if (!PrismClass)
	{
		PrismClass = LoadClass<ADungeonPrism>(
			nullptr,
			TEXT("/Game/BP/BP_DungeonPrism.BP_DungeonPrism_C"));
	}

	if (PrismClass)
	{
		const FVector SpawnLocation = RewardPlayer->GetActorLocation()
			+ RewardPlayer->GetActorForwardVector() * 250.f
			+ FVector(0.f, 0.f, 50.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ADungeonPrism>(
			PrismClass,
			SpawnLocation,
			RewardPlayer->GetActorRotation(),
			SpawnParams);
	}

	return LivingMonsters.Num();
}
