// Fill out your copyright notice in the Description page of Project Settings.


#include "DangerZone.h"
#include "Components/DecalComponent.h"

// Sets default values
ADangerZone::ADangerZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));

	RootComponent = Decal;

}

// Called when the game starts or when spawned
void ADangerZone::BeginPlay()
{
	Super::BeginPlay();
	
	SetLifeSpan(LifeTime);

}

// Called every frame
void ADangerZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADangerZone::SetLineShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			500.f,
			5000.f
		);
}

void ADangerZone::SetBiteShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			350.f,
			350.f);
}

void ADangerZone::SetCloseBreathShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			1000.f,
			700.f);
}

