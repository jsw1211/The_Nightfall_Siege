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
}

// Called when the game starts or when spawned
void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AArrowProjectile::OnArrowOverlap);

	ProjectileMovement->OnProjectileStop.AddDynamic(this,&AArrowProjectile::OnProjectileStop);

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
		if (ArrowType == EArrowType::Explosive)
		{
			Explode();
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

		if (ArrowType == EArrowType::Explosive ||
			ArrowType == EArrowType::QExplosive)
		{
			Explode();
			return;
		}
		else if (ArrowType == EArrowType::Pierce)
		{
		}
		else
		{
			Destroy();
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

		if (ArrowType == EArrowType::QExplosive || ArrowType == EArrowType::Explosive)
		{
			Explode();
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

		if (ArrowType == EArrowType::Explosive ||
			ArrowType == EArrowType::QExplosive)
		{
			Explode();
			return;
		}
		else if (ArrowType == EArrowType::Pierce)
		{
		}
		else
		{
			Destroy();
			return;
		}
	}
}

void AArrowProjectile::Explode()
{
	if (ArrowType == EArrowType::QExplosive)
	{
		if (QImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				QImpactFX,
				GetActorLocation());
		}
	}
	else
	{
		if (EImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				EImpactFX,
				GetActorLocation());
		}
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
	bool bHit =
		GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);

	if (bHit)
	{
		for (auto& Result : Overlaps)
		{
			AMonster* Monster = Cast<AMonster>(Result.GetActor());

			if (Monster && OwnerCharacter && ArrowType != EArrowType::Explosive)
			{
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
			!Dragon->IsWithinDamageRadius(GetActorLocation(), Radius))
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

	Destroy();
}

void AArrowProjectile::OnProjectileStop(
	const FHitResult& ImpactResult)
{
	if (ArrowType == EArrowType::Explosive ||
		ArrowType == EArrowType::QExplosive)
	{
		Explode();
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

	Destroy();
}

void AArrowProjectile::SetupTrail()
{
	if (ArrowType == EArrowType::Pierce)
	{
		if (ArcherRTrailFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				ArcherRTrailFX,
				RootComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				true
			);
		}
	}
	else
	{
		if (ArcherTrailFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				ArcherTrailFX,
				RootComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				true
			);
		}
	}
}

