// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBreathProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Reflector.h"
#include "DragonBoss.h"
#include "BaseCharacter.h"
#include "Engine/DamageEvents.h"

// Sets default values
ADragonBreathProjectile::ADragonBreathProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

    RootComponent = Collision;

    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    Collision->SetCollisionResponseToAllChannels(ECR_Overlap);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

    Mesh->SetupAttachment(RootComponent);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

    ProjectileMovement->InitialSpeed = 1200.f;
    ProjectileMovement->MaxSpeed = 1200.f;

    Collision->OnComponentBeginOverlap.AddDynamic(this, &ADragonBreathProjectile::OnOverlapBegin);

}

// Called when the game starts or when spawned
void ADragonBreathProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADragonBreathProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

    UE_LOG(LogTemp, Error,
        TEXT("Projectile Hit : %s"),
        *OtherActor->GetName());

    AReflector* Reflector = Cast<AReflector>(OtherActor);

    if (Reflector)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Breath Hit Reflector"));

        Reflector->ReflectBreath();

        Destroy();

        return;
    }

    ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);

    if (Player)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Breath Hit Player"));

        float Damage = Player->MaxHP * 0.8f;

        Player->TakeDamage(Damage, FDamageEvent(), nullptr, this);

        Destroy();

        return;
    }
}

