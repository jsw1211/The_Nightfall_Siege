	// Fill out your copyright notice in the Description page of Project Settings.

	#pragma once

	#include "CoreMinimal.h"
	#include "GameFramework/Actor.h"
	#include "VillageManager.generated.h"

	class APortal;

	UCLASS()
	class THE_NIGHTFALL_SIEGE_API AVillageManager : public AActor
	{
		GENERATED_BODY()
	
	public:	
		// Sets default values for this actor's properties
		AVillageManager();

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;

	public:	
		// Called every frame
		virtual void Tick(float DeltaTime) override;

		UPROPERTY(EditAnywhere, Category = "Portal")
		TSubclassOf<APortal> BossPortalClass;

		UPROPERTY(EditAnywhere, Category = "Portal")
		FVector BossPortalLocation;

		UFUNCTION()
		void SpawnBossPortal();
	};
