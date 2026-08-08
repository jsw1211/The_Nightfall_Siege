// Fill out your copyright notice in the Description page of Project Settings.

#include "DragonBreathProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Reflector.h"
#include "DragonBoss.h"
#include "BaseCharacter.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ADragonBreathProjectile::ADragonBreathProjectile()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

	RootComponent = Collision;

	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	Mesh->SetupAttachment(RootComponent);

	ProjectileMovement =
		CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovement"));

	// 화염구 속도
	ProjectileMovement->InitialSpeed = 2200.f;
	ProjectileMovement->MaxSpeed = 2200.f;

	// 핵심: 중력 제거
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// 날아가는 방향으로 화염구 회전
	ProjectileMovement->bRotationFollowsVelocity = true;

	// 빠른 투사체의 충돌 누락 방지
	ProjectileMovement->bForceSubStepping = true;

	// 화염구가 너무 오래 남지 않도록 수명 설정
	InitialLifeSpan = 8.f;

	Collision->OnComponentBeginOverlap.AddDynamic(
		this,
		&ADragonBreathProjectile::OnOverlapBegin);
}

// Called when the game starts or when spawned
void ADragonBreathProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			ProjectileFX,
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
}

// Called every frame
void ADragonBreathProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADragonBreathProjectile::LaunchInDirection(
	const FVector& Direction)
{
	const FVector NormalizedDirection =
		Direction.GetSafeNormal();

	if (NormalizedDirection.IsNearlyZero())
	{
		return;
	}

	ProjectileMovement->Velocity =
		NormalizedDirection *
		ProjectileMovement->InitialSpeed;

	SetActorRotation(
		NormalizedDirection.Rotation());
}

void ADragonBreathProjectile::ReflectTowardDragon(
	ADragonBoss* TargetDragon)
{
	if (!HasAuthority() || bReflected || !TargetDragon)
	{
		return;
	}

	bReflected = true;

	// 드래곤 몸통보다 약간 위를 향하게 함
	const FVector TargetPoint =
		TargetDragon->GetActorLocation() +
		FVector(0.f, 0.f, 150.f);

	const FVector ReturnDirection =
		(TargetPoint - GetActorLocation()).GetSafeNormal();

	if (ReturnDirection.IsNearlyZero())
	{
		return;
	}

	ProjectileMovement->InitialSpeed = ReturnSpeed;
	ProjectileMovement->MaxSpeed = ReturnSpeed;

	ProjectileMovement->Velocity =
		ReturnDirection * ReturnSpeed;

	SetActorRotation(
		ReturnDirection.Rotation());

	// 네트워크 클라이언트에 변경 즉시 전파
	ForceNetUpdate();
}

void ADragonBreathProjectile::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	// 충돌에 의한 실제 게임 로직은 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	// 자기 자신은 무시
	if (OtherActor == this)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Error,
		TEXT("Projectile Hit : %s"),
		*OtherActor->GetName());

	/*
	 * ---------------------------------------------------------
	 * 1. Reflector 충돌
	 * ---------------------------------------------------------
	 */

	AReflector* Reflector =
		Cast<AReflector>(OtherActor);

	if (Reflector)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Breath Hit Reflector"));

		// 이미 반사된 화염구가 Reflector에 다시 맞는 경우
		// 다시 반사되지 않도록 무시
		if (bReflected)
		{
			return;
		}

		// Reflector가 가지고 있는 DragonBoss에게
		// 화염구를 되돌림
		ReflectTowardDragon(
			Reflector->DragonBoss);

		return;
	}

	/*
	 * ---------------------------------------------------------
	 * 2. Dragon 충돌
	 * ---------------------------------------------------------
	 */

	ADragonBoss* Dragon =
		Cast<ADragonBoss>(OtherActor);

	if (Dragon)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Breath Hit Dragon"));

		// 일반 화염구가 발사 직후 드래곤 자신과
		// 겹친 경우에는 아무것도 하지 않음
		if (!bReflected)
		{
			return;
		}

		// 반사된 화염구가 드래곤에게 맞은 경우
		Dragon->OnBreathReflected();

		if (ExplosionFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ExplosionFX,
				GetActorLocation()
			);
		}

		Destroy();

		return;
	}

	/*
	 * ---------------------------------------------------------
	 * 3. Player 충돌
	 * ---------------------------------------------------------
	 */

	ABaseCharacter* Player =
		Cast<ABaseCharacter>(OtherActor);

	if (Player)
	{
		// 반사된 화염구는 플레이어에게 피해를 주지 않음
		if (bReflected)
		{
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Breath Hit Player"));

		const float Damage =
			Player->MaxHP *
			0.8f *
			DamageMultiplier;

		Player->TakePlayerDamage(Damage);

		if (ExplosionFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ExplosionFX,
				GetActorLocation()
			);
		}

		Destroy();

		return;
	}
}

void ADragonBreathProjectile::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(
		OutLifetimeProps);

	DOREPLIFETIME(
		ADragonBreathProjectile,
		bReflected);
}