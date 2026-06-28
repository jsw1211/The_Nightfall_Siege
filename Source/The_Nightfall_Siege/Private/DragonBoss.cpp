// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBoss.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "DungeonPrism.h"
#include "DragonBreathProjectile.h"
#include "DangerZone.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ADragonBoss::ADragonBoss()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	StartAttackTelegraph(EDragonAttackType::Breath);

	bFirstBreathDone = true;
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
			FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

			FVector Forward =
				GetActorForwardVector();

			FVector SpawnLocation = MouthLocation + Forward * 500.f;

			CurrentBreathZone->SetActorLocation(SpawnLocation);

			CurrentBreathZone->SetActorRotation(TargetRotation);
		}
	}

	if (bCenterMechanicActive)
	{
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

		const float CombatRange = 700.f;

		if (bIsFlying)
		{
			FlyToTarget();
		}
		else
		{
			if (Distance > 2000.f)
			{
				FlyToTarget();
			}
			else if (Distance > AttackRange)
			{
				WalkToTarget();
			}
			else
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

	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Breath"));

	MulticastPlayAttack(EDragonAttackType::Breath);

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			OnAttackFinished();
		},
		4.0f,
		false
	);
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

void ADragonBoss::WalkToTarget()
{
	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController && TargetPlayer)
	{
		AIController->MoveToActor(
			TargetPlayer,
			AttackRange);
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

		PlayAnimMontage(LeapMontage);

		return;
	}

	float Distance =
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation());

	if (Distance <= AttackRange)
	{
		bIsFlying = false;

		CurrentState =
			EDragonState::Landing;

		PlayAnimMontage(
			LandMontage);

		return;
	}

	FVector TargetLoc =
		TargetPlayer->GetActorLocation();

	TargetLoc.Z += 300.f;

	FVector NewLocation =
		FMath::VInterpConstantTo(
			GetActorLocation(),
			TargetLoc,
			GetWorld()->GetDeltaSeconds(),
			1200.f);

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

	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->MoveToLocation(
			ArenaCenter,
			100.f
		);
	}

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

	CurrentHP -= Damage;

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

		CurrentHP -= Damage;

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

	UE_LOG(LogTemp, Warning,
		TEXT("ExecutePattern Distance = %f"),
		Distance);

	if (Distance > AttackRange)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Target Out Of Range : %f"), Distance);

		if (Distance > 2000.f)
		{
			FlyToTarget();
		}
		else
		{
			WalkToTarget();
		}

		StartAttackCycle();
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

	UE_LOG(LogTemp, Warning,
		TEXT("Center Target : %s"),
		*TargetPlayer->GetName());

	GetWorldTimerManager().SetTimer(
		CenterFailHandle,
		this,
		&ADragonBoss::StartAttackCycle,
		8.f,
		false
	);
}

void ADragonBoss::OnCenterMechanicSuccess()
{
	if (!bCenterMechanicActive)
	{
		return;
	}

	float Damage = MaxHP * 0.1f;

	CurrentHP -= Damage;

	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic Success"));

	UE_LOG(LogTemp, Warning,
		TEXT("Damage : %f"),
		Damage);

	UE_LOG(LogTemp, Warning,
		TEXT("Boss HP : %f"),
		CurrentHP);

	bCenterMechanicActive = false;

	StartAttackCycle();

	GetWorldTimerManager().ClearTimer(
		CenterFailHandle
	);
}

void ADragonBoss::OnAttackFinished()
{
	UpdatePlayerList();

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
	}

	bIsAttacking = false;

	StartAttackCycle();
}

void ADragonBoss::Die()
{
	CurrentState = EDragonState::Dead;

	bIsAttacking = false;

	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().ClearTimer(
		AttackTimerHandle
	);

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Dead"));

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
		FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

		FVector Forward = GetActorForwardVector();

		FVector SpawnLocation = MouthLocation + Forward * 500.f;

		FRotator Rot = (TargetPlayer->GetActorLocation() - MouthLocation).Rotation();

		ADangerZone* Zone = GetWorld()->SpawnActor<ADangerZone>(DangerZoneClass, SpawnLocation, Rot);

		if (Zone)
		{
			Zone->ZoneType = EDangerZoneType::Line;
			Zone->OnRep_ZoneType();
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

		if (bCenterMechanicActive)
		{
			bCenterTracking = false;

			bCenterMechanicActive = false;
			bCenterBreathStarted = false;
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
	UE_LOG(LogTemp, Error,
		TEXT("===== LEAP FINISHED ====="));

	bIsLeaping = false;
	bIsFlying = true;

	CurrentState = EDragonState::Flying;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Flying);

	UE_LOG(LogTemp, Error,
		TEXT("Target = %s"),
		*TargetPlayer->GetName());

	FlyToTarget();
}

void ADragonBoss::OnLandFinished()
{
	bIsFlying = false;

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

	float Damage = AttackPower * 1.0f;

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

	float Damage = AttackPower * 2.0f;

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

	GetWorld()->SpawnActor<ADragonBreathProjectile>(BreathProjectileClass, MouthLocation, SpawnRotation);
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

