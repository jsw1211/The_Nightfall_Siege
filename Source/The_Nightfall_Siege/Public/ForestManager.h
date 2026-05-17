// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestManager.generated.h"

UCLASS()
class THE_NIGHTFALL_SIEGE_API AForestManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AForestManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere)
	TSubclassOf<ADungeonPortal> PortalClass;

	UPROPERTY(EditAnywhere)
	TArray<AActor*> PortalSpawnPoints;

	void SpawnDungeonPortal();
};
