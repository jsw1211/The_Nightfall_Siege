// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseCharacter.h"
#include "DungeonPrism.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADungeonPrism : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonPrism();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PrismMesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* SphereCollision;

	bool bActivated;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:

	void ActivatePrism();

	UPROPERTY()
	TArray<ABaseCharacter*> ActivatedPlayers;

	void RemoveDarknessDebuff();
};
