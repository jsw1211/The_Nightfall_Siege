// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBoss.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "AIController.h"

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
	
	StartAttackCycle();
}

// Called every frame
void ADragonBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetPlayer == nullptr)
	{
		TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	}

	if (TargetPlayer && !bIsAttacking)
	{
		float Distance = FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation()
		);

		if (Distance < 1200.f)
		{
			WalkToTarget();
		}
		else
		{
			FlyToTarget();
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
	float RandomDelay = FMath::RandRange(5.f, 7.f);

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ADragonBoss::ExecuteRandomAttack,
		RandomDelay,
		false
	);
}

EDragonAttackType ADragonBoss::ChooseRandomAttack()
{
	int32 Rand = FMath::RandRange(1, 100);

	// Bite 35%
	if (Rand <= 35)
	{
		return EDragonAttackType::Bite;
	}

	// Close Breath 35%
	else if (Rand <= 70)
	{
		return EDragonAttackType::CloseBreath;
	}

	// Breath 20%
	else if (Rand <= 90)
	{
		return EDragonAttackType::Breath;
	}

	// Debuff 10%
	return EDragonAttackType::Debuff;
}

void ADragonBoss::ExecuteRandomAttack()
{
	EDragonAttackType AttackType = ChooseRandomAttack();

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
		BiteAttack();
		break;

	case EDragonAttackType::CloseBreath:
		CloseBreathAttack();
		break;

	case EDragonAttackType::Breath:
		BreathAttack();
		break;

	case EDragonAttackType::Debuff:
		DebuffAttack();
		break;
	}

	StartAttackCycle();
}

void ADragonBoss::BiteAttack()
{
	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Bite"));

	float Damage = AttackPower * 1.0f;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && BiteMontage)
	{
		AnimInstance->Montage_Play(BiteMontage);
	}

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			bIsAttacking = false;
		},
		2.0f,
		false
	);
}

void ADragonBoss::CloseBreathAttack()
{
	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Close Breath"));

	float Damage = AttackPower * 2.0f;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && CloseBreathMontage)
	{
		AnimInstance->Montage_Play(CloseBreathMontage);
	}

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			bIsAttacking = false;
		},
		3.0f,
		false
	);
}

void ADragonBoss::BreathAttack()
{
	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Breath"));

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && BreathMontage)
	{
		AnimInstance->Montage_Play(BreathMontage);
	}

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			bIsAttacking = false;
		},
		4.0f,
		false
	);
}

void ADragonBoss::DebuffAttack()
{
	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Debuff"));

	// TODO:
	// 시야 제한
	// HP 감소 디버프

	FTimerHandle AttackEndHandle;

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		[this]()
		{
			bIsAttacking = false;
		},
		2.5f,
		false
	);
}

void ADragonBoss::WalkToTarget()
{
	if (bIsFlying)
	{
		bIsFlying = false;

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		CurrentState = EDragonState::Walking;

		UE_LOG(LogTemp, Warning, TEXT("Dragon Walking"));
	}

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController && TargetPlayer)
	{
		AIController->MoveToActor(TargetPlayer, 150.f);
	}
}

void ADragonBoss::FlyToTarget()
{
	if (!bIsFlying)
	{
		bIsFlying = true;

		GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		CurrentState = EDragonState::Flying;

		UE_LOG(LogTemp, Warning, TEXT("Dragon Flying"));
	}

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController && TargetPlayer)
	{
		AIController->MoveToActor(TargetPlayer, 300.f);
	}
}

void ADragonBoss::FlyToCenter()
{
	bIsFlying = true;

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Flying To Center"));

	// TODO:
	// AI Move To ArenaCenter
}

