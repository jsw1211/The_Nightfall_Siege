// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coin.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ACoin : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACoin();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere)
    USphereComponent* Sphere;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    // Persistent pickup highlight.  Because this is an actor component, it
    // begins with the replicated coin and is removed automatically when the
    // coin is collected and the actor is destroyed.
    UPROPERTY(VisibleAnywhere, Category = "Effects")
    UNiagaraComponent* GoldDropEffect;

    UFUNCTION()
    void OnOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

};
