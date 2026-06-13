// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DangerZone.generated.h"

class UDecalComponent;

UENUM(BlueprintType)
enum class EDangerZoneType : uint8
{
	Circle,
	Cone,
	Line
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADangerZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADangerZone();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UDecalComponent* Decal;

	UPROPERTY(EditAnywhere)
	float LifeTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDangerZoneType ZoneType;

	UFUNCTION(BlueprintCallable)
	void SetLineShape();
	
	UFUNCTION(BlueprintCallable)
	void SetBiteShape();

	UFUNCTION(BlueprintCallable)
	void SetCloseBreathShape();
};
