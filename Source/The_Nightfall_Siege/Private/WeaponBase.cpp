// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Monster.h"

// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));

	WeaponCollision->SetupAttachment(Mesh);

	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponCollision->SetGenerateOverlapEvents(true);

	WeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);

	WeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
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
		UE_LOG(LogTemp, Warning, TEXT("Weapon Hit Monster"));
		if (OwnerCharacter)
		{
			Monster->TakeMonsterDamage(OwnerCharacter->GetAttackPower());
		}
	}
}

void AWeaponBase::EnableCollision()
{
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWeaponBase::DisableCollision()
{
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

