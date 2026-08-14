// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBoss.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "DungeonPrism.h"
#include "BasePlayerState.h"
#include "DragonBreathProjectile.h"
#include "DangerZone.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "The_Nightfall_SiegeGameMode.h"

// Sets default values
ADragonBoss::ADragonBoss()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Keep these gameplay effects wired even when the BP defaults have not
	// explicitly overridden the corresponding C++ properties.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BlackoutChargingAsset(
		TEXT("/Game/Effects/6_Dragon/Blackout_Debuff/NS_Dragon_Blackout_Charging.NS_Dragon_Blackout_Charging"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BlackoutReleaseAsset(
		TEXT("/Game/Effects/6_Dragon/Blackout_Debuff/NS_Dragon_Blackout_Release.NS_Dragon_Blackout_Release"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PhaseTwoAsset(
		TEXT("/Game/Effects/6_Dragon/Second_Phase/NS_Dragon_FX.NS_Dragon_FX"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PhaseTwoMaterialAsset(
		TEXT("/Game/Effects/6_Dragon/Second_Phase/M_Dragon_FX.M_Dragon_FX"));
	static ConstructorHelpers::FClassFinder<ADragonBreathProjectile> BreathProjectileAsset(
		TEXT("/Game/BP_Monster/Dragon/BP_DragonBreathProjectile"));

	if (BlackoutChargingAsset.Succeeded())
	{
		BlackoutChargingFX = BlackoutChargingAsset.Object;
	}

	if (BlackoutReleaseAsset.Succeeded())
	{
		BlackoutReleaseFX = BlackoutReleaseAsset.Object;
	}

	if (PhaseTwoAsset.Succeeded())
	{
		PhaseTwoFX = PhaseTwoAsset.Object;
	}

	if (PhaseTwoMaterialAsset.Succeeded())
	{
		PhaseTwoOverlayMaterial = PhaseTwoMaterialAsset.Object;
	}

	if (BreathProjectileAsset.Succeeded())
	{
		BreathProjectileClass = BreathProjectileAsset.Class;
	}

}

// Called when the game starts or when spawned
void ADragonBoss::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	ArenaCenter = FVector(0.f, 0.f, 0.f);

	bShielded = true;
	bCanTakeDamage = false;

	CurrentState = EDragonState::Idle;

	UpdatePlayerList();

	ChooseRandomTarget();

	bFirstBreathDone = false;

	StartAttackTelegraph(EDragonAttackType::Breath);

}

// Called every frame
void ADragonBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	if (bCenterTracking && TargetPlayer)
	{
		FVector Direction = TargetPlayer->GetActorLocation() - GetActorLocation();

		Direction.Z = 0.f;

		FRotator TargetRotation = Direction.Rotation();

		SetActorRotation(TargetRotation);

		if (CurrentBreathZone)
		{
			// The warning is a ground rectangle from the dragon itself to the
			// selected target, rather than a socket-relative decal that can drift
			// sideways as the head animation moves.
			FVector LineStart = GetActorLocation();
			FVector LineEnd = TargetPlayer->GetActorLocation();
			LineEnd.Z = LineStart.Z;
			FVector LineDirection = LineEnd - LineStart;

			const float TargetDistance = LineDirection.Size2D();
			const float TelegraphLength = FMath::Clamp(
				TargetDistance + 500.f, 1000.f, BreathTelegraphRange);
			LineDirection = LineDirection.GetSafeNormal2D();
			FVector SpawnLocation = LineStart + LineDirection * (TelegraphLength * 0.5f);
			FRotator LineRotation = LineDirection.Rotation();
			LineRotation.Pitch = -90.f;

			CurrentBreathZone->SetActorLocation(SpawnLocation);

			CurrentBreathZone->SetActorRotation(LineRotation);
			CurrentBreathZone->LineLength = TelegraphLength;
			CurrentBreathZone->OnRep_LineLength();
			CurrentBreathZone->ForceNetUpdate();
		}
	}

	if (bCenterMechanicActive)
	{
		if (!bCenterBreathStarted)
		{
			const FVector PreviousLocation = GetActorLocation();
			const FVector NewLocation = FMath::VInterpConstantTo(
				PreviousLocation, ArenaCenter, DeltaTime, 1200.f);

			if (DeltaTime > UE_SMALL_NUMBER)
			{
				GetCharacterMovement()->Velocity =
					(NewLocation - PreviousLocation) / DeltaTime;
			}

			SetActorLocation(NewLocation);

			FVector CenterDirection = ArenaCenter - NewLocation;
			CenterDirection.Z = 0.f;
			if (!CenterDirection.IsNearlyZero())
			{
				SetActorRotation(CenterDirection.Rotation());
			}
		}

		float Distance =
			FVector::Dist(
				GetActorLocation(),
				ArenaCenter);

		if (Distance <= 200.f && !bCenterBreathStarted)
		{
			bCenterBreathStarted = true;

			UE_LOG(LogTemp, Warning,
				TEXT("Center Arrived"));

			bCenterTracking = true;

			StartAttackTelegraph(
				EDragonAttackType::Breath);

			return;
		}

		return;
	}

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	if (!bFirstBreathDone)
	{
		return;
	}

	if (CurrentState == EDragonState::Leap)
	{
		return;
	}

	if (CurrentState == EDragonState::Flying)
	{
		FlyToTarget();
		return;
	}

	if (CurrentState == EDragonState::Landing)
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
	}

	if (bCenterMechanicActive)
	{
		return;
	}

	if (TargetPlayer && !bIsAttacking && !bStunned && !bIsTelegraphing)
	{
		float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		const float EffectiveEngageRange = FMath::Max(AttackEngageRange, AttackStartRange + 150.f);

		if (bIsFlying)
		{
			FlyToTarget();
		}
		else
		{
			if (Distance > FlyStartRange)
			{
				FlyToTarget();
			}
			else if (Distance > EffectiveEngageRange)
			{
				WalkToTarget();
			}
			else if (Distance <= EffectiveEngageRange)
			{
				if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
				{
					StartAttackCycle();
				}
			}
		}
	}
}

// Called to bind functionality to input
void ADragonBoss::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void ADragonBoss::StartAttackCycle()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(
		AttackTimerHandle);

	if (bStunned)
	{
		return;
	}

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("StartAttackCycle"));

	UpdatePlayerList();

	if (AlivePlayers.Num() == 0)
	{
		return;
	}
	//ChooseRandomTarget();

	float RandomDelay = FMath::RandRange(1.f, 2.f); // 디버그용으로 1~2초

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ADragonBoss::ExecutePattern,
		RandomDelay,
		false
	);
}

EDragonAttackType ADragonBoss::ChooseRandomAttack()
{
	int32 Rand = FMath::RandRange(1, 100);

	if (Rand <= 50)
	{
		return EDragonAttackType::Bite;
	}

	if (Rand <= 90)
	{
		return EDragonAttackType::CloseBreath;
	}

	return EDragonAttackType::Debuff;
}

void ADragonBoss::ExecuteRandomAttack()
{
	if (!HasAuthority())
	{
		return;
	}

	EDragonAttackType AttackType =
		ChooseRandomAttack();

	StartAttackTelegraph(
		AttackType);
}

void ADragonBoss::BiteAttack()
{
	///////////////////////////////////// Debug Log
	BiteCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] Bite (%d / Total:%d)"),
		BiteCount,
		TotalPatternCount);
	/////////////////////////////////////
	SetActorRotation(TelegraphRotation);

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Bite"));

	MulticastPlayAttack(EDragonAttackType::Bite);

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			OnAttackFinished();
		},
		2.0f,
		false
	);
}

void ADragonBoss::CloseBreathAttack()
{
	////////////////////////////////
	CloseBreathCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] CloseBreath (%d / Total:%d)"),
		CloseBreathCount,
		TotalPatternCount);
	/////////////////////////////////
	SetActorRotation(TelegraphRotation);

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Close Breath"));

	MulticastPlayAttack(EDragonAttackType::CloseBreath);

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			OnAttackFinished();
		},
		3.0f,
		false
	);
}

void ADragonBoss::BreathAttack()
{
	/////////////////////////////////////////
	TargetChangeBreathCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[TargetChange] Breath (%d)"),
		TargetChangeBreathCount);
	//////////////////////////////////////////
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	if (bCenterMechanicActive)
	{
		bMovementLockedForBreath = true;
		GetCharacterMovement()->DisableMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Breath"));

	// AM_dragon_breath_attack's BreathFire notify calls BreathFire exactly
	// when the dragon's mouth opens. Center breath uses this same path.
	MulticastPlayAttack(EDragonAttackType::Breath);

	if (BreathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ADragonBoss::OnBreathMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, BreathMontage);
			return;
		}
	}

	// Only used if the montage is not assigned or an anim instance is missing.
	FTimerHandle AttackEndHandle;
	GetWorldTimerManager().SetTimer(AttackEndHandle, this,
		&ADragonBoss::OnAttackFinished, 4.0f, false);
}

void ADragonBoss::OnBreathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (HasAuthority() && Montage == BreathMontage)
	{
		OnAttackFinished();
	}
}

void ADragonBoss::DebuffAttack()
{
	/////////////////////////////////
	DebuffCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] Debuff (%d / Total:%d)"),
		DebuffCount,
		TotalPatternCount);
	//////////////////////////////////
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Used Debuff"));

	ResetPrismCleanseParticipants();

	// The telegraph has completed: release the blackout before applying it.
	MulticastSpawnBlackoutReleaseFX(GetActorLocation());

	TArray<AActor*> Prisms;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ADungeonPrism::StaticClass(),
		Prisms);

	for (AActor* Actor : Prisms)
	{
		ADungeonPrism* Prism =
			Cast<ADungeonPrism>(Actor);

		if (Prism)
		{
			Prism->ActivatePrism();
		}
	}

	UpdatePlayerList();

	for (ABaseCharacter* Player : AlivePlayers)
	{
		if (Player)
		{
			Player->bDarknessDebuff = true;

			UE_LOG(LogTemp, Warning,
				TEXT("%s Darkness Debuff"),
				*Player->GetName());
		}
	}

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			OnAttackFinished();
		},
		2.5f,
		false
	);
}

void ADragonBoss::ResetPrismCleanseParticipants()
{
	PrismCleanseParticipants.Empty();
}

bool ADragonBoss::ArePrismHoldersGathered(const TArray<AActor*>& PlayerActors) const
{
	const float MaxDistanceSquared = FMath::Square(PrismCleanseGatherRadius);
	for (AActor* FirstActor : PlayerActors)
	{
		ABaseCharacter* First = Cast<ABaseCharacter>(FirstActor);
		if (!First || First->IsDead() || !First->bHasPrism) continue;

		for (AActor* SecondActor : PlayerActors)
		{
			ABaseCharacter* Second = Cast<ABaseCharacter>(SecondActor);
			if (!Second || Second == First || Second->IsDead() || !Second->bHasPrism) continue;

			if (FVector::DistSquared(First->GetActorLocation(), Second->GetActorLocation()) > MaxDistanceSquared)
			{
				return false;
			}
		}
	}

	return true;
}

void ADragonBoss::RegisterPrismCleanseParticipant(ABaseCharacter* Player)
{
	if (!HasAuthority() || !Player || Player->IsDead() || !Player->bHasPrism || !Player->bPrismEquipped || !Player->bDarknessDebuff)
	{
		return;
	}

	PrismCleanseParticipants.Add(Player);

	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), PlayerActors);
	int32 RequiredParticipants = 0;
	for (AActor* Actor : PlayerActors)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(Actor);
		if (Character && !Character->IsDead() && Character->bHasPrism)
		{
			++RequiredParticipants;
			if (!PrismCleanseParticipants.Contains(Character))
			{
				UE_LOG(LogTemp, Warning, TEXT("Prism cleanse waiting: %d/%d"), PrismCleanseParticipants.Num(), RequiredParticipants);
				return;
			}
		}
	}

	if (RequiredParticipants == 0)
	{
		return;
	}

	if (!ArePrismHoldersGathered(PlayerActors))
	{
		PrismCleanseParticipants.Empty();
		UE_LOG(LogTemp, Warning, TEXT("Prism cleanse failed: holders must gather within %.0f units"), PrismCleanseGatherRadius);
		return;
	}

	for (AActor* Actor : PlayerActors)
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(Actor))
		{
			Character->bDarknessDebuff = false;
			Character->ForceNetUpdate();
		}
	}

	PrismCleanseParticipants.Empty();
	UE_LOG(LogTemp, Warning, TEXT("Group prism cleanse completed by %d players"), RequiredParticipants);
}

void ADragonBoss::WalkToTarget()
{
	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController && TargetPlayer)
	{
		const float EffectiveChaseStopRange = FMath::Clamp(ChaseStopRange, 100.f, AttackStartRange * 0.8f);
		AIController->MoveToActor(
			TargetPlayer,
			EffectiveChaseStopRange);
	}
}

void ADragonBoss::FlyToTarget()
{
	if (!bIsFlying)
	{
		if (bIsLeaping)
		{
			return;
		}

		//////////////////////////////////////
		TargetChangeFlyCount++;

		UE_LOG(LogTemp, Warning,
			TEXT("[TargetChange] Fly (%d)"),
			TargetChangeFlyCount);
		//////////////////////////////////////

		bIsLeaping = true;

		CurrentState = EDragonState::Leap;

		MulticastPlayMovementTransition(false);
		// Fallback for missing/failed animation notifies: never remain in Leap.
		GetWorldTimerManager().SetTimer(LeapRecoveryHandle, this, &ADragonBoss::OnLeapFinished, 2.f, false);

		return;
	}

	float Distance =
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation());

	if (Distance <= FlyLandingRange)
	{
		bIsFlying = false;

		CurrentState =
			EDragonState::Landing;

		MulticastPlayMovementTransition(true);
		GetWorldTimerManager().SetTimer(LandingRecoveryHandle, this, &ADragonBoss::OnLandFinished, 2.f, false);

		return;
	}

	FVector TargetLoc =
		TargetPlayer->GetActorLocation();

	TargetLoc.Z += 300.f;

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FVector PreviousLocation = GetActorLocation();

	FVector NewLocation =
		FMath::VInterpConstantTo(
			PreviousLocation,
			TargetLoc,
			DeltaSeconds,
			1200.f);

	// SetActorLocation does not provide a stable CharacterMovement velocity.
	// Keep it updated so animation blueprints never see a zero-speed frame
	// while the dragon is visibly flying.
	if (DeltaSeconds > UE_SMALL_NUMBER)
	{
		GetCharacterMovement()->Velocity =
			(NewLocation - PreviousLocation) / DeltaSeconds;
	}

	SetActorLocation(NewLocation);

	SetActorRotation(
		(TargetLoc - GetActorLocation())
		.Rotation());
}

void ADragonBoss::FlyToCenter()
{
	bIsFlying = true;

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Flying To Center"));

	UE_LOG(LogTemp, Error,
		TEXT("Moving To Center : %s"),
		*ArenaCenter.ToString());
}

void ADragonBoss::TakeBossDamage(float Damage)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bCanTakeDamage)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Dragon Is Invincible"));
		return;
	}

	CurrentHP = FMath::Max(
		0.f,
		CurrentHP - Damage);

	CheckPhaseTwo();

	MulticastShowDamage(Damage);

	ForceNetUpdate();

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon HP : %f"),
		CurrentHP);

	if (CurrentHP <= 0.f)
	{
		Die();
	}
}

void ADragonBoss::OnBreathReflected()
{
	if (bCenterMechanicActive)
	{
		OnCenterMechanicSuccess();
		return;
	}

	if (!bShielded)
	{
		float Damage = MaxHP * 0.1f;

		CurrentHP = FMath::Max(
			0.f,
			CurrentHP - Damage);

		CheckPhaseTwo();

		UE_LOG(LogTemp, Warning,
			TEXT("Reflect Damage : %f"),
			Damage);

		UE_LOG(LogTemp, Warning,
			TEXT("Boss HP : %f"),
			CurrentHP);

		if (CurrentHP <= 0.f)
		{
			Die();
		}

		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Shield Broken"));

	bShielded = false;
	bCanTakeDamage = true;

	bStunned = true;

	CurrentState = EDragonState::Attacking;

	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&ADragonBoss::EndStun,
		5.f,
		false
	);
}

void ADragonBoss::EndStun()
{
	UE_LOG(LogTemp, Error,
		TEXT("EndStun Called"));

	bStunned = false;
	bIsAttacking = false;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Walking
	);

	UE_LOG(LogTemp, Warning,
		TEXT("Stun End"));

	StartAttackCycle();
}

void ADragonBoss::UpdatePlayerList()
{
	AlivePlayers.Empty();

	TArray<AActor*> FoundPlayers;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ABaseCharacter::StaticClass(),
		FoundPlayers
	);

	for (AActor* Actor : FoundPlayers)
	{
		ABaseCharacter* Player =
			Cast<ABaseCharacter>(Actor);

		if (Player && !Player->IsDead())
		{
			AlivePlayers.Add(Player);
		}
	}
}

void ADragonBoss::ChooseRandomTarget()
{
	if (!HasAuthority())
	{
		return;
	}

	UpdatePlayerList();

	if (AlivePlayers.Num() == 0)
	{
		TargetPlayer = nullptr;
		return;
	}

	int32 RandomIndex =
		FMath::RandRange(
			0,
			AlivePlayers.Num() - 1
		);

	TargetPlayer = AlivePlayers[RandomIndex];

	UE_LOG(LogTemp, Warning,
		TEXT("New Target : %s"),
		*TargetPlayer->GetName());

	bIsLeaping = false;
	bIsFlying = false;
	bIsAttacking = false;

	CurrentState = EDragonState::Walking;
}

EDragonPatternType ADragonBoss::ChoosePattern()
{
	int32 Rand = FMath::RandRange(1, 100);

	if (Rand <= 70)
	{
		return EDragonPatternType::NormalAttack;
	}

	if (Rand <= 90)
	{
		return EDragonPatternType::TargetChange;
	}

	return EDragonPatternType::CenterMechanic;
}

void ADragonBoss::ExecutePattern()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();

		if (!TargetPlayer)
		{
			return;
		}
	}

	if (CurrentState == EDragonState::Flying ||
		CurrentState == EDragonState::Leap ||
		CurrentState == EDragonState::Landing)
	{
		return;
	}

	float Distance =
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation());
	const float EffectiveEngageRange = FMath::Max(AttackEngageRange, AttackStartRange + 150.f);

	UE_LOG(LogTemp, Warning,
		TEXT("ExecutePattern Distance = %f"),
		Distance);

	if (Distance > EffectiveEngageRange)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Target Out Of Range : %f"), Distance);

		if (Distance > FlyStartRange)
		{
			FlyToTarget();
		}
		else
		{
			WalkToTarget();
		}

		// Tick keeps the boss chasing and will start a new cycle only after it
		// enters the attack envelope.  Rescheduling here caused boundary spam.
		return;
	}

	if (bStunned)
	{
		return;
	}

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecutePattern"));

	EDragonPatternType Pattern = ChoosePattern();

	switch (Pattern)
	{
	case EDragonPatternType::NormalAttack:
		ExecuteRandomAttack();
		break;

	case EDragonPatternType::TargetChange:
		TargetChangePattern();
		break;

	case EDragonPatternType::CenterMechanic:
		CenterMechanicPattern();
		break;
	}
	///////////////////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("===== Dragon Pattern Statistics ====="));

	UE_LOG(LogTemp, Warning,
		TEXT("Bite             : %d"),
		BiteCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Close Breath     : %d"),
		CloseBreathCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Debuff           : %d"),
		DebuffCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Change    : %d"),
		TargetChangeCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Fly       : %d"),
		TargetChangeFlyCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Breath    : %d"),
		TargetChangeBreathCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic  : %d"),
		CenterMechanicCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Total            : %d"),
		TotalPatternCount);

	///////////////////////////////////////////
}

void ADragonBoss::TargetChangePattern()
{
	/////////////////////////////////
	TargetChangeCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] TargetChange (%d / Total:%d)"),
		TargetChangeCount,
		TotalPatternCount);
	//////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("Target Change Pattern"));

	ChooseRandomTarget();

	UE_LOG(LogTemp, Warning,
		TEXT("TargetChange Distance=%f"),
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation()));

	bIsAttacking = false;

	CurrentState = EDragonState::Walking;

	StartAttackCycle();
}

void ADragonBoss::CenterMechanicPattern()
{
	//////////////////////////////////////////
	CenterMechanicCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] CenterMechanic (%d / Total:%d)"),
		CenterMechanicCount,
		TotalPatternCount);
	//////////////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic"));

	bCenterMechanicActive = true;

	FlyToCenter();

	ChooseRandomTarget();
	if (!TargetPlayer)
	{
		FailCenterMechanic();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Center Target : %s"),
		*TargetPlayer->GetName());

	GetWorldTimerManager().SetTimer(
		CenterFailHandle,
		this,
		&ADragonBoss::FailCenterMechanic,
		8.f,
		false
	);
}

void ADragonBoss::FailCenterMechanic()
{
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	// Once the central Breath montage begins, its end callback owns the
	// transition back into the normal pattern loop.
	if (bMovementLockedForBreath)
	{
		return;
	}

	// The former timeout scheduled an attack but left bCenterMechanicActive
	// true, so Tick returned forever and the boss stopped attacking.
	bCenterMechanicActive = false;
	bCenterBreathStarted = false;
	bCenterTracking = false;
	bIsAttacking = false;
	bIsTelegraphing = false;
	bIsLeaping = false;
	bIsFlying = false;
	CurrentState = EDragonState::Walking;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetWorldTimerManager().ClearTimer(CenterFailHandle);
	ChooseRandomTarget();
	StartAttackCycle();
}

void ADragonBoss::OnCenterMechanicSuccess()
{
	if (!bCenterMechanicActive)
	{
		return;
	}

	float Damage = MaxHP * 0.1f;

	CurrentHP = FMath::Max(
		0.f,
		CurrentHP - Damage);

	CheckPhaseTwo();

	if (CurrentHP <= 0.f)
	{
		Die();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic Success"));

	UE_LOG(LogTemp, Warning,
		TEXT("Damage : %f"),
		Damage);

	UE_LOG(LogTemp, Warning,
		TEXT("Boss HP : %f"),
		CurrentHP);

	const bool bBreathMontageIsPlaying =
		bIsAttacking && CurrentState == EDragonState::Attacking;

	bCenterMechanicActive = false;
	bCenterTracking = false;
	bCenterBreathStarted = false;
	GetWorldTimerManager().ClearTimer(CenterFailHandle);

	// A reflected breath can resolve the mechanic before the animation ends.
	// Let its montage callback start the next pattern in that case.
	if (bBreathMontageIsPlaying)
	{
		return;
	}

	bIsAttacking = false;
	CurrentState = EDragonState::Walking;
	StartAttackCycle();
}

void ADragonBoss::OnAttackFinished()
{
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	UpdatePlayerList();

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
	}

	bIsAttacking = false;
	bIsTelegraphing = false;
	if (bCenterMechanicActive || bMovementLockedForBreath)
	{
		// Keep the central mechanic active until the breath montage ends so the
		// next pattern cannot interrupt its animation or projectile.
		bCenterMechanicActive = false;
		bCenterTracking = false;
		bCenterBreathStarted = false;
		bIsFlying = false;
		bMovementLockedForBreath = false;
		CurrentState = EDragonState::Walking;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetWorldTimerManager().ClearTimer(CenterFailHandle);
	}
	else if (!bIsFlying)
	{
		CurrentState = EDragonState::Walking;
	}

	if (!bFirstBreathDone)
	{
		bFirstBreathDone = true;
	}

	StartAttackCycle();
}

void ADragonBoss::Die()
{

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	CurrentHP = 0.f;

	CurrentState = EDragonState::Dead;

	bIsFlying = false;
	bIsLeaping = false;
	bIsAttacking = false;

	bCenterTracking = false;
	bCenterMechanicActive = false;

	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	MulticastPlayDeath();

	GetWorldTimerManager().ClearTimer(
		AttackTimerHandle
	);

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Dead"));

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
	for (AActor* Actor : Players)
	{
		if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
		{
			if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
			{
				PS->NotifyBossDefeated();
				Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
			}
		}
	}

	if (AThe_Nightfall_SiegeGameMode* GameMode = GetWorld()->GetAuthGameMode<AThe_Nightfall_SiegeGameMode>())
	{
		GameMode->HandleBossDefeated();
	}

	GetWorldTimerManager().ClearTimer(
		TelegraphHandle);

	GetWorldTimerManager().ClearTimer(
		StunTimerHandle);

	GetWorldTimerManager().ClearTimer(
		CenterFailHandle);
}

void ADragonBoss::StartAttackTelegraph(
	EDragonAttackType AttackType)
{
	if (!HasAuthority() || CurrentState == EDragonState::Dead || bStunned)
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
		if (!TargetPlayer)
		{
			bIsTelegraphing = false;
			bIsAttacking = false;
			CurrentState = EDragonState::Idle;
			return;
		}
	}

	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	GetCharacterMovement()
		->StopMovementImmediately();

	bIsTelegraphing = true;

	bIsAttacking = true;

	if (TargetPlayer)
	{
		FVector Direction =
			TargetPlayer->GetActorLocation()
			- GetActorLocation();

		Direction.Z = 0.f;

		TelegraphRotation =
			Direction.Rotation();

		SetActorRotation(
			TelegraphRotation);
	}

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
	{
		FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

		FVector Forward = GetActorForwardVector();

		FVector SpawnLocation = MouthLocation + Forward * 500.f;

		FRotator ZoneRotation = TelegraphRotation;

		ZoneRotation.Pitch = -90.f;

		ADangerZone* Zone = GetWorld()->SpawnActor<ADangerZone>(DangerZoneClass, SpawnLocation, ZoneRotation);

		if (Zone)
		{
			Zone->ZoneType = EDangerZoneType::Circle;
			Zone->OnRep_ZoneType();
		}

		break;
	}

	case EDragonAttackType::CloseBreath:
	{
		FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

		FVector Forward = GetActorForwardVector();

		FVector SpawnLocation = MouthLocation + Forward * 700.f;

		FRotator ZoneRotation = TelegraphRotation;

		ZoneRotation.Pitch = -90.f;

		ADangerZone* Zone = GetWorld()->SpawnActor<ADangerZone>(DangerZoneClass, SpawnLocation, ZoneRotation);

		if (Zone)
		{
			Zone->ZoneType = EDangerZoneType::Cone;
			Zone->OnRep_ZoneType();
		}

		break;
	}

	case EDragonAttackType::Breath:
	{
		// Build the rectangle directly from the dragon to its chosen target.
		// Its centre is the midpoint and its long local axis is the target line.
		FVector LineStart = GetActorLocation();
		FVector LineEnd = TargetPlayer->GetActorLocation();
		LineEnd.Z = LineStart.Z;
		FVector LineDirection = LineEnd - LineStart;
		const float TargetDistance = LineDirection.Size2D();
		const float TelegraphLength = FMath::Clamp(
			TargetDistance + 500.f, 1000.f, BreathTelegraphRange);
		LineDirection = LineDirection.GetSafeNormal2D();
		FVector SpawnLocation = LineStart + LineDirection * (TelegraphLength * 0.5f);

		FRotator Rot = LineDirection.Rotation();
		Rot.Pitch = -90.f;

		ADangerZone* Zone = GetWorld()->SpawnActor<ADangerZone>(DangerZoneClass, SpawnLocation, Rot);

		if (Zone)
		{
			Zone->LineLength = TelegraphLength;
			Zone->ZoneType = EDangerZoneType::Line;
			Zone->OnRep_ZoneType();
			CurrentBreathZone = Zone;
		}

		break;
	}

	case EDragonAttackType::Debuff:
	{
		FVector SpawnLocation = ArenaCenter;

		SpawnLocation.Z += 5.f;

		FRotator Rotation(-90.f, 0.f, 0.f);

		ADangerZone* Zone = GetWorld()->SpawnActor<ADangerZone>(DangerZoneClass, SpawnLocation, Rotation);

		if (Zone)
		{
			Zone->ZoneType = EDangerZoneType::FullMap;
			Zone->OnRep_ZoneType();
		}

		// This effect is only cosmetic; the actual debuff is applied when the
		// charge finishes in DebuffAttack().
		MulticastSpawnBlackoutChargingFX();

		break;
	}
	}

	FTimerDelegate Delegate;

	Delegate.BindLambda(
		[this, AttackType]()
		{
			ExecuteTelegraphedAttack(
				AttackType);
		});

	GetWorldTimerManager().SetTimer(
		TelegraphHandle,
		Delegate,
		3.f,
		false);
}

void ADragonBoss::ExecuteTelegraphedAttack(
	EDragonAttackType AttackType)
{
	bIsTelegraphing = false;

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
		BiteAttack();
		break;

	case EDragonAttackType::CloseBreath:
		CloseBreathAttack();
		break;

	case EDragonAttackType::Breath:
		if (CurrentBreathZone)
		{
			CurrentBreathZone = nullptr;
		}

		BreathAttack();
		break;

	case EDragonAttackType::Debuff:
		DebuffAttack();
		break;
	}
}

void ADragonBoss::OnLeapFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Error,
		TEXT("===== LEAP FINISHED ====="));

	bIsLeaping = false;
	GetWorldTimerManager().ClearTimer(LeapRecoveryHandle);
	bIsFlying = true;

	UE_LOG(LogTemp, Warning, TEXT("bIsFlying = %d"), bIsFlying);

	CurrentState = EDragonState::Flying;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Flying);

	UE_LOG(LogTemp, Error,
		TEXT("Target = %s"),
		TargetPlayer ? *TargetPlayer->GetName() : TEXT("None"));

	if (TargetPlayer)
	{
		FlyToTarget();
	}
}

void ADragonBoss::OnLandFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsFlying = false;
	GetWorldTimerManager().ClearTimer(LandingRecoveryHandle);

	CurrentState = EDragonState::Walking;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Walking);

	UE_LOG(LogTemp, Error,
		TEXT("===== LAND FINISHED ====="));

	StartAttackCycle();
}

void ADragonBoss::BiteHit()
{
	if (!HasAuthority())
	{
		return;
	}

	float Damage = AttackPower * 1.0f * GetCurrentDamageMultiplier();

	FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

	FVector Forward = GetActorForwardVector();

	FVector BiteCenter = MouthLocation + Forward * 500.f;

	MulticastSpawnBiteFX(BiteCenter);

	TArray<AActor*> Players;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

	for (AActor* Actor : Players)
	{
		ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);

		if (!Player)
		{
			continue;
		}

		float Distance = FVector::Dist(Player->GetActorLocation(), BiteCenter);

		if (Distance <= 350.f)
		{
			Player->TakePlayerDamage(Damage);

			UE_LOG(LogTemp, Warning, TEXT("Bite Hit : %s"), *Player->GetName());
		}
	}
}

void ADragonBoss::CloseBreathFire()
{
	if (!HasAuthority())
	{
		return;
	}

	float Damage = AttackPower * 2.0f * GetCurrentDamageMultiplier();

	FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

	FVector Forward = GetActorForwardVector();

	FVector BreathCenter = MouthLocation + Forward * 700.f;

	BreathCenter.Z = GetActorLocation().Z;

	MulticastSpawnCloseBreathFX(BreathCenter);

	TArray<AActor*> Players;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

	for (AActor* Actor : Players)
	{
		ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);

		if (!Player)
		{
			continue;
		}

		float Distance = FVector::Dist(Player->GetActorLocation(), BreathCenter);

		if (Distance <= 350.f)
		{
			Player->TakePlayerDamage(Damage);

			UE_LOG(LogTemp, Warning, TEXT("Close Breath Hit : %s"), *Player->GetName());
		}
	}
}

void ADragonBoss::BreathFire()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!BreathProjectileClass)
	{
		return;
	}

	FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

	FRotator SpawnRotation = GetActorRotation();

	ADragonBreathProjectile* Projectile =
		GetWorld()->SpawnActor<ADragonBreathProjectile>(BreathProjectileClass, MouthLocation, SpawnRotation);

	if (Projectile)
	{
		Projectile->DamageMultiplier = GetCurrentDamageMultiplier();

		Projectile->LaunchInDirection(GetActorForwardVector());
	}
}

void ADragonBoss::MulticastShowDamage_Implementation(float Damage)
{
	ShowDamage(Damage);
}

void ADragonBoss::MulticastPlayAttack_Implementation(EDragonAttackType AttackType)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance)
	{
		return;
	}

	switch (AttackType)
	{
	case EDragonAttackType::Bite:

		if (BiteMontage)
		{
			AnimInstance->Montage_Play(BiteMontage);
		}

		break;

	case EDragonAttackType::CloseBreath:

		if (CloseBreathMontage)
		{
			AnimInstance->Montage_Play(CloseBreathMontage);
		}

		break;

	case EDragonAttackType::Breath:

		if (BreathMontage)
		{
			AnimInstance->Montage_Play(BreathMontage);
		}

		break;

	default:
		break;
	}
}

void ADragonBoss::MulticastPlayMovementTransition_Implementation(bool bLanding)
{
	UAnimMontage* Montage = bLanding ? LandMontage : LeapMontage;

	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

void ADragonBoss::MulticastSpawnBiteFX_Implementation(
	FVector Location)
{
	if (!BiteFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		BiteFX,
		Location,
		FRotator::ZeroRotator,
		FVector(3.f));
}

void ADragonBoss::MulticastSpawnCloseBreathFX_Implementation(
	FVector Location)
{
	if (!CloseBreathFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		CloseBreathFX,
		Location);
}

void ADragonBoss::MulticastSpawnBlackoutChargingFX_Implementation()
{
	if (!BlackoutChargingFX || !GetMesh())
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(
		BlackoutChargingFX,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true);
}

void ADragonBoss::MulticastSpawnBlackoutReleaseFX_Implementation(FVector Location)
{
	if (!BlackoutReleaseFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		BlackoutReleaseFX,
		Location);
}

void ADragonBoss::MulticastStartPhaseTwoFX_Implementation()
{
	EnsurePhaseTwoFX();
	ApplyPhaseTwoMaterial();
}

bool ADragonBoss::TakeArcherQVolleyDamage(ABaseCharacter* Attacker, int32 VolleyId, float Damage)
{
	if (!Attacker || VolleyId == INDEX_NONE || !HasAuthority())
	{
		return false;
	}

	int32& LastVolleyId = LastArcherQVolleyByAttacker.FindOrAdd(Attacker, INDEX_NONE);
	if (LastVolleyId == VolleyId)
	{
		return false;
	}

	LastVolleyId = VolleyId;
	TakeBossDamage(Damage);
	return true;
}

void ADragonBoss::EnsurePhaseTwoFX()
{
	if (!PhaseTwoFX || !GetMesh() || IsValid(PhaseTwoFXComponent))
	{
		return;
	}

	// The phase effect remains attached to the mesh for the rest of the fight.
	PhaseTwoFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		PhaseTwoFX,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false);
}

void ADragonBoss::ApplyPhaseTwoMaterial()
{
	if (GetMesh() && PhaseTwoOverlayMaterial)
	{
		GetMesh()->SetOverlayMaterial(PhaseTwoOverlayMaterial);
	}
}

float ADragonBoss::GetCurrentDamageMultiplier() const
{
	return bIsPhaseTwo ? PhaseTwoDamageMultiplier : 1.f;
}

void ADragonBoss::CheckPhaseTwo()
{
	if (bIsPhaseTwo || MaxHP <= 0.f || CurrentHP > MaxHP * PhaseTwoHealthThreshold)
	{
		return;
	}

	bIsPhaseTwo = true;
	ForceNetUpdate();
	MulticastStartPhaseTwoFX();

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon entered phase two. Damage multiplier: %.2f"),
		PhaseTwoDamageMultiplier);
}

void ADragonBoss::OnRep_IsPhaseTwo()
{
	if (bIsPhaseTwo)
	{
		EnsurePhaseTwoFX();
		ApplyPhaseTwoMaterial();
	}
}

void ADragonBoss::OnRep_CurrentState()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Dragon State Changed : %d"),
		(int32)CurrentState);
}

void ADragonBoss::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADragonBoss, CurrentState);
	DOREPLIFETIME(ADragonBoss, CurrentHP);
	DOREPLIFETIME(ADragonBoss, bIsFlying);
	DOREPLIFETIME(ADragonBoss, bIsPhaseTwo);
}

void ADragonBoss::OnRep_CurrentHP()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Boss HP : %.1f"),
		CurrentHP);
}

void ADragonBoss::DebugBite()
{
	StartAttackTelegraph(EDragonAttackType::Bite);
}

void ADragonBoss::DebugCloseBreath()
{
	StartAttackTelegraph(EDragonAttackType::CloseBreath);
}

void ADragonBoss::DebugBreath()
{
	StartAttackTelegraph(EDragonAttackType::Breath);
}

void ADragonBoss::DebugDebuff()
{
	StartAttackTelegraph(EDragonAttackType::Debuff);
}

void ADragonBoss::DebugCenterMechanic()
{
	if (!bCenterMechanicActive && CurrentState != EDragonState::Dead)
	{
		CenterMechanicPattern();
	}
}

void ADragonBoss::MulticastPlayDeath_Implementation()
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}
