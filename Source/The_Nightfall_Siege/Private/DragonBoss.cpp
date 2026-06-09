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

	ArenaCenter = FVector(0.f, 0.f, 0.f);
	
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

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	if (TargetPlayer == nullptr)
	{
		ChooseRandomTarget();
	}

	if (TargetPlayer && !bIsAttacking && !bStunned && !bIsTelegraphing)
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
	/*int32 Rand = FMath::RandRange(1, 100);

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

	return EDragonAttackType::Bite;*/
	return EDragonAttackType::Bite;
}

void ADragonBoss::ExecuteRandomAttack()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ExecuteRandomAttack"));

	EDragonAttackType AttackType = ChooseRandomAttack();

	StartAttackTelegraph(AttackType);
}

void ADragonBoss::BiteAttack()
{
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

	float Damage = AttackPower * 1.0f;

	ABaseCharacter* Player = Cast<ABaseCharacter>(TargetPlayer);

	if (Player)
	{
		Player->TakePlayerDamage(Damage);
	}

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
			OnAttackFinished();
		},
		2.0f,
		false
	);
}

void ADragonBoss::CloseBreathAttack()
{
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

	float Damage = AttackPower * 2.0f;

	ABaseCharacter* Player = Cast<ABaseCharacter>(TargetPlayer);

	if (Player)
	{
		Player->TakePlayerDamage(Damage);
	}

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
			OnAttackFinished();
		},
		3.0f,
		false
	);
}

void ADragonBoss::BreathAttack()
{
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

	if (BreathProjectileClass)
	{
		FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 200.f;

		FRotator SpawnRotation = GetActorRotation();

		GetWorld()->SpawnActor<ADragonBreathProjectile>(BreathProjectileClass, SpawnLocation, SpawnRotation);
	}

	ABaseCharacter* Player = Cast<ABaseCharacter>(TargetPlayer);

	if (Player)
	{
		float Damage = Player->MaxHP * 0.8f;

		Player->TakePlayerDamage(Damage);
	}

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
			OnAttackFinished();
		},
		4.0f,
		false
	);
}

void ADragonBoss::DebuffAttack()
{
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
		Die();
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
			BreathAttack();
			return;
		}
		else
		{
			FlyToTarget();
		}
	}
	else
	{
		WalkToTarget();
	}

	StartAttackCycle();
}

void ADragonBoss::CenterMechanicPattern()
{
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
	if (CurrentState == EDragonState::Dead)
	{
		return;
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
	bIsTelegraphing = true;

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	if (DangerZoneClass &&
		TargetPlayer)
	{
		FVector SpawnLocation =
			TargetPlayer->GetActorLocation();

		SpawnLocation.Z -= 90.f;

		GetWorld()->SpawnActor<ADangerZone>(
			DangerZoneClass,
			SpawnLocation,
			FRotator(-90.f, 0.f, 0.f));
	}

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
		UE_LOG(LogTemp, Warning,
			TEXT("Bite Danger Area"));
		break;

	case EDragonAttackType::CloseBreath:
		UE_LOG(LogTemp, Warning,
			TEXT("Close Breath Danger Area"));
		break;

	case EDragonAttackType::Breath:
		UE_LOG(LogTemp, Warning,
			TEXT("Breath Danger Area"));
		break;

	case EDragonAttackType::Debuff:
		UE_LOG(LogTemp, Warning,
			TEXT("Debuff Warning"));
		break;
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
		BreathAttack();
		break;

	case EDragonAttackType::Debuff:
		DebuffAttack();
		break;
	}
}

