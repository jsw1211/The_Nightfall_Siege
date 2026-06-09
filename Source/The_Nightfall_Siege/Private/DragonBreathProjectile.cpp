// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBreathProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ADragonBreathProjectile::ADragonBreathProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

    RootComponent = Collision;

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
}

