// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Monster.h"
#include "DragonBoss.h"

namespace
{
void ConfigureNonBlockingWeaponCollision(
	USkeletalMeshComponent* Mesh,
	UBoxComponent* WeaponCollision)
{
	if (Mesh)
	{
		// The skeletal mesh is visual only; WeaponCollision owns all melee hits.
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(TEXT("NoCollision"));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->CanCharacterStepUpOn = ECB_No;
		Mesh->SetCanEverAffectNavigation(false);
	}

	if (WeaponCollision)
	{
		// Some weapon Blueprints have serialized blocking responses that override
		// the native constructor. Reapplying overlap-only responses before every
		// swing prevents the hitbox from pushing players while preserving overlap
		// notifications for monsters and all dragon hitbox object types.
		WeaponCollision->SetSimulatePhysics(false);
		WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponCollision->SetGenerateOverlapEvents(true);
		WeaponCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
		WeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
		WeaponCollision->CanCharacterStepUpOn = ECB_No;
		WeaponCollision->SetCanEverAffectNavigation(false);
	}
}
}

// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));

	WeaponCollision->SetupAttachment(Mesh);

	ConfigureNonBlockingWeaponCollision(Mesh, WeaponCollision);
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint defaults are applied after the native constructor, so enforce
	// the collision contract again once the final component templates exist.
	ConfigureNonBlockingWeaponCollision(Mesh, WeaponCollision);
	
	WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnWeaponOverlap);

}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponBase::OnWeaponOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMonster* Monster = Cast<AMonster>(OtherActor);

	if (Monster)
	{
		if (HitMonsters.Contains(Monster))
		{
			return;
		}

		HitMonsters.Add(Monster);

		UE_LOG(LogTemp, Warning, TEXT("Weapon Hit Monster"));

		if (OwnerCharacter)
		{
			Monster->TakeMonsterDamage(OwnerCharacter->GetAttackPower());
		}
	}

	ADragonBoss* Dragon = Cast<ADragonBoss>(OtherActor);

	if (Dragon)
	{
		if (HitDragons.Contains(Dragon))
		{
			return;
		}
		HitDragons.Add(Dragon);

		UE_LOG(LogTemp, Warning,
			TEXT("Weapon Hit Dragon"));

		if (OwnerCharacter)
		{
			Dragon->TakeBossDamage(OwnerCharacter->GetAttackPower());
		}
	}
}

void AWeaponBase::EnableCollision()
{
	HitMonsters.Empty();
	HitDragons.Empty();

	// Apply responses before enabling queries so an already-overlapping player
	// never sees even one frame of the Blueprint's stale blocking hitbox.
	ConfigureNonBlockingWeaponCollision(Mesh, WeaponCollision);
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWeaponBase::DisableCollision()
{
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

