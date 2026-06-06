// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "BaseCharacter.h"
#include "WeaponBase.generated.h"

class AMonster;

UCLASS()
class THE_NIGHTFALL_SIEGE_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* WeaponCollision;

	UFUNCTION()
	void OnWeaponOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void EnableCollision();
	void DisableCollision();

	UPROPERTY()
	ABaseCharacter* OwnerCharacter;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
