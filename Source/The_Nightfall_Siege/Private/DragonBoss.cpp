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
	
	bShielded = true;
	bCanTakeDamage = false;

	CurrentState = EDragonState::Idle;

	UpdatePlayerList();

	StartAttackCycle();
}

// Called every frame
void ADragonBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetPlayer == nullptr)
	{
		ChooseRandomTarget();
	}

	if (TargetPlayer && !bIsAttacking && !bStunned)
	{
		float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

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
	UE_LOG(LogTemp, Warning, TEXT("StartAttackCycle"));

	ChooseRandomTarget();

	float RandomDelay = FMath::RandRange(5.f, 7.f);

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

	if (Rand <= 40)
	{
		return EDragonAttackType::Bite;
	}

	if (Rand <= 80)
	{
		return EDragonAttackType::CloseBreath;
	}

	if (Rand <= 100)
	{
		return EDragonAttackType::Debuff;
	}

	return EDragonAttackType::Bite;
}

void ADragonBoss::ExecuteRandomAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("ExecuteRandomAttack"));

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

void ADragonBoss::TakeBossDamage(float Damage)
{
	if (!bCanTakeDamage)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Dragon Is Invincible"));
		return;
	}

	CurrentHP -= Damage;

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon HP : %f"),
		CurrentHP);

	if (CurrentHP <= 0.f)
	{
		CurrentState = EDragonState::Dead;
	}
}

void ADragonBoss::OnBreathReflected()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Shield Broken"));

	bShielded = false;
	bCanTakeDamage = true;

	bStunned = true;

	CurrentState = EDragonState::Attacking;

	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ADragonBoss::EndStun,
		5.f,
		false
	);
}

void ADragonBoss::EndStun()
{
	bStunned = false;

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

	TargetPlayer =
		AlivePlayers[RandomIndex];

	UE_LOG(LogTemp, Warning,
		TEXT("New Target : %s"),
		*TargetPlayer->GetName());
}

EDragonPatternType ADragonBoss::ChoosePattern()
{
	/*int32 Rand = FMath::RandRange(1, 100);

	if (Rand <= 70)
	{
		return EDragonPatternType::NormalAttack;
	}

	if (Rand <= 90)
	{
		return EDragonPatternType::TargetChange;
	}

	return EDragonPatternType::CenterMechanic;*/
	return EDragonPatternType::TargetChange;
}

void ADragonBoss::ExecutePattern()
{
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
}

void ADragonBoss::TargetChangePattern()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Target Change Pattern"));

	ChooseRandomTarget();

	if (!TargetPlayer)
	{
		StartAttackCycle();
		return;
	}

	float Distance = FVector::Dist(
		GetActorLocation(),
		TargetPlayer->GetActorLocation()
	);

	if (Distance > 1500.f)
	{
		int32 Rand = FMath::RandRange(0, 1);

		if (Rand == 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Far Breath"));

			BreathAttack();
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Fly Only"));

			FlyToTarget();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Close Target"));

		WalkToTarget();
	}

	StartAttackCycle();
}

void ADragonBoss::CenterMechanicPattern()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic"));

	FlyToCenter();

	StartAttackCycle();
}

