// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBoss.h"

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

	if (TargetPlayer)
	{
		float Distance = FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation()
		);

		// 가까우면 걷기
		if (Distance < 1200.f)
		{
			WalkToTarget();
		}
		// 멀면 비행
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
	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Bite"));

	float Damage = AttackPower * 1.0f;

	// TODO:
	// 단일 대상에게 데미지 적용
	// bite 애니메이션 재생
}

void ADragonBoss::CloseBreathAttack()
{
	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Close Breath"));

	float Damage = AttackPower * 2.0f;

	// TODO:
	// 근거리 범위 데미지
	// closerangebreath 애니메이션 재생
}

void ADragonBoss::BreathAttack()
{
	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Breath"));

	// 중앙 이동
	FlyToCenter();

	// TODO:
	// breath 애니메이션
	// 직선 범위 공격
	// 플레이어 최대 체력의 80% 피해
}

void ADragonBoss::DebuffAttack()
{
	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Debuff"));

	// TODO:
	// 모든 플레이어 시야 제한
	// 초당 최대 체력 2% 감소
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

	// TODO:
	// AI Move To Target
	// walking 애니메이션
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

	// TODO:
	// AI Move To Target
	// flying 애니메이션
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

