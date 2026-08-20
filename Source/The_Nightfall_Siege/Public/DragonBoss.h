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
class UCapsuleComponent;
class UBoxComponent;

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

	// The Character capsule remains the movement/root collider.  These three
	// query-only hitbox components provide the actual damageable silhouette and are
	// attached to the dragon's animated bones.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision|Hitboxes")
	TObjectPtr<UCapsuleComponent> HeadHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision|Hitboxes")
	TObjectPtr<UBoxComponent> BodyHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision|Hitboxes")
	TObjectPtr<UCapsuleComponent> TailHitbox;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// Fits the damage hitboxes to the current animated bone positions.
	void UpdateDamageHitboxes();
	FVector ClampToMovementBounds(const FVector& DesiredLocation) const;
	void MoveWithSweepAndSlide(const FVector& DesiredLocation);

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

	// Checks the boss's animated damage hitboxes instead of its root location.
	// Ground-targeted skills use this so they also register on the head or tail.
	bool IsWithinDamageRadius(const FVector& Location, float Radius) const;

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

	// Kept separate from bIsPhaseTwo so joining clients do not show the phase-two
	// overlay before the transition montage reaches its final half second.
	UPROPERTY(ReplicatedUsing = OnRep_PhaseTwoMaterialApplied, BlueprintReadOnly, Category = "Boss|Phase Two")
	bool bPhaseTwoMaterialApplied = false;

	UFUNCTION()
	void OnRep_PhaseTwoMaterialApplied();

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
	FTimerHandle AttackEndHandle;
	FTimerHandle ChargingEffectHandle;
	FTimerHandle PhaseTwoMaterialHandle;
	FTimerHandle PhaseTwoMaterialReplicationHandle;
	FTimerHandle PhaseTransitionEndHandle;

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
	FVector GetCloseBreathCenter() const;

	void ExecuteTelegraphedAttack(
		EDragonAttackType AttackType);
	void StartBlackoutChargingFX();

	UPROPERTY(BlueprintReadOnly)
	bool bIsTelegraphing = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ADangerZone> DangerZoneClass;

	// Close-breath damage and its DangerZone both use these same values.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Close Breath", meta = (ClampMin = "0.0"))
	float CloseBreathHitRadius = 525.f;

	// Distance from MouthSocket to the centre of the close-breath area.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Close Breath", meta = (ClampMin = "0.0"))
	float CloseBreathForwardOffset = 200.f;

	// The bite hitbox is a rectangle extending forward from MouthSocket.
	// These values are shared by damage detection and its warning decal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Bite", meta = (ClampMin = "0.0"))
	float BiteHitRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Bite", meta = (ClampMin = "0.0"))
	float BiteHalfWidth = 150.f;

	// Pulls the entire bite rectangle back from MouthSocket while preserving its size.
	// This value is shared by damage detection and its warning decal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Bite", meta = (ClampMin = "0.0"))
	float BiteBackwardOffset = 300.f;

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
	void StopBreathTracking();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DebuffMontage;

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

	// Horizontal movement is constrained to a circle centred on the world origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement", meta = (ClampMin = "0.0"))
	float MovementBoundaryRadius = 4600.f;

	// The encounter remains idle until at least one living player enters this range.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter", meta = (ClampMin = "0.0"))
	float EncounterStartRange = 1000.f;

	bool bEncounterStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> BiteFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> CloseBreathFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Blackout")
	TObjectPtr<UNiagaraSystem> BlackoutChargingFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Blackout")
	TObjectPtr<UNiagaraSystem> BlackoutReleaseFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BlackoutChargingFXComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Phase Two")
	TObjectPtr<UNiagaraSystem> PhaseTwoFX;

	// Applied as a mesh overlay so the dragon keeps its base texture while the
	// phase-two shader remains visible across every client.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Phase Two")
	TObjectPtr<UMaterialInterface> PhaseTwoOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PhaseTwoFXComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Barrier")
	TObjectPtr<UNiagaraSystem> BarrierFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BarrierFXComponent;

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
	void MulticastStopBlackoutChargingFX();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBlackoutReleaseFX(FVector Location);


	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartPhaseTwoFX();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartPhaseTwoTransition();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartBarrierFX();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopBarrierFX();

	void CheckPhaseTwo();
	void FinishPhaseTwoTransition();
	void MarkPhaseTwoMaterialApplied();
	void EnsurePhaseTwoFX();
	void EnsureBarrierFX();
	void ApplyPhaseTwoMaterial();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	////////////////////////////////////디버그
	void DebugBite();
	void DebugCloseBreath();
	void DebugBreath();
	void DebugDebuff();
	void DebugCenterMechanic();
};
