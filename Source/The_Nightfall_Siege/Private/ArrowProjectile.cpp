// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "DragonBoss.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values
AArrowProjectile::AArrowProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));

	Mesh->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);

	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AArrowProjectile::OnArrowOverlap);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AArrowProjectile::OnProjectileStop);
}

// Called when the game starts or when spawned
void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// Blueprint defaults must never make an arrow or its visual ricochet.
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->Bounciness = 0.f;
	ProjectileMovement->bForceSubStepping = true;

	// Only Archer R may pass through a pawn. Normal/Q/E arrows use a blocking
	// sweep so the projectile cannot continue or appear to ricochet after the
	// first monster contact.
	Collision->SetCollisionResponseToChannel(
		ECC_Pawn,
		ArrowType == EArrowType::Pierce ? ECR_Overlap : ECR_Block);

	// The skeletal mesh is visual-only. Blueprint physics/collision overrides
	// must not let the visible arrow separate from the projectile and bounce.
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called every frame
void AArrowProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AArrowProjectile::OnArrowOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMonster* Monster = Cast<AMonster>(OtherActor);

	if (Monster && OwnerCharacter)
	{
		// Q/E damage is owned exclusively by the explosion. Applying a direct hit
		// first would damage a point-blank target twice.
		if (ArrowType == EArrowType::Explosive)
		{
			// The rain field belongs to the monster that intercepted the arrow,
			// even when that happens before the 1500-unit maximum destination.
			Explode(Monster->GetActorLocation());
			return;
		}
		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(GetActorLocation());
			return;
		}

		if (HitMonsters.Contains(Monster))
		{
			return;
		}

		HitMonsters.Add(Monster);

		float Damage = OwnerCharacter->GetAttackPower() * DamageMultiplier;

		Monster->TakeMonsterDamage(Damage);

		if (ArrowType == EArrowType::Pierce)
		{
			if (RImpactFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					RImpactFX,
					SweepResult.ImpactPoint
				);
			}
		}

		if (ArrowType == EArrowType::Pierce)
		{
		}
		else
		{
			StopTrailAndDestroy();
			return;
		}
	}

	ADragonBoss* Dragon = Cast<ADragonBoss>(OtherActor);

	if (Dragon && OwnerCharacter)
	{
		if (HitDragons.Contains(Dragon))
		{
			return;
		}
		HitDragons.Add(Dragon);

		if (ArrowType == EArrowType::Explosive)
		{
			Explode(Dragon->GetActorLocation());
			return;
		}
		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(GetActorLocation());
			return;
		}

		float Damage = OwnerCharacter->GetAttackPower() * DamageMultiplier;
		if (ArrowType == EArrowType::Pierce &&
			OwnerCharacter->bRBonusDamage)
		{
			Damage += Dragon->MaxHP * 0.05f;
		}
		Dragon->TakeBossDamage(Damage);

		if (ArrowType == EArrowType::Pierce)
		{
			if (RImpactFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					RImpactFX,
					SweepResult.ImpactPoint
				);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Arrow Damage : %f"),	Damage);

		UE_LOG(LogTemp, Warning, TEXT("Arrow Hit Dragon"));

		if (ArrowType == EArrowType::Pierce)
		{
		}
		else
		{
			StopTrailAndDestroy();
			return;
		}
	}
}

void AArrowProjectile::Explode(const FVector& ImpactCenter)
{
	if (bImpactHandled)
	{
		return;
	}
	bImpactHandled = true;

	if (ArrowType == EArrowType::QExplosive)
	{
		if (QImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				QImpactFX,
				ImpactCenter);
		}
	}
	else
	{
		if (EImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				EImpactFX,
				ImpactCenter);
		}
	}

	// Archer E uses the actual intercepted monster (or ground impact) as the
	// field center. The 1500-unit point is only its maximum flight destination.
	if (ArrowType == EArrowType::Explosive)
	{
		if (OwnerCharacter)
		{
			OwnerCharacter->ApplyArcherERainDamage(ImpactCenter);
		}
		StopTrailAndDestroy();
		return;
	}

	TArray<FOverlapResult> Overlaps;

	float Radius = EExplosionRadius;

	if (ArrowType == EArrowType::QExplosive)
	{
		Radius = QExplosionRadius;
	}

	FCollisionShape Sphere =
		FCollisionShape::MakeSphere(Radius);

	TSet<ADragonBoss*> ExplodedDragons;
	TSet<AMonster*> ExplodedMonsters;
	bool bHit =
		GetWorld()->OverlapMultiByChannel(Overlaps, ImpactCenter, FQuat::Identity, ECC_Pawn, Sphere);

	if (bHit)
	{
		for (auto& Result : Overlaps)
		{
			AMonster* Monster = Cast<AMonster>(Result.GetActor());

			if (Monster && OwnerCharacter && ArrowType != EArrowType::Explosive &&
				!ExplodedMonsters.Contains(Monster))
			{
				ExplodedMonsters.Add(Monster);
				float Damage = OwnerCharacter->GetAttackPower() * DamageMultiplier;

				Monster->TakeMonsterDamage(Damage);
			}

			ADragonBoss* Dragon = Cast<ADragonBoss>(Result.GetActor());

			if (Dragon && OwnerCharacter)
			{
				if (ExplodedDragons.Contains(Dragon))
				{
					continue;
				}
				ExplodedDragons.Add(Dragon);

				float Damage = OwnerCharacter->GetAttackPower() * DamageMultiplier;
				if (ArrowType == EArrowType::QExplosive)
				{
					Dragon->TakeArcherQVolleyDamage(OwnerCharacter, QVolleyId, Damage);
					continue;
				}
				if (ArrowType == EArrowType::Explosive)
				{
					// Archer E's boss damage is dealt by its falling-arrow area over time.
					continue;
				}

				if (ArrowType == EArrowType::Pierce &&
					OwnerCharacter->bRBonusDamage)
				{
					Damage += Dragon->MaxHP * 0.05f;
				}
				Dragon->TakeBossDamage(Damage);
			}
		}
	}

	// Explosion overlap can miss a bone-attached hitbox while the dragon is
	// animating.  Query its segmented hitboxes as a fallback so Archer Q still
	// damages the boss when the visual explosion reaches any part of it.
	for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
	{
		ADragonBoss* Dragon = *It;
		if (!Dragon || !OwnerCharacter || ExplodedDragons.Contains(Dragon) ||
			!Dragon->IsWithinDamageRadius(ImpactCenter, Radius))
		{
			continue;
		}

		ExplodedDragons.Add(Dragon);
		if (ArrowType == EArrowType::QExplosive)
		{
			Dragon->TakeArcherQVolleyDamage(
				OwnerCharacter,
				QVolleyId,
				OwnerCharacter->GetAttackPower() * DamageMultiplier);
		}
	}

	StopTrailAndDestroy();
}

void AArrowProjectile::OnProjectileStop(
	const FHitResult& ImpactResult)
{
	if (bImpactHandled)
	{
		return;
	}

	AActor* HitActor = ImpactResult.GetActor();

	if (AMonster* Monster = Cast<AMonster>(HitActor))
	{
		if (ArrowType == EArrowType::Explosive)
		{
			Explode(Monster->GetActorLocation());
			return;
		}

		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(Monster->GetActorLocation());
			return;
		}

		if (ArrowType == EArrowType::Normal && OwnerCharacter &&
			!HitMonsters.Contains(Monster))
		{
			HitMonsters.Add(Monster);
			Monster->TakeMonsterDamage(
				OwnerCharacter->GetAttackPower() * DamageMultiplier);
		}

		StopTrailAndDestroy();
		return;
	}

	if (ADragonBoss* Dragon = Cast<ADragonBoss>(HitActor))
	{
		if (ArrowType == EArrowType::Explosive)
		{
			Explode(Dragon->GetActorLocation());
			return;
		}

		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(Dragon->GetActorLocation());
			return;
		}

		if (ArrowType == EArrowType::Normal && OwnerCharacter &&
			!HitDragons.Contains(Dragon))
		{
			HitDragons.Add(Dragon);
			Dragon->TakeBossDamage(
				OwnerCharacter->GetAttackPower() * DamageMultiplier);
		}

		StopTrailAndDestroy();
		return;
	}

	if (ArrowType == EArrowType::Explosive ||
		ArrowType == EArrowType::QExplosive)
	{
		Explode(ImpactResult.ImpactPoint);
		return;
	}

	if (ArrowType == EArrowType::Pierce && OwnerCharacter)
	{
		for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
		{
			ADragonBoss* Dragon = *It;
			if (!Dragon || HitDragons.Contains(Dragon) ||
				!Dragon->IsWithinDamageRadius(ImpactResult.ImpactPoint, 150.f))
			{
				continue;
			}

			HitDragons.Add(Dragon);
			float Damage = OwnerCharacter->GetAttackPower() * DamageMultiplier;
			if (OwnerCharacter->bRBonusDamage)
			{
				Damage += Dragon->MaxHP * 0.05f;
			}
			Dragon->TakeBossDamage(Damage);
			break;
		}
	}

	StopTrailAndDestroy();
}

void AArrowProjectile::SetupTrail()
{
	if (TrailComponent)
	{
		return;
	}

	UNiagaraSystem* TrailSystem = ArrowType == EArrowType::Pierce
		? ArcherRTrailFX
		: ArcherTrailFX;

	if (!TrailSystem)
	{
		return;
	}

	TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem,
		RootComponent,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false);

	if (TrailComponent)
	{
		TrailComponent->SetAbsolute(false, false, false);
	}
}

void AArrowProjectile::StopTrailAndDestroy()
{
	bImpactHandled = true;

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailComponent)
	{
		TrailComponent->DeactivateImmediate();
		TrailComponent->DestroyComponent();
		TrailComponent = nullptr;
	}

	Destroy();
}

