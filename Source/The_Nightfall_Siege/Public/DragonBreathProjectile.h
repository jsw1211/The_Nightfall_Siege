// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "DragonBreathProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADragonBreathProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADragonBreathProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    USphereComponent* Collision;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> ProjectileFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> ExplosionFX;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
