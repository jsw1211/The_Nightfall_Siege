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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

	UPROPERTY()
	ADungeonManager* DungeonManager;

	bool bIsDead;

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
};
