// Fill out your copyright notice in the Description page of Project Settings.


#include "Reflector.h"
#include "Components/StaticMeshComponent.h"
#include "DragonBoss.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AReflector::AReflector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>( TEXT("Mesh"));

	RootComponent = Mesh;
}

// Called when the game starts or when spawned
void AReflector::BeginPlay()
{
	Super::BeginPlay();

    TArray<AActor*> Dragons;

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADragonBoss::StaticClass(), Dragons);

    if (Dragons.Num() > 0)
    {
        DragonBoss = Cast<ADragonBoss>(Dragons[0]);

        UE_LOG(LogTemp, Warning, TEXT("Dragon Found"));
    }
}

// Called every frame
void AReflector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AReflector::ReflectBreath()
{
    if (!DragonBoss)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Breath Reflected"));

    if (DragonBoss->bCenterMechanicActive)
    {
        DragonBoss->OnCenterMechanicSuccess();
    }
    else
    {
        DragonBoss->OnBreathReflected();
    }
}

