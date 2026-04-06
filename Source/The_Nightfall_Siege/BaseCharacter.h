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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void LeftPunch(const FInputActionValue& Value);
	void RightPunch(const FInputActionValue& Value);
	void ToggleInventory();
	UFUNCTION()
	void TakePlayerDamage(float Damage);

protected:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* LeftPunchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* RightPunchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* IMC_BaseCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_LeftPunch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_RightPunch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Inventory;


	// 쿨타임 상태
	bool bCanUseLeftPunch = true;
	bool bCanUseRightPunch = true;

	// 쿨타임 시간
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float LeftPunchCooldown = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float RightPunchCooldown = 5.0f;

	// 타이머 핸들
	FTimerHandle LeftPunchCooldownTimer;
	FTimerHandle RightPunchCooldownTimer;

	// 쿨타임 리셋 함수
	void ResetLeftPunchCooldown();
	void ResetRightPunchCooldown();

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;
	UUserWidget* InventoryWidget;
	bool bInventoryOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP;

};
