// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimMontage.h"
#include "Monster.generated.h"

class ADungeonManager;
class AAltar;
class UWidgetComponent;
class ABaseCharacter;
class ACoin;
class ADungeonPrism;
class UMaterialInterface;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API AMonster : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMonster();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 공격 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAttacking;

	// 공격 가능 여부
	bool bCanAttack;

	// 공격 쿨타임
	float AttackCooldown;

	FTimerHandle AttackTimerHandle;

	void ResetAttack();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	float MaxHP;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	float CurrentHP;

	// 도발 대상
	UPROPERTY()
	ABaseCharacter* TauntTarget = nullptr;

	// 도발 여부
	UPROPERTY(BlueprintReadOnly)
	bool bIsTaunted = false;

	// 도발 시간
	FTimerHandle TauntTimerHandle;

	// 도발 적용
	void ApplyTaunt(ABaseCharacter* Target);

	// 도발 해제
	void ClearTaunt();

	// 데미지 받는 함수
	UFUNCTION()
	void TakeMonsterDamage(float Damage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowDamage(float Damage);

	void MulticastShowDamage_Implementation(float Damage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeath();

	void MulticastPlayDeath_Implementation();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDamage(float Damage);
	/** Red overlay shown whenever this monster takes valid damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float HitFlashDuration = 0.5f;

	/** Material applied over the mesh during the hit reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reaction")
	TObjectPtr<UMaterialInterface> HitFlashOverlayMaterial;

	/** Plays the hit-flash locally; invoked through the damage multicast. */
	void PlayHitFlash();

	/** Removes the temporary hit-flash material overlay. */
	void ClearHitFlash();

	FTimerHandle HitFlashTimerHandle;

	UPROPERTY()
	ADungeonManager* DungeonManager;

	bool bIsDead;

	UPROPERTY(ReplicatedUsing = OnRep_BarrierActive)
	bool bBarrierActive = false;

	UFUNCTION()
	void OnRep_BarrierActive();

	UPROPERTY()
	AAltar* OwnerAltar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UWidgetComponent* HPWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DeathMontage;

	UFUNCTION(BlueprintCallable)
	void DestroyMonster();

	FTimerHandle DeathTimerHandle;

	UFUNCTION()
	void DestroyMonsterDelay();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SightRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SightAngle = 90.f;

	bool CanSeePlayer(ABaseCharacter* Player);

	UPROPERTY(BlueprintReadOnly)
	bool bIsChasing = false;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<class ACoin> CoinClass;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<class ADungeonPrism> PrismClass;

	UFUNCTION()
	void SpawnCoin();

	UFUNCTION()
	void SpawnPrism();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 무적 상태 베리어 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
	TObjectPtr<UNiagaraSystem> BarrierFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
	TObjectPtr<UNiagaraComponent> BarrierFXComponent;

	void EnsureBarrierFX();
};

