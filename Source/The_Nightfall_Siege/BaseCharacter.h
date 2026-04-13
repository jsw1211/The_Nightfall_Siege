// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "BaseCharacter.generated.h"

UCLASS()
class THE_NIGHTFALL_SIEGE_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Die();
	void PlayHit();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Q(const FInputActionValue& Value);
	void W(const FInputActionValue& Value);
	void E(const FInputActionValue& Value);
	void R(const FInputActionValue& Value);
	void ToggleInventory();
	UFUNCTION()
	void TakePlayerDamage(float Damage);

protected:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* QMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* WMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* EMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* RMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* IMC_BaseCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Q;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_W;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_E;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_R;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Inventory;


	// 쿨타임 상태
	bool bCanUseQ = true;
	bool bCanUseW = true;
	bool bCanUseE = true;
	bool bCanUseR = true;

	// 쿨타임 시간
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float QCooldown = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float WCooldown = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float ECooldown = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float RCooldown = 7.0f;

	// 타이머 핸들
	FTimerHandle QCooldownTimer;
	FTimerHandle WCooldownTimer;
	FTimerHandle ECooldownTimer;
	FTimerHandle RCooldownTimer;

	// 쿨타임 리셋 함수
	void ResetQCooldown();
	void ResetWCooldown();
	void ResetECooldown();
	void ResetRCooldown();

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;
	UUserWidget* InventoryWidget;
	bool bInventoryOpen = false;

	// 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP;

	// 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// 애니메이션 상태 전달용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsHit = false;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HitMontage;
};
