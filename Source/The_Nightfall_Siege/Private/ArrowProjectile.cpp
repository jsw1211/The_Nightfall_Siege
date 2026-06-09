// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "Engine/OverlapResult.h"
#include "DragonBoss.h"

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
		if (HitMonsters.Contains(Monster))
		{
			return;
		}

		HitMonsters.Add(Monster);

		Monster->TakeMonsterDamage(OwnerCharacter->GetAttackPower());

		if (ArrowType == EArrowType::Explosive)
		{
			Explode();
			return;
		}
		else if (ArrowType == EArrowType::Pierce)
		{
		}
		else
		{
		}
	}

	ADragonBoss* Dragon = Cast<ADragonBoss>(OtherActor);

	if (Dragon && OwnerCharacter)
	{
		Dragon->TakeBossDamage(OwnerCharacter->GetAttackPower());

		UE_LOG(LogTemp, Warning, TEXT("Arrow Hit Dragon"));

		if (ArrowType == EArrowType::Explosive)
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
	TArray<FOverlapResult> Overlaps;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(300.f);

	bool bHit =
		GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);

	if (bHit)
	{
		for (auto& Result : Overlaps)
		{
			AMonster* Monster = Cast<AMonster>(Result.GetActor());

			if (Monster && OwnerCharacter)
			{
				Monster->TakeMonsterDamage(OwnerCharacter->GetAttackPower() * OwnerCharacter->EMultiplier);
			}

			ADragonBoss* Dragon = Cast<ADragonBoss>(Result.GetActor());

			if (Dragon && OwnerCharacter)
			{
				Dragon->TakeBossDamage(OwnerCharacter->GetAttackPower() * OwnerCharacter->EMultiplier);
			}
		}
	}

	Destroy();
}

void AArrowProjectile::OnProjectileStop(
	const FHitResult& ImpactResult)
{
	if (ArrowType == EArrowType::Explosive)
	{
		Explode();
	}
	else
	{
		Destroy();
	}
}

