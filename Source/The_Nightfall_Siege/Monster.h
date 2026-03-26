// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Monster.generated.h"

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

};
