// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "BaseCharacter.h"
#include "NiagaraSystem.h"
#include "DragonBoss.generated.h"

class ADragonBreathProjectile;
class UMaterialInterface;
class ADangerZone;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class EDragonState : uint8
{
	Idle,
	Walking,
	Leap,
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

	int32 BiteCount = 0;
	int32 CloseBreathCount = 0;
	int32 DebuffCount = 0;

	int32 TargetChangeCount = 0;
	int32 TargetChangeFlyCount = 0;
	int32 TargetChangeBreathCount = 0;

	int32 CenterMechanicCount = 0;

	int32 TotalPatternCount = 0;

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

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, BlueprintReadOnly, Category = "Boss")
	float CurrentHP = 8000.f;

	UFUNCTION()
	void OnRep_CurrentHP();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float AttackPower = 120.f;

	// A Q volley is allowed to damage the boss once regardless of how many
	// projectiles or overlapping explosions from that volley connect.
	bool TakeArcherQVolleyDamage(ABaseCharacter* Attacker, int32 VolleyId, float Damage);

	// =========================
	// Phase Two
	// =========================

	// Phase two starts once the dragon reaches this fraction of MaxHP.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Phase Two", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseTwoHealthThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Phase Two", meta = (ClampMin = "1.0"))
	float PhaseTwoDamageMultiplier = 1.5f;

	UPROPERTY(ReplicatedUsing = OnRep_IsPhaseTwo, BlueprintReadOnly, Category = "Boss|Phase Two")
	bool bIsPhaseTwo = false;

	UFUNCTION()
	void OnRep_IsPhaseTwo();

	UFUNCTION(BlueprintPure, Category = "Boss|Phase Two")
	float GetCurrentDamageMultiplier() const;

	// =========================
	// State
	// =========================

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly)
	EDragonState CurrentState = EDragonState::Idle;

	UFUNCTION()
	void OnRep_CurrentState();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bIsFlying = false;

	// =========================
	// Target
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	ABaseCharacter* TargetPlayer;

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
	void ResetPrismCleanseParticipants();
	void RegisterPrismCleanseParticipant(ABaseCharacter* Player);

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowDamage(float Damage);

	void MulticastShowDamage_Implementation(float Damage);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDamage(float Damage);

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
	FTimerHandle LeapRecoveryHandle;
	FTimerHandle LandingRecoveryHandle;

	FTimerHandle StunTimerHandle;

	void Die();
	void FailCenterMechanic();

	UPROPERTY(EditAnywhere)
	TSubclassOf<ADragonBreathProjectile> BreathProjectileClass;

	FTimerHandle TelegraphHandle;

	void StartAttackTelegraph(
		EDragonAttackType AttackType);

	void ExecuteTelegraphedAttack(
		EDragonAttackType AttackType);

	UPROPERTY(BlueprintReadOnly)
	bool bIsTelegraphing = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ADangerZone> DangerZoneClass;

	// The projectile travels at 2200 units/sec for 8 seconds by default.
	// Keep the warning zone long enough to cover its full reachable path.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Breath", meta = (ClampMin = "0.0"))
	float BreathTelegraphRange = 17600.f;

	FTimerHandle CenterBreathHandle;

	bool bCenterBreathStarted = false;

	TSet<TObjectPtr<ABaseCharacter>> PrismCleanseParticipants;

	// All prism holders must gather within this radius of one another before
	// their F interactions can clear the blackout debuff.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Debuff", meta = (ClampMin = "0.0"))
	float PrismCleanseGatherRadius = 1000.f;

	bool ArePrismHoldersGathered(const TArray<AActor*>& PlayerActors) const;

	FTimerHandle CenterTrackingHandle;

	bool bCenterTracking = false;

	// Central breath must remain stationary until its montage end callback.
	bool bMovementLockedForBreath = false;

	UPROPERTY()
	ADangerZone* CurrentBreathZone = nullptr;

	bool bFirstBreathDone = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* LeapMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* LandMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DeathMontage;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeath();

	UFUNCTION(BlueprintCallable)
	void OnLeapFinished();

	UFUNCTION(BlueprintCallable)
	void OnLandFinished();

	UPROPERTY()
	FRotator TelegraphRotation;

	UPROPERTY()
	bool bIsLeaping = false;

	TMap<TObjectPtr<ABaseCharacter>, int32> LastArcherQVolleyByAttacker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float FlyStartRange = 2000.f;

	// Walking stops and attacks become eligible at this same distance.
	// Keeping one value avoids a dead zone between movement and attacks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float AttackStartRange = 700.f;

	// Hysteresis prevents the boss from stopping exactly on the old 700-unit
	// boundary and endlessly re-entering StartAttackCycle without attacking.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float AttackEngageRange = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float ChaseStopRange = 500.f;

	// Once flight starts it remains latched until this distance is reached.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float FlyLandingRange = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> BiteFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> CloseBreathFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Blackout")
	TObjectPtr<UNiagaraSystem> BlackoutChargingFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Blackout")
	TObjectPtr<UNiagaraSystem> BlackoutReleaseFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Phase Two")
	TObjectPtr<UNiagaraSystem> PhaseTwoFX;

	// Applied as a mesh overlay so the dragon keeps its base texture while the
	// phase-two shader remains visible across every client.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Phase Two")
	TObjectPtr<UMaterialInterface> PhaseTwoOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PhaseTwoFXComponent;

	UFUNCTION(BlueprintCallable)
	void BiteHit();

	UFUNCTION(BlueprintCallable)
	void CloseBreathFire();

	UFUNCTION(BlueprintCallable)
	void BreathFire();
	void OnBreathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAttack(EDragonAttackType AttackType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMovementTransition(bool bLanding);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBiteFX(FVector Location);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnCloseBreathFX(FVector Location);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBlackoutChargingFX();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBlackoutReleaseFX(FVector Location);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartPhaseTwoFX();

	void CheckPhaseTwo();
	void EnsurePhaseTwoFX();
	void ApplyPhaseTwoMaterial();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	////////////////////////////////////디버그
	void DebugBite();
	void DebugCloseBreath();
	void DebugBreath();
	void DebugDebuff();
	void DebugCenterMechanic();
};
