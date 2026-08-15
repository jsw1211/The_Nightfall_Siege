// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "DragonBreathProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class ADragonBoss;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADragonBreathProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADragonBreathProjectile();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 발사 방향을 설정하고 해당 방향으로 투사체를 발사
	void LaunchInDirection(const FVector& Direction);

	// Reflector에 맞았을 때 드래곤 방향으로 화염구를 반사
	void ReflectTowardDragon(ADragonBoss* TargetDragon);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 네트워크 복제 변수 등록
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraComponent> FireballFXComponent;

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

	// 반사된 화염구인지 여부
	// 반사된 화염구는 플레이어에게 다시 피해를 주지 않음
	UPROPERTY(Replicated)
	bool bReflected = false;

	// The dragon has multiple hitbox components.  A returned projectile may
	// overlap more than one of them in one frame, but must resolve only once.
	bool bResolvedAgainstDragon = false;

	// 반사된 화염구가 보스에게 돌아가는 속도
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float ReturnSpeed = 2800.f;

public:
	// Set by the dragon when the projectile is spawned.
	// Damage is resolved on the server, so this does not need to be replicated.
	float DamageMultiplier = 1.f;
};


