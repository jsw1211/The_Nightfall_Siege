// Fill out your copyright notice in the Description page of Project Settings.


#include "Coin.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "BaseCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ACoin::ACoin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);

    Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
    RootComponent = Sphere;

    Sphere->SetSphereRadius(60.f);
    Sphere->SetCollisionProfileName("OverlapAll");

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
    Mesh->SetupAttachment(Sphere);

    GoldDropEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GoldDropEffect"));
    GoldDropEffect->SetupAttachment(RootComponent);
    GoldDropEffect->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
    GoldDropEffect->SetAutoActivate(true);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> GoldDropEffectAsset(
        TEXT("/Game/Effects/dropped_item/NS_Gold.NS_Gold"));
    if (GoldDropEffectAsset.Succeeded())
    {
        GoldDropEffect->SetAsset(GoldDropEffectAsset.Object);
    }

    Sphere->OnComponentBeginOverlap.AddDynamic(
        this,
        &ACoin::OnOverlap);
}

// Called when the game starts or when spawned
void ACoin::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACoin::OnOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    ABaseCharacter* Player =
        Cast<ABaseCharacter>(OtherActor);

    if (!Player)
    {
        return;
    }

    Player->ServerPickupCoin(this);
}

