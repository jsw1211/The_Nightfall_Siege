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
#include "Components/SphereComponent.h"
#include "BaseCharacter.generated.h"

class USkillTreeWidget;
class ALantern;
class UPlayerHUDWidget;
class AArrowProjectile;
class APortal;
class AAltar;
class ADungeonPortal;
class ADragonBoss;
class AQuestGiver;
class UQuestWidget;
class UQuestDialogueWidget;
class UAltarProgressWidget;
class UAnimSequenceBase;

UENUM(BlueprintType)
enum class EShopItemType : uint8
{
	HealPotion UMETA(DisplayName = "Heal Potion"),
	HPPotion UMETA(DisplayName = "HP Potion"),
	AttackPotion UMETA(DisplayName = "Attack Potion")
};

// One entry is added for every successful purchase.  Entries are deliberately
// not stacked: the inventory can therefore display its slots in purchase order.
USTRUCT(BlueprintType)
struct FShopInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EShopItemType ItemType = EShopItemType::HealPotion;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 1;
};

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

	UFUNCTION(Client, Reliable)
	void ClientStartSkillCooldown(ESkillType SkillType, float Duration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayR();

	void ExecuteR();
	void ApplyPaladinRHeal();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayRHealEffect(FVector Location);

	UFUNCTION(Server, Reliable)
	void ServerAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack();

	void ExecuteAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastQImpact(FVector Location);

	UFUNCTION(Server, Reliable)
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

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void ToggleShop();

	// Called immediately before portal travel so no newly spawned pawn restores
	// the lantern in its hand.
	void PrepareForPortalTravel();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestBuyPotion();

	// Call these from WBP_Shop's three existing buy buttons.  The resulting
	// item is appended to PurchasedItems, never inserted or sorted.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void BuyShopItem(EShopItemType ItemType);

	UFUNCTION()
	void BuyHealPotionFromShop();

	UFUNCTION()
	void BuyHPPotionFromShop();

	UFUNCTION()
	void BuyAttackPotionFromShop();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FShopInventoryItem>& GetPurchasedItems() const { return PurchasedItems; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetPurchasedItemCount() const { return PurchasedItems.Num(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetPurchasedItem(int32 Index, FShopInventoryItem& Item) const;

	// Called by an inventory slot after a successful drag-and-drop operation.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void MovePurchasedItem(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AssignPurchasedItemToSlot4(int32 ItemIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UsePurchasedItemAtIndex(int32 ItemIndex);

	UFUNCTION(Server, Reliable)
	void ServerBuyPotion();

	UFUNCTION(Server, Reliable)
	void ServerBuyShopItem(EShopItemType ItemType);

	UFUNCTION(Server, Reliable)
	void ServerMovePurchasedItem(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void ServerAssignPurchasedItemToSlot4(int32 ItemIndex);

	UFUNCTION(Server, Reliable)
	void ServerUsePurchasedItem(int32 ItemIndex);
	UFUNCTION()
	void TakePlayerDamage(float Damage);

	void ExecuteQDamage();

	// 스킬 사용 중인지
	bool bIsUsingSkill = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Character")
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
	FTimerHandle PaladinWBuffHandle;

	float DefaultAttackSpeed = 1.0f;

	float BuffAttackSpeed = 1.5f;
	float PaladinWDefenseRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (ClampMin = "0"))
	float WBuffDuration = 5.0f;

	// 현재 보유 스킬 포인트
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 SkillPoints = 0;

	// 스킬 레벨 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TMap<ESkillType, int32> SkillLevels;

	// 스킬 업그레이드
	UFUNCTION(BlueprintCallable)
	bool UpgradeSkill(FSkillUpgradeData UpgradeData);

	UFUNCTION(Server, Reliable)
	void ServerUpgradeSkill(FSkillUpgradeData UpgradeData);

	UFUNCTION(Client, Reliable)
	void ClientConfirmSkillUpgrade(bool bSuccess, ESkillType SkillType, int32 NewLevel, int32 NewSkillPoints);

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

	UFUNCTION(Client, Reliable)
	void ClientShowAltarPlacementProgress(float Duration);

	UFUNCTION(Client, Reliable)
	void ClientHideAltarPlacementProgress();

	UFUNCTION(Server, Reliable)
	void ServerPickupLantern(ALantern* Lantern);

	UFUNCTION(Server, Reliable)
	void ServerUseSlot1();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayLanternMontage(bool bEquip);

	UFUNCTION(Server, Reliable)
	void ServerPickupPrism(ADungeonPrism* Prism);

	UFUNCTION(Server, Reliable)
	void ServerUseSlot3();

	UFUNCTION(Server, Reliable)
	void ServerRequestGroupPrismCleanse();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPrismMontage(bool bEquip);

	UFUNCTION(Server, Reliable)
	void ServerInteractPortal(APortal* Portal);

	UFUNCTION(Server, Reliable)
	void ServerInteractDungeonPortal();

    UFUNCTION(Server, Reliable)
    void ServerInteractQuestGiver(AQuestGiver* QuestGiver);

    UFUNCTION(Server, Reliable)
    void ServerSubmitQuestDecision(AQuestGiver* QuestGiver, bool bAccepted);

    UFUNCTION(Client, Reliable)
    void ClientOpenQuestDialogue(AQuestGiver* QuestGiver, const TArray<FText>& DialogueLines, const FText& SpeakerName, bool bRequiresQuestDecision);

    UFUNCTION(Client, Reliable)
    void ClientFinishQuestDialogue(bool bAccepted, const FText& ResultMessage);

    void CloseQuestDialogue();

    // Server-only reward helper. The lantern occupies item slot 1.
    void GrantQuestLantern();

    UFUNCTION(Client, Reliable)
    void ClientShowQuestMessage(const FString& Message);

    void InteractWithQuestGiver();

    UPROPERTY()
    AQuestGiver* NearbyQuestGiver = nullptr;

    void SetNearbyQuestGiver(AQuestGiver* QuestGiver);

	UPROPERTY(ReplicatedUsing = OnRep_HasLantern, BlueprintReadOnly)
	bool bHasLantern = false;

	UFUNCTION()
	void OnRep_HasLantern();

	UPROPERTY()
	ALantern* NearbyLantern = nullptr;

	void SetNearbyLantern(ALantern* Lantern);

	void Interact(const FInputActionValue& Value);

	UPROPERTY(ReplicatedUsing = OnRep_LanternEquipped, BlueprintReadOnly)
	bool bLanternEquipped = false;

	UFUNCTION()
	void OnRep_LanternEquipped();

	void RefreshLanternState();

	UPROPERTY(ReplicatedUsing = OnRep_HasPrism, BlueprintReadOnly)
	bool bHasPrism = false;
	
	UFUNCTION()
	void OnRep_HasPrism();

	UPROPERTY(ReplicatedUsing = OnRep_PrismEquipped, BlueprintReadOnly)
	bool bPrismEquipped = false;

	UPROPERTY()
	class ADungeonPrism* NearbyPrism = nullptr;

	UPROPERTY()
	APortal* NearbyPortal = nullptr;

	void SetNearbyPortal(APortal* Portal);

	UPROPERTY()
	ADungeonPortal* NearbyDungeonPortal = nullptr;

	void SetNearbyDungeonPortal(ADungeonPortal* Portal);

	void UseSlot1(const FInputActionValue& Value);
	void SetNearbyPrism(class ADungeonPrism* Prism);

	void UseSlot2(const FInputActionValue& Value);

	void UseSlot3(const FInputActionValue& Value);
	void UseSlot4(const FInputActionValue& Value);
	UFUNCTION(Server, Reliable)
	void ServerUseSlot4();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EquippedLanternMesh;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* LanternLight;

	// Ground ring that makes the lantern's gameplay-safe boundary readable.
	UPROPERTY(VisibleAnywhere, Category = "Lantern")
	class UDecalComponent* LanternSafeZoneDecal;

	// Local-only guide effect. It is rotated toward the active dungeon portal
	// while the lantern is equipped in the village.
	UPROPERTY(VisibleAnywhere, Category = "Lantern")
	UNiagaraComponent* LanternDirectionEffectComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Lantern")
	UNiagaraSystem* LanternDirectionEffect;

	// NS_Lantern_Direction is authored along its local Y axis, while Unreal's
	// direction vectors use X as forward.
	UPROPERTY(EditDefaultsOnly, Category = "Lantern")
	FRotator LanternDirectionRotationOffset = FRotator(90.f, 0.f, 0.f);

	float LanternDirectionUpdateElapsed = 0.f;

	// Local visual gate: the direction trail must not appear until the lantern
	// equip montage has completed.
	bool bLanternGuideReady = false;

	void UpdateLanternDirectionEffect(float DeltaTime);

	UPROPERTY(VisibleAnywhere)
	USphereComponent* LanternLightSphere;

	UFUNCTION()
	void OnLanternLightBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnLanternLightEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	FTimerHandle DarknessTimer;

	void CheckDarknessDamage();

	UPROPERTY()
	bool bInsideLanternLight = false;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* EquippedPrismMesh;

	UFUNCTION(BlueprintCallable)
	void OnLanternEquipped();

	// The lantern is returned directly from a cleared altar, so there is no
	// equip montage to wait for before resuming local navigation guidance.
	void ResumeLanternGuidanceAfterAltar();

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

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bPrismPoseActive = false;

	UFUNCTION()
	void OnRep_PrismEquipped();

	void RefreshPrismState();

	virtual void OnRep_PlayerState() override;

	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	UPROPERTY()
	UPlayerHUDWidget* HUDWidget;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UQuestWidget> QuestWidgetClass;

    UPROPERTY()
    UQuestWidget* QuestWidget;

    // Loads WBP_Quest even when a child character Blueprint has an older,
    // serialized default value for QuestWidgetClass.
    void EnsureQuestWidget();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UQuestDialogueWidget> QuestDialogueWidgetClass;

    UPROPERTY()
    UQuestDialogueWidget* QuestDialogueWidget = nullptr;

    void EnsureQuestDialogueWidget();

	// 체력
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP;

	UFUNCTION()
	void OnRep_CurrentHP();

	void HealPlayer(float Amount);

	FTimerHandle HealOverTimeHandle;
	FTimerHandle PaladinRHealTimer;

	int32 HealTickCount = 0;

	UFUNCTION()
	void HealOverTimeTick();

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

	// Unmodified values used to apply Warrior upgrades absolutely rather than
	// accumulating the same upgrade again after travel/load.
	float BaseAttackPower = 100.0f;

	float WarriorRDamageBonus = 0.0f;
	float WarriorWCooldownReduction = 0.0f;
	FTimerHandle WarriorRBuffHandle;

	void EndWarriorRBuff();
	void EndPaladinWBuff();

	UPROPERTY(Replicated, BlueprintReadOnly)
	float QRemainingCooldown = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float WRemainingCooldown = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float ERemainingCooldown = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly)
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

	UPROPERTY(ReplicatedUsing = OnRep_PotionCount, BlueprintReadOnly)
	int32 PotionCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion")
	float PotionHealPercent = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion")
	float PotionUseDuration = 1.25f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Potion")
	bool bIsUsingPotion = false;

	UPROPERTY(EditAnywhere, Category = "Potion")
	TObjectPtr<UAnimSequenceBase> PaladinPotionAnimation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Potion")
	TObjectPtr<UAnimSequenceBase> ArcherPotionAnimation = nullptr;

	UPROPERTY(EditAnywhere, Category = "Potion")
	TObjectPtr<UAnimSequenceBase> WarriorPotionAnimation = nullptr;

	FTimerHandle PotionUseTimer;
	TObjectPtr<UAnimMontage> ActivePotionMontage = nullptr;
	TObjectPtr<UNiagaraComponent> PotionHealEffectComponent = nullptr;

	UFUNCTION(Server, Reliable)
	void ServerCancelPotionUse();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartPotionUse();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCancelPotionUse();

	void FinishPotionUse();
	void CancelPotionUse();
	UAnimSequenceBase* GetPotionAnimation() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 PotionPrice = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 HPPotionPrice = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 AttackPotionPrice = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UTexture2D> HPPotionIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UTexture2D> AttackPotionIcon = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PurchasedItems, BlueprintReadOnly, Category = "Inventory")
	TArray<FShopInventoryItem> PurchasedItems;

	UPROPERTY(ReplicatedUsing = OnRep_Slot4PurchasedItemIndex, BlueprintReadOnly, Category = "Inventory")
	int32 Slot4PurchasedItemIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_Slot4PurchasedItemIndex();

	UFUNCTION()
	void OnRep_PurchasedItems();

	void RefreshInventoryWidget();
	void BindShopButtons();

	UFUNCTION()
	void OnRep_PotionCount();

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
	void ApplyArcherERainDamage();
	void EndArcherERainDamage();

	UFUNCTION(BlueprintCallable)
	bool IsDead() const;

	UPROPERTY(ReplicatedUsing = OnRep_DarknessDebuff, BlueprintReadOnly)
	bool bDarknessDebuff = false;

	UFUNCTION()
	void OnRep_DarknessDebuff();

	void EndAttackSpeedBuff();

	void RotateToMouseCursor();

	UPROPERTY(BlueprintReadOnly)
	bool bIsAttacking = false;

	bool CanUseCombatAction() const;

	// 상태
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(ReplicatedUsing = OnRep_Coin, BlueprintReadOnly)
	int32 Coin = 15;

	UFUNCTION()
	void OnRep_Coin();

	UFUNCTION(Server, Reliable)
	void ServerPickupCoin(class ACoin* CoinActor);

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
	UPROPERTY(Replicated)
	bool bCanUseQ = true;

	UPROPERTY(Replicated)
	bool bCanUseW = true;

	UPROPERTY(Replicated)
	bool bCanUseE = true;

	UPROPERTY(Replicated)
	bool bCanUseR = true;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float QDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float WDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float EDamage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archer|E")
	float ArcherERainDamagePerSecond = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archer|E")
	float ArcherERainDuration = 3.f;

	FTimerHandle ArcherERainDamageTimer;
	FTimerHandle ArcherERainEndTimer;
	FVector ArcherERainCenter = FVector::ZeroVector;
	int32 NextArcherQVolleyId = 0;

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
	float WarriorERadius = 400.f;

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

	UPROPERTY(EditAnywhere, Category = "Shop")
	TSubclassOf<class UUserWidget> ShopWidgetClass;

	UPROPERTY()
	UUserWidget* ShopWidget;

	bool bShopOpen = false;

	UFUNCTION(Server, Reliable)
	void ServerUsePotion();

	UPROPERTY()
	UAltarProgressWidget* AltarProgressWidget = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Altar")
	bool bIsPlacingLantern = false;

	FTimerHandle AltarPlacementTimer;
	TWeakObjectPtr<AAltar> AltarBeingPlaced;
	void FinishAltarPlacement();

	// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackPower = 100.f;

	// 방어율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DefenseRate = 0.f;

	// 공격속도
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
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

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* HealEffect;

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

	////////////////////////////////////////////디버그
	UFUNCTION(Server, Reliable)
	void ServerDebugBossPattern(uint8 PatternIndex);

	// Development-only shortcut: teleports the player beside the active dungeon portal.
	UFUNCTION(Server, Reliable)
	void ServerDebugTeleportToDungeonPortal();

	UFUNCTION(Server, Reliable)
	void ServerDebugCompleteRaid();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Debug1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Debug2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Debug3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Debug4;

	void DebugBossPattern1();
	void DebugBossPattern2();
	void DebugBossPattern3();
	void DebugBossPattern4();
	void DebugBossCenterMechanic();
	void DebugTeleportToDungeonPortal();
	void DebugCompleteRaid();
};
