// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraSystem.h"
#include "ArrowProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ABaseCharacter;
class AMonster;

UENUM(BlueprintType)
enum class EArrowType : uint8
{
	Normal,
	QExplosive,
	Explosive,
	Pierce
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API AArrowProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArrowProjectile();

	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY()
	ABaseCharacter* OwnerCharacter;

	UFUNCTION()
	void OnArrowOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY()
	EArrowType ArrowType = EArrowType::Normal;

	TSet<AMonster*> HitMonsters;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	float DamageMultiplier = 1.f;

	// All five arrows spawned by one Archer Q share this identifier.
	UPROPERTY()
	int32 QVolleyId = INDEX_NONE;

	void Explode();

	UPROPERTY(EditAnywhere)
	float QExplosionRadius = 120.f;

	UPROPERTY(EditAnywhere)
	float EExplosionRadius = 300.f;

	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ArcherTrailFX;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ArcherRTrailFX;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraSystem> QImpactFX;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraSystem> EImpactFX;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraSystem> RImpactFX;

	void SetupTrail();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
