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

    // The dragon deliberately ignores generic WorldDynamic objects so its
    // broad root capsule does not consume projectile hits. Give reflectors a
    // dedicated object channel so they can still block dragon movement without
    // changing damage hitboxes or unrelated dynamic actors.
    Mesh->SetCollisionObjectType(ECC_GameTraceChannel2);
    // Delegate step-up permission to CanBeBaseForCharacter below. This keeps
    // the restriction dragon-specific instead of making the reflector
    // unwalkable for players too.
    Mesh->CanCharacterStepUpOn = ECB_Owner;
}

// Called when the game starts or when spawned
void AReflector::BeginPlay()
{
	Super::BeginPlay();

    // Blueprint component defaults are applied after the native constructor.
    // Enforce the runtime channel and class-specific step-up behavior here too.
    Mesh->SetCollisionObjectType(ECC_GameTraceChannel2);
    Mesh->CanCharacterStepUpOn = ECB_Owner;

    TArray<AActor*> Dragons;

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADragonBoss::StaticClass(), Dragons);

    if (Dragons.Num() > 0)
    {
        DragonBoss = Cast<ADragonBoss>(Dragons[0]);

        UE_LOG(LogTemp, Warning, TEXT("Dragon Found"));
    }
}

bool AReflector::CanBeBaseForCharacter(APawn* Pawn) const
{
    if (Pawn && Pawn->IsA<ADragonBoss>())
    {
        return false;
    }

    return Super::CanBeBaseForCharacter(Pawn);
}

// Called every frame
void AReflector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AReflector::ReflectBreath()
{
    // The projectile owns the reflection flow and resolves the result only
    // when its reflected copy physically reaches the dragon.  Calling the
    // boss here would resolve shield/damage as soon as the reflector is hit.
    UE_LOG(LogTemp, Warning, TEXT("Breath reflected; awaiting dragon impact"));
}

