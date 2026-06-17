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
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "BaseCharacter.generated.h"

class USkillTreeWidget;
class ALantern;
class UPlayerHUDWidget;
class AArrowProjectile;
class APortal;
class AAltar;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

	UFUNCTION(Server, Reliable)
	void ServerUseQ();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayQ();

	UFUNCTION(Server, Reliable)
	void ServerUseW();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayW();

	UFUNCTION(Server, Reliable)
	void ServerUseE();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayE();

	void ExecuteE();

	UFUNCTION(Server, Reliable)
	void ServerUseR();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayR();

	void ExecuteR();

	UFUNCTION(Server, Reliable)
	void ServerAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack();

	void ExecuteAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastQImpact(FVector Location);

	UFUNCTION(Server, Unreliable)
	void ServerRotate(FRotator NewRotation);

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

	void Attack(const FInputActionValue& Value);
	void Q(const FInputActionValue& Value);
	void UseQ();
	void W(const FInputActionValue& Value);
	void E(const FInputActionValue& Value);
	void R(const FInputActionValue& Value);
	void ToggleInventory();
	UFUNCTION()
	void TakePlayerDamage(float Damage);

	void ExecuteQDamage();

	// 스킬 사용 중인지
	bool bIsUsingSkill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	ECharacterType CharacterType;

	// 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float QMultiplier = 1.0f;

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

	bool bRBonusDamage = false;

	FTimerHandle AttackSpeedBuffHandle;

	float DefaultAttackSpeed = 1.0f;

	float BuffAttackSpeed = 1.5f;

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

	void RestoreSkillUpgrades();

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

	UPROPERTY()
	AAltar* NearbyAltar = nullptr;

	void SetNearbyAltar(AAltar* Altar);

	UFUNCTION(Server, Reliable)
	void ServerInteractAltar();

	UFUNCTION(Server, Reliable)
	void ServerPickupLantern(ALantern* Lantern);

	UFUNCTION(Server, Reliable)
	void ServerUseSlot1();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayLanternMontage(bool bEquip);

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bHasLantern = false;

	UPROPERTY()
	ALantern* NearbyLantern = nullptr;

	void SetNearbyLantern(ALantern* Lantern);

	void Interact(const FInputActionValue& Value);

	UPROPERTY(ReplicatedUsing = OnRep_LanternEquipped, BlueprintReadOnly)
	bool bLanternEquipped = false;

	UFUNCTION()
	void OnRep_LanternEquipped();

	void RefreshLanternState();

	UPROPERTY(BlueprintReadOnly)
	bool bHasPrism = false;

	UPROPERTY(BlueprintReadOnly)
	bool bPrismEquipped = false;

	UPROPERTY()
	class ADungeonPrism* NearbyPrism = nullptr;

	UPROPERTY()
	APortal* NearbyPortal = nullptr;

	void SetNearbyPortal(APortal* Portal);

	void UseSlot1(const FInputActionValue& Value);
	void SetNearbyPrism(class ADungeonPrism* Prism);

	void UseSlot2(const FInputActionValue& Value);

	void UseSlot3(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EquippedLanternMesh;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* LanternLight;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EquippedPrismMesh;

	UFUNCTION(BlueprintCallable)
	void OnLanternEquipped();

	UPROPERTY(BlueprintReadOnly)
	bool bIsEquippingLantern = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsEquippingPrism = false;

	UFUNCTION(BlueprintCallable)
	void OnLanternUnequipped();

	UFUNCTION(BlueprintCallable)
	void OnLanternUnequipFinished();

	UFUNCTION(BlueprintCallable)
	void OnPrismEquipped();

	UFUNCTION(BlueprintCallable)
	void OnPrismUnequipped();

	UFUNCTION(BlueprintCallable)
	void OnPrismUnequipFinished();

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bLanternPoseActive = false;

	UPROPERTY(BlueprintReadOnly)
	bool bPrismPoseActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	UPROPERTY()
	UPlayerHUDWidget* HUDWidget;

	// 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP;

	UFUNCTION()
	void OnRep_CurrentHP();

	void HealPlayer(float Amount);

	virtual void GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 보호막
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float ShieldHP = 0.f;

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

	UPROPERTY(BlueprintReadOnly)
	int32 PotionCount = 0;

	UPROPERTY(EditAnywhere, Category = "Item")
	UTexture2D* PrismIcon;

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

	UFUNCTION(BlueprintCallable)
	void SpawnArrow();

	UFUNCTION(BlueprintCallable)
	void SpawnQArrow();

	UFUNCTION(BlueprintCallable)
	void SpawnRArrow();

	UFUNCTION(BlueprintCallable)
	void SpawnEArrow();

	UFUNCTION(BlueprintCallable)
	bool IsDead() const;

	UPROPERTY(BlueprintReadOnly)
	bool bDarknessDebuff = false;

	void EndAttackSpeedBuff();

	void RotateToMouseCursor();

	UPROPERTY(BlueprintReadOnly)
	bool bIsAttacking = false;

	bool CanUseCombatAction() const;

	// 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly)
	int32 Coin = 0;

protected:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* PrismEquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* PrismUnequipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* IMC_BaseCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Attack;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Slot2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Slot3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* IA_Slot4;

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
	float ERadius = 700.f;

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
	UNiagaraSystem* QImpactEffect;

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* WSkillEffect;
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* ESkillEffect;
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* RSkillEffect;

	UPROPERTY()
	UNiagaraComponent* WAreaComponent;

	UFUNCTION()
	void EndArcherWBuff();

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
