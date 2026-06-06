// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "SkillUpgradeData.h"
#include "CharacterType.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "BaseCharacter.generated.h"

class USkillTreeWidget;
class ALantern;
class UPlayerHUDWidget;
class AArrowProjectile;

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

	void EquipWeapon(TSubclassOf<AActor> WeaponClass, FName SocketName, AActor*& OutWeapon);

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

	// 스킬 사용 중인지
	bool bIsUsingSkill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	ECharacterType CharacterType;

	// 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float QMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float WMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float EMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float RMultiplier = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float HealAmount = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float RHealAmount = 0.2f;	

	// 현재 보유 스킬 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 SkillPoints = 0;

	// 스킬 레벨 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TMap<ESkillType, int32> SkillLevels;

	// 스킬 업그레이드
	UFUNCTION(BlueprintCallable)
	bool UpgradeSkill(FSkillUpgradeData UpgradeData);

	// 실제 능력치 적용
	UFUNCTION(BlueprintCallable)
	void ApplySkillUpgrade(FSkillUpgradeData UpgradeData);

	void ToggleSkillTree();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* QSkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* WSkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* ESkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* RSkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UTexture2D* PortraitTexture;

	UPROPERTY(BlueprintReadOnly)
	bool bHasLantern = false;

	UPROPERTY()
	ALantern* NearbyLantern = nullptr;

	void SetNearbyLantern(ALantern* Lantern);

	void Interact(const FInputActionValue& Value);

	UPROPERTY(BlueprintReadOnly)
	bool bLanternEquipped = false;

	void UseSlot1(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EquippedLanternMesh;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* LanternLight;

	UFUNCTION(BlueprintCallable)
	void OnLanternEquipped();

	UPROPERTY(BlueprintReadOnly)
	bool bIsEquippingLantern = false;

	UFUNCTION(BlueprintCallable)
	void OnLanternUnequipped();

	UFUNCTION(BlueprintCallable)
	void OnLanternUnequipFinished();

	UPROPERTY(BlueprintReadOnly)
	bool bLanternPoseActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	UPROPERTY()
	UPlayerHUDWidget* HUDWidget;

	// 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP;

	// 쿨타임 시간
	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float QCooldown = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float WCooldown = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float ECooldown = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Cooldown")
	float RCooldown = 20.0f;

	UPROPERTY(BlueprintReadOnly)
	float QRemainingCooldown = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float WRemainingCooldown = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ERemainingCooldown = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RRemainingCooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UTexture2D* Slot1Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UTexture2D* Slot2Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UTexture2D* Slot3Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UTexture2D* Slot4Icon;

	UPROPERTY(EditAnywhere, Category = "Item")
	UTexture2D* LanternIcon;

	UPROPERTY(EditAnywhere, Category = "Item")
	UTexture2D* PotionIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* EmptySlotIcon;

	UFUNCTION(BlueprintCallable)
	void EnableWeaponCollision();

	UFUNCTION(BlueprintCallable)
	void DisableWeaponCollision();

	float GetAttackPower() const
	{
		return AttackPower;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AArrowProjectile> ArrowClass;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lantern")
	UAnimMontage* LanternEquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lantern")
	UAnimMontage* LanternUnequipMontage;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_SkillTree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Interact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Slot1;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<USkillTreeWidget> SkillTreeWidgetClass;

	UPROPERTY()
	USkillTreeWidget* SkillTreeWidget;

	bool bSkillTreeOpen = false;

	// 쿨타임 상태
	bool bCanUseQ = true;
	bool bCanUseW = true;
	bool bCanUseE = true;
	bool bCanUseR = true;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float QDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float WDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float EDamage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float RDamage = 50.f;

	// 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float QRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float WRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float ERadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float RRadius = 200.f;

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

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;
	UUserWidget* InventoryWidget;
	bool bInventoryOpen = false;

	// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackPower = 100.f;

	// 방어율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DefenseRate = 0.f;

	// 공격속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackSpeed = 1.f;

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
	
	//VFX
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* QSkillEffect;
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* WSkillEffect;
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* ESkillEffect;
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* RSkillEffect;


	// Weapon & Socket
	UPROPERTY(EditDefaultsOnly, Category = "Socket")
	FName RightHandSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Socket")
	FName LeftHandSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> RightHandWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> LeftHandWeaponClass;

	UPROPERTY()
	AActor* RightHandWeapon;

	UPROPERTY()
	AActor* LeftHandWeapon;
};
