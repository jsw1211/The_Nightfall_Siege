// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "Altar.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "DragonBoss.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

namespace
{
	// Keep E's impact VFX immediate, but let players see it before damage lands.
	constexpr float ArcherEImpactDamageDelay = 0.5f;
}

// Sets default values
AArrowProjectile::AArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 실제 액터의 기준점
	USceneComponent* SceneRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	RootComponent = SceneRoot;

	// 충돌 구체
	Collision =
		CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

	Collision->SetupAttachment(SceneRoot);

	// 화살 외형
	Mesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));

	Mesh->SetupAttachment(SceneRoot);

	// 투사체 이동
	ProjectileMovement =
		CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovement"));

	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetGenerateOverlapEvents(true);

	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);

	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	Collision->OnComponentBeginOverlap.AddDynamic(
		this,
		&AArrowProjectile::OnArrowOverlap);

	ProjectileMovement->OnProjectileStop.AddDynamic(
		this,
		&AArrowProjectile::OnProjectileStop);
}

// Called when the game starts or when spawned
void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();

	// ProjectileMovement automatically chose the Blueprint's non-primitive
	// SceneRoot, so the child sphere never participated in the movement sweep.
	// Promote the sphere at runtime while preserving the authored mesh offsets.
	// This also makes the trail (attached after BeginPlay) follow the component
	// whose per-actor ignore list actually controls projectile movement.
	USceneComponent* PreviousRoot = GetRootComponent();
	if (Collision && PreviousRoot && PreviousRoot != Collision)
	{
		Collision->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		SetRootComponent(Collision);
		PreviousRoot->AttachToComponent(
			Collision,
			FAttachmentTransformRules::KeepWorldTransform);
	}

	ProjectileMovement->SetUpdatedComponent(Collision);
	
	// Blueprint defaults must never make an arrow or its visual ricochet.
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->Bounciness = 0.f;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->bRotationFollowsVelocity = true;

	// Normal/Q/E use the same Pawn overlap path: overlap -> damage/effect ->
	// immediate destroy. Reapply the complete query response here so a Blueprint
	// collision preset cannot turn a monster contact back into a blocking hit.
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	// The skeletal mesh is visual-only. Blueprint physics/collision overrides
	// must not let the visible arrow separate from the projectile and bounce.
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Keep these actors' own collision settings intact. Only this projectile's
	// moving collision ignores its owner, other players, and altar actors.
	if (OwnerCharacter)
	{
		Collision->IgnoreActorWhenMoving(OwnerCharacter, true);
	}

	// Players, monsters, and the dragon all use the Pawn channel, so changing
	// that channel would also remove valid enemy hits. Ignore only the other
	// player-character actors for this projectile's movement sweep instead.
	for (TActorIterator<ABaseCharacter> It(GetWorld()); It; ++It)
	{
		ABaseCharacter* PlayerCharacter = *It;
		if (PlayerCharacter && PlayerCharacter != OwnerCharacter)
		{
			Collision->IgnoreActorWhenMoving(PlayerCharacter, true);
		}
	}

	for (TActorIterator<AAltar> It(GetWorld()); It; ++It)
	{
		AAltar* Altar = *It;

		// 제단 전체를 화살 충돌에서 완전히 무시
		Collision->IgnoreActorWhenMoving(Altar, true);
	}

	ResolveInitialOverlaps();
}

// Called every frame
void AArrowProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector AArrowProjectile::ResolveOverlapImpactPoint(
	UPrimitiveComponent* OtherComp,
	bool bFromSweep,
	const FHitResult& SweepResult) const
{
	// ProjectileMovement supplies the swept sphere contact point here. Unlike
	// the target actor's origin, it cannot jump when an altar moves a monster
	// out of its exclusion capsule during the same frame.
	if (bFromSweep)
	{
		return SweepResult.ImpactPoint;
	}

	const FVector ProjectileLocation = Collision
		? Collision->GetComponentLocation()
		: GetActorLocation();

	FVector ClosestPoint = ProjectileLocation;
	if (OtherComp &&
		OtherComp->GetClosestPointOnCollision(ProjectileLocation, ClosestPoint) >= 0.f)
	{
		return ClosestPoint;
	}

	// Initial overlaps do not contain sweep data. The projectile center remains
	// a stable launch-time contact point for this point-blank case.
	return ProjectileLocation;
}

void AArrowProjectile::ResolveInitialOverlaps()
{
	if (bImpactHandled || !OwnerCharacter || !Collision || !GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> InitialOverlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ArrowInitialOverlap), false, this);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const FCollisionShape Shape = FCollisionShape::MakeSphere(
		FMath::Max(Collision->GetScaledSphereRadius(), 1.f));
	GetWorld()->OverlapMultiByObjectType(
		InitialOverlaps,
		Collision->GetComponentLocation(),
		FQuat::Identity,
		ObjectQuery,
		Shape,
		QueryParams);

	for (const FOverlapResult& Result : InitialOverlaps)
	{
		AActor* OtherActor = Result.GetActor();
		if (!OtherActor || OtherActor == OwnerCharacter ||
			(!OtherActor->IsA<AMonster>() && !OtherActor->IsA<ADragonBoss>()))
		{
			continue;
		}

		FHitResult InitialHit;
		InitialHit.ImpactPoint = Collision->GetComponentLocation();
		OnArrowOverlap(
			Collision,
			OtherActor,
			Result.GetComponent(),
			INDEX_NONE,
			false,
			InitialHit);

		if (bImpactHandled)
		{
			break;
		}
	}
}

void AArrowProjectile::OnArrowOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{

	AMonster* Monster = Cast<AMonster>(OtherActor);

	if (Monster && OwnerCharacter)
	{
		const FVector ImpactPoint = ResolveOverlapImpactPoint(
			OtherComp,
			bFromSweep,
			SweepResult);

		// Q/E damage is owned exclusively by the explosion. Applying a direct hit
		// first would damage a point-blank target twice.
		if (ArrowType == EArrowType::Explosive)
		{
			// The rain field belongs to the exact contact point, even when a
			// monster intercepts the arrow before its configured destination.
			Explode(ImpactPoint);
			return;
		}
		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(ImpactPoint, Monster);
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
		const FVector ImpactPoint = ResolveOverlapImpactPoint(
			OtherComp,
			bFromSweep,
			SweepResult);

		if (ArrowType == EArrowType::Explosive)
		{
			Explode(ImpactPoint);
			return;
		}
		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(ImpactPoint);
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

void AArrowProjectile::Explode(
	const FVector& ImpactCenter,
	AMonster* DirectHitMonster)
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
			FTimerDelegate DelayedDamageDelegate = FTimerDelegate::CreateUObject(
				OwnerCharacter,
				&ABaseCharacter::ApplyArcherERainDamage,
				ImpactCenter);
			FTimerHandle DelayedDamageTimerHandle;
			OwnerCharacter->GetWorldTimerManager().SetTimer(
				DelayedDamageTimerHandle,
				DelayedDamageDelegate,
				ArcherEImpactDamageDelay,
				false);
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

			if (Monster && OwnerCharacter &&
				ArrowType != EArrowType::Explosive &&
				ArrowType != EArrowType::QExplosive &&
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

	if (ArrowType == EArrowType::QExplosive &&
		DirectHitMonster &&
		OwnerCharacter)
	{
		const float Damage =
			OwnerCharacter->GetAttackPower() * DamageMultiplier;

		DirectHitMonster->TakeMonsterDamage(Damage);
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
			Explode(ImpactResult.ImpactPoint);
			return;
		}

		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(ImpactResult.ImpactPoint, Monster);
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
			Explode(ImpactResult.ImpactPoint);
			return;
		}

		if (ArrowType == EArrowType::QExplosive)
		{
			Explode(ImpactResult.ImpactPoint);
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

	if (ArrowType == EArrowType::Explosive)
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

