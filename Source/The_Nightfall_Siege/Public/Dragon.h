// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Dragon.generated.h"

UENUM(BlueprintType)
enum class EDragonAttackType : uint8
{
	Bite,
	Breath,
	CloseBreath
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADragon : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADragon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    // 공격 애니메이션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
    UAnimMontage* BiteMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
    UAnimMontage* BreathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
    UAnimMontage* CloseBreathMontage;

    // 공격 간 대기 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
    float AttackDelay = 3.0f;

    // 공격 중 체크
    bool bIsAttacking = false;

    // 현재 공격 순서
    int32 CurrentAttackIndex = 0;

    // 타이머
    FTimerHandle AttackTimerHandle;

    // 공격 시작
    void StartAttack();

    // 공격 종료
    UFUNCTION()
    void OnAttackEnded(UAnimMontage* Montage, bool bInterrupted);

    // 다음 공격 예약
    void ScheduleNextAttack();

    // 공격 선택
    EDragonAttackType SelectNextAttack();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
