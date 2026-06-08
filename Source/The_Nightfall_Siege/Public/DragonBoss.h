// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "BaseCharacter.h"
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

UENUM(BlueprintType)
enum class EDragonPatternType : uint8
{
	NormalAttack,
	TargetChange,
	CenterMechanic
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

	UPROPERTY()
	TArray<ABaseCharacter*> AlivePlayers;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* BiteMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* BreathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* CloseBreathMontage;

	UPROPERTY(BlueprintReadOnly)
	float Speed;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAttacking = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bShielded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bCanTakeDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bStunned = false;

	void OnBreathReflected();

	void EndStun();

	void TakeBossDamage(float Damage);

	void UpdatePlayerList();

	void ChooseRandomTarget();

	EDragonPatternType ChoosePattern();

	void ExecutePattern();

	void TargetChangePattern();

	void CenterMechanicPattern();

	UPROPERTY(BlueprintReadOnly)
	bool bCenterMechanicActive = false;

	void OnCenterMechanicSuccess();

	void OnAttackFinished();

	FTimerHandle CenterFailHandle;
};
