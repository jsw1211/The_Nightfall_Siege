// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Reflector.generated.h"

class UStaticMeshComponent;
class ADragonBoss;

UCLASS()
class THE_NIGHTFALL_SIEGE_API AReflector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AReflector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UFUNCTION()
	void ReflectBreath();

	UPROPERTY(EditAnywhere)
	ADragonBoss* DragonBoss;
};
