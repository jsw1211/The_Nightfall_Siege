// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DragonBoss.generated.h"

UENUM(BlueprintType)
enum class EDragonState : uint8
{
	Idle,
	Walking,
	Flying,
	Landing,
	Attacking,
	Dead
};

UENUM(BlueprintType)
enum class EDragonAttackType : uint8
{
	Bite,
	CloseBreath,
	Breath,
	Debuff
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADragonBoss : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADragonBoss();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// =========================
	// Boss Stats
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float MaxHP = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float CurrentHP = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float AttackPower = 120.f;

	// =========================
	// State
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	EDragonState CurrentState = EDragonState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bIsFlying = false;

	// =========================
	// Target
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	AActor* TargetPlayer;

	// =========================
	// Arena
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	FVector ArenaCenter;

	// =========================
	// Attack Timer
	// =========================

	FTimerHandle AttackTimerHandle;

	// =========================
	// Functions
	// =========================

	void StartAttackCycle();

	void ExecuteRandomAttack();

	EDragonAttackType ChooseRandomAttack();

	void BiteAttack();

	void CloseBreathAttack();

	void BreathAttack();

	void DebuffAttack();

	void WalkToTarget();

	void FlyToTarget();

	void FlyToCenter();

};
