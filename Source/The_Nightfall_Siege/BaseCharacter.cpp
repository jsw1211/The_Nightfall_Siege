// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Monster.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "BaseController.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "SkillTreeWidget.h"
#include "Lantern.h"
#include "PlayerHUDWidget.h"
#include "WeaponBase.h"
#include "ArrowProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DungeonPrism.h"
#include "DragonBoss.h"
#include "TheNightfallSiegeInstance.h"
#include "DrawDebugHelpers.h"
#include "Portal.h"
#include "Net/UnrealNetwork.h"
#include "Altar.h"
#include "DungeonPortal.h"
#include "QuestGiver.h"
#include "QuestWidget.h"
#include "AltarProgressWidget.h"
#include "Animation/AnimSequenceBase.h"
#include "Coin.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "InventoryItemSlotWidget.h"
#include "BasePlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"


// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

    // WBP_Shop is the authored shop UI.  Keeping the assignment here means
    // every BaseCharacter-derived Blueprint opens it without per-BP setup.
    static ConstructorHelpers::FClassFinder<UUserWidget> ShopWidgetBP(TEXT("/Game/BP/WBP_Shop"));
    if (ShopWidgetBP.Succeeded())
    {
        ShopWidgetClass = ShopWidgetBP.Class;
    }

    static ConstructorHelpers::FClassFinder<UQuestWidget> QuestWidgetBP(TEXT("/Game/BP_Character/WBP_Quest"));
    if (QuestWidgetBP.Succeeded())
    {
        QuestWidgetClass = QuestWidgetBP.Class;
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> HPPotionTexture(TEXT("/Game/Asset/UI/items/HP_Potion"));
    if (HPPotionTexture.Succeeded())
    {
        HPPotionIcon = HPPotionTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> AttackPotionTexture(TEXT("/Game/Asset/UI/items/ATK_Potion"));
    if (AttackPotionTexture.Succeeded())
    {
        AttackPotionIcon = AttackPotionTexture.Object;
    }

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    SpringArm->TargetArmLength = 1500.f;
    SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

    // ?? 이게 제일 중요
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritRoll = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->bUsePawnControlRotation = false;

    EquippedLanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedLanternMesh"));

    EquippedLanternMesh->SetupAttachment(GetMesh(), TEXT("LanternSocket"));

    EquippedLanternMesh->SetVisibility(false);

    EquippedPrismMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedPrismMesh"));

    EquippedPrismMesh->SetupAttachment(GetMesh(), TEXT("PrismSocket"));

    EquippedPrismMesh->SetVisibility(false);

    LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));

    LanternLight->SetupAttachment(EquippedLanternMesh);

    LanternDirectionEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(
        TEXT("LanternDirectionEffectComponent"));
    // The guide must originate from the held lantern, not from the pawn root.
    LanternDirectionEffectComponent->SetupAttachment(EquippedLanternMesh);
    LanternDirectionEffectComponent->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
    LanternDirectionEffectComponent->SetAutoActivate(false);
    LanternDirectionEffectComponent->SetVisibility(false);

    LanternLightSphere =
        CreateDefaultSubobject<USphereComponent>(
            TEXT("LanternLightSphere"));

    LanternLightSphere->SetupAttachment(
        EquippedLanternMesh);

    LanternLightSphere->SetSphereRadius(1800.f);

    LanternLightSphere->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    LanternLightSphere->SetCollisionResponseToAllChannels(
        ECR_Ignore);

    LanternLightSphere->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap);

    LanternLightSphere->SetGenerateOverlapEvents(true);

    LanternLightSphere->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);

    LanternLight->SetVisibility(false);

    // A warm local pool of light keeps nearby enemies readable in the lava
    // dungeon while the inverse-square falloff preserves the distant darkness.
    LanternLight->SetIntensity(6500.f);

    LanternLight->SetAttenuationRadius(1800.f);

    LanternLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.18f));

    UE_LOG(LogTemp, Warning, TEXT("%s"),
        *LanternLight->GetLightColor().ToString());

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealEffectAsset(
        TEXT("/Game/Effects/1_Heal/NS_Heal.NS_Heal"));

    if (HealEffectAsset.Succeeded())
    {
        HealEffect = HealEffectAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PaladinPotionAsset(TEXT("/Game/Asset/paladin/Animation/paladin_potion"));
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> ArcherPotionAsset(TEXT("/Game/Asset/archer/Animation/archer_potion"));
    static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> WarriorPotionAsset(TEXT("/Game/Asset/Warrior/Animation/warrior_potion"));
    PaladinPotionAnimation = PaladinPotionAsset.Object;
    ArcherPotionAnimation = ArcherPotionAsset.Object;
    WarriorPotionAnimation = WarriorPotionAsset.Object;

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LanternDirectionEffectAsset(
        TEXT("/Game/Effects/91_Lantern_Directions/NS_Lantern_Direction.NS_Lantern_Direction"));

    if (LanternDirectionEffectAsset.Succeeded())
    {
        LanternDirectionEffect = LanternDirectionEffectAsset.Object;
        LanternDirectionEffectComponent->SetAsset(LanternDirectionEffect);
    }

}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// The PlayerState is the server-authoritative character selection. The
	// visual pawn class can be correct while this property still holds its
	// blueprint/default value, which applies upgrades for the wrong class.

    LanternLightSphere->OnComponentBeginOverlap.AddDynamic(
        this,
        &ABaseCharacter::OnLanternLightBegin);

    LanternLightSphere->OnComponentEndOverlap.AddDynamic(
        this,
        &ABaseCharacter::OnLanternLightEnd);

	if (ABasePlayerState* InitialPlayerState = GetPlayerState<ABasePlayerState>())
	{
		CharacterType = InitialPlayerState->SelectedCharacter;
	}

    switch (CharacterType)
    {
    case ECharacterType::Paladin:

        MaxHP = 500.f;
        AttackPower = 100.f;

        // Q
        QMultiplier = 1.0f;
        QCooldown = 5.f;
        QRadius = 200.f;

        // W
        DefenseRate = 0.f;

        // E
        HealAmount = 0.f;

        // R
        RHealAmount = 0.f;

        break;

    case ECharacterType::Archer:

        MaxHP = 300.f;
        AttackPower = 200.f;

        QMultiplier = 1.5f;
        EMultiplier = 1.5f;
        RMultiplier = 3.0f;

        AttackSpeed = 1.0f;

        DefaultAttackSpeed = 1.0f;

        break;

    case ECharacterType::Warrior:

        MaxHP = 400.f;
        AttackPower = 300.f;

		QMultiplier = 1.0f;
		EMultiplier = 1.0f;

        break;
    }

    // PlayerState stays with the player when the pawn is recreated for the
    // next map, so it is the authoritative home for permanent shop bonuses.
    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>(); PS && PS->bHasShopStatBonuses)
    {
        MaxHP = PS->SavedMaxHP;
        AttackPower = PS->SavedAttackPower;
    }

    CurrentHP = MaxHP;

	BaseAttackPower = AttackPower;
	
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->AddMappingContext(IMC_BaseCharacter, 0);
            }
        }
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->OnMontageEnded.AddDynamic(this, &ABaseCharacter::OnMontageEnded);
    }

    EquipWeapon(RightHandWeaponClass, RightHandSocketName, RightHandWeapon);
    EquipWeapon(LeftHandWeaponClass, LeftHandSocketName, LeftHandWeapon);

    FString MapName = GetWorld()->GetMapName();

    if (HUDWidgetClass)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());

        if (PC && PC->IsLocalController() && !HUDWidget)
        {
            HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, HUDWidgetClass);

            if (HUDWidget)
            {
                HUDWidget->AddToViewport();
            }
        }
    }

    EnsureQuestWidget();

    Slot1Icon = EmptySlotIcon;
    Slot2Icon = EmptySlotIcon;
    Slot3Icon = EmptySlotIcon;
    Slot4Icon = EmptySlotIcon;

    if (HasAuthority())
    {
        PotionCount = 5;
    }

    OnRep_PotionCount();

    UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance());

    if (GI)
    {
        SkillPoints = GI->SkillPoints;
        SkillLevels = GI->SkillLevels;
    }

	// Every skill always starts at level 1, including sessions where no saved
	// GameInstance entry exists yet.
	SkillLevels.FindOrAdd(ESkillType::Q) = FMath::Clamp(SkillLevels.FindRef(ESkillType::Q), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::W) = FMath::Clamp(SkillLevels.FindRef(ESkillType::W), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::E) = FMath::Clamp(SkillLevels.FindRef(ESkillType::E), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::R) = FMath::Clamp(SkillLevels.FindRef(ESkillType::R), 1, 4);

	RestoreSkillUpgrades();

    ABasePlayerState* PS = GetPlayerState<ABasePlayerState>();

    if (PS)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("BeginPlay PS Lantern=%d CharacterBefore=%d"),
            PS->bHasLantern,
            bHasLantern);

        bHasLantern = PS->bHasLantern;
        bLanternEquipped = PS->bLanternEquipped;

        bHasPrism = PS->bHasPrism;
        bPrismEquipped = PS->bPrismEquipped;

        UE_LOG(LogTemp, Warning,
            TEXT("CharacterAfter Lantern=%d"),
            bHasLantern);
    }

    if (bHasLantern)
    {
        Slot1Icon = LanternIcon;
    }

    if (bHasPrism)
    {
        Slot3Icon = PrismIcon;
    }

    if (bLanternEquipped)
    {
        OnRep_LanternEquipped();
    }

    if (bPrismEquipped)
    {
        OnRep_PrismEquipped();
    }

    if (HasAuthority() && bHasLantern)
    {
        TArray<AActor*> Lanterns;

        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ALantern::StaticClass(),
            Lanterns);

        for (AActor* Actor : Lanterns)
        {
            Actor->Destroy();
        }

        UE_LOG(LogTemp, Warning,
            TEXT("Destroyed World Lantern"));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Authority : %d"),
        HasAuthority());

    if (HasAuthority() &&
        GetWorld()->GetMapName().Contains(TEXT("Village")))
    {
        GetWorldTimerManager().SetTimer(
            DarknessTimer,
            this,
            &ABaseCharacter::CheckDarknessDamage,
            1.f,
            true);
    }
}

void ABaseCharacter::Die()
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;
    CurrentHP = 0.f;

    ForceNetUpdate();

    GetCharacterMovement()->DisableMovement();

    ABaseController* PC = Cast<ABaseController>(GetController());
    if (PC)
    {
        DisableInput(PC);
    }

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 이거 추가해야 함
    PlayAnimMontage(DeathMontage);

}

void ABaseCharacter::OnRep_IsDead()
{
    if (!bIsDead)
    {
        return;
    }

    GetCharacterMovement()->DisableMovement();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->StopMovement();
        DisableInput(PC);
    }
}

void ABaseCharacter::PlayHit()
{
    bIsHit = true;

    // 0.3초 후 자동 해제
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            bIsHit = false;
        });
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// PIE can recapture the mouse after a click.  Keep the cursor visible for
	// as long as either interactive UI remains on screen.
	if ((bInventoryOpen || bShopOpen) && IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetShowMouseCursor(true);
		}
	}

    UpdateLanternDirectionEffect(DeltaTime);

    QRemainingCooldown = FMath::Max(0.f, QRemainingCooldown - DeltaTime);

    WRemainingCooldown = FMath::Max(0.f, WRemainingCooldown - DeltaTime);

    ERemainingCooldown = FMath::Max(0.f, ERemainingCooldown - DeltaTime);

    RRemainingCooldown = FMath::Max(0.f, RRemainingCooldown - DeltaTime);

}

void ABaseCharacter::UpdateLanternDirectionEffect(float DeltaTime)
{
    if (!LanternDirectionEffectComponent)
    {
        return;
    }

    // Directional guidance is personal UI-like feedback, so it is shown only
    // to the owning player and only before entering the dungeon.
    const bool bCanGuide = IsLocallyControlled()
        && bLanternEquipped
        && GetWorld()
        && GetWorld()->GetMapName().Contains(TEXT("Village_Forest"));

    if (!bCanGuide)
    {
        LanternDirectionEffectComponent->Deactivate();
        LanternDirectionEffectComponent->SetVisibility(false);
        LanternDirectionUpdateElapsed = 0.f;
        return;
    }

    LanternDirectionUpdateElapsed += DeltaTime;
    if (LanternDirectionUpdateElapsed < 0.15f)
    {
        return;
    }
    LanternDirectionUpdateElapsed = 0.f;

    ADungeonPortal* ClosestPortal = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (TActorIterator<ADungeonPortal> It(GetWorld()); It; ++It)
    {
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestPortal = *It;
        }
    }

    if (!ClosestPortal)
    {
        LanternDirectionEffectComponent->Deactivate();
        LanternDirectionEffectComponent->SetVisibility(false);
        return;
    }

    // Calculate from the lantern itself so the visible trail points from the
    // held light directly to the placed/spawned BP_DungeonPortal actor.
    FVector Direction = ClosestPortal->GetActorLocation()
        - LanternDirectionEffectComponent->GetComponentLocation();
    Direction.Z = 0.f;
    if (Direction.IsNearlyZero())
    {
        return;
    }

    LanternDirectionEffectComponent->SetWorldRotation(
        Direction.Rotation() + LanternDirectionRotationOffset);
    LanternDirectionEffectComponent->SetVisibility(true);
    if (!LanternDirectionEffectComponent->IsActive())
    {
        LanternDirectionEffectComponent->Activate(true);
    }
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(IA_Attack, ETriggerEvent::Started, this, &ABaseCharacter::Attack);
        EnhancedInput->BindAction(IA_Q, ETriggerEvent::Started, this, &ABaseCharacter::Q);
        EnhancedInput->BindAction(IA_W, ETriggerEvent::Started, this, &ABaseCharacter::W);
        EnhancedInput->BindAction(IA_E, ETriggerEvent::Started, this, &ABaseCharacter::E);
        EnhancedInput->BindAction(IA_R, ETriggerEvent::Started, this, &ABaseCharacter::R);
        EnhancedInput->BindAction(IA_Inventory, ETriggerEvent::Started, this, &ABaseCharacter::ToggleInventory);
        EnhancedInput->BindAction(IA_SkillTree, ETriggerEvent::Started, this, &ABaseCharacter::ToggleSkillTree);
        EnhancedInput->BindAction(IA_Interact, ETriggerEvent::Started, this, &ABaseCharacter::Interact);
        EnhancedInput->BindAction(IA_Slot1, ETriggerEvent::Started, this, &ABaseCharacter::UseSlot1);
        EnhancedInput->BindAction(IA_Slot2, ETriggerEvent::Started, this, &ABaseCharacter::UseSlot2);
        EnhancedInput->BindAction(IA_Slot3, ETriggerEvent::Started, this, &ABaseCharacter::UseSlot3);
        EnhancedInput->BindAction(IA_Slot4, ETriggerEvent::Started, this, &ABaseCharacter::UseSlot4);

        EnhancedInput->BindAction(IA_Debug1, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern1);
        EnhancedInput->BindAction(IA_Debug2, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern2);
        EnhancedInput->BindAction(IA_Debug3, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern3);
        EnhancedInput->BindAction(IA_Debug4, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern4);
    }

    // The shop is deliberately bound directly so it works without requiring a
    // new Input Action asset to be configured in every character Blueprint.
    PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABaseCharacter::ToggleShop);
    PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ABaseCharacter::InteractWithQuestGiver);
#if !UE_BUILD_SHIPPING
    PlayerInputComponent->BindKey(EKeys::Equals, IE_Pressed, this, &ABaseCharacter::DebugTeleportToDungeonPortal);
#endif

    if (bIsDead) return; // 죽으면 입력 등록 안함
}

void ABaseCharacter::Attack(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (!CanUseCombatAction())
    {
        return;
    }

    RotateToMouseCursor();

    ServerAttack();
}

void ABaseCharacter::Q(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (!bCanUseQ)
        return;

    if (!CanUseCombatAction())
        return;

    RotateToMouseCursor();

    ServerUseQ();
}

void ABaseCharacter::W(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (!bCanUseW)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    RotateToMouseCursor();

    ServerUseW();
}

void ABaseCharacter::E(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (!bCanUseE)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    RotateToMouseCursor();

    ServerUseE();
}

void ABaseCharacter::R(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (!bCanUseR)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    RotateToMouseCursor();

    ServerUseR();
}

void ABaseCharacter::ResetQCooldown()
{
    bCanUseQ = true;
    QRemainingCooldown = 0.f;
    ForceNetUpdate();
}

void ABaseCharacter::ResetWCooldown()
{
    bCanUseW = true;
    WRemainingCooldown = 0.f;
    ForceNetUpdate();
}

void ABaseCharacter::ResetECooldown()
{
    bCanUseE = true;
    ERemainingCooldown = 0.f;
    ForceNetUpdate();
}

void ABaseCharacter::ResetRCooldown()
{
    bCanUseR = true;
    RRemainingCooldown = 0.f;
    ForceNetUpdate();
}

void ABaseCharacter::ToggleInventory()
{
    UE_LOG(LogTemp, Warning, TEXT("Inventory Toggle"));

    if (!InventoryWidgetClass) return;

    if (!bInventoryOpen)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        InventoryWidget = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);

        if (InventoryWidget)
        {
            InventoryWidget->AddToViewport();
            RefreshInventoryWidget();

            if (PC)
            {
                FInputModeGameAndUI InputMode;
                InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                InputMode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(InputMode);
                PC->SetShowMouseCursor(true);
            }
        }

        bInventoryOpen = true;
    }
    else
    {
        if (InventoryWidget)
        {
            InventoryWidget->RemoveFromParent();
        }

        if (!bShopOpen)
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                PC->SetInputMode(FInputModeGameOnly());
                PC->SetShowMouseCursor(false);
            }
        }

        bInventoryOpen = false;
    }
}

void ABaseCharacter::ToggleShop()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    if (!bShopOpen)
    {
        if (!ShopWidget)
        {
            TSubclassOf<UUserWidget> WidgetClass = ShopWidgetClass;
            if (!WidgetClass)
            {
                return;
            }

            ShopWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
        }

        if (!ShopWidget)
        {
            return;
        }

        // WBP_Shop is a mouse-driven widget by default.  Enable focus before
        // assigning it so PIE does not emit the "does not support focus" warning.
        ShopWidget->SetIsFocusable(true);
        ShopWidget->AddToViewport(20);
        BindShopButtons();
        ShopWidget->SetKeyboardFocus();

        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(ShopWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        UCharacterMovementComponent* Movement = GetCharacterMovement();
        Movement->StopMovementImmediately();
        // Enhanced Input may bypass controller-level ignore flags, so disable
        // the movement component as the authoritative movement lock as well.
        Movement->DisableMovement();
        bShopOpen = true;
        return;
    }

    if (ShopWidget)
    {
        ShopWidget->RemoveFromParent();
    }

    PC->SetInputMode(FInputModeGameOnly());
    PC->SetShowMouseCursor(false);
    PC->SetIgnoreMoveInput(false);
    PC->SetIgnoreLookInput(false);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    bShopOpen = false;
}

void ABaseCharacter::RequestBuyPotion()
{
    BuyShopItem(EShopItemType::HealPotion);
}

void ABaseCharacter::ServerBuyPotion_Implementation()
{
    ServerBuyShopItem_Implementation(EShopItemType::HealPotion);
}

void ABaseCharacter::BuyShopItem(EShopItemType ItemType)
{
    if (HasAuthority())
    {
        ServerBuyShopItem_Implementation(ItemType);
        return;
    }

    ServerBuyShopItem(ItemType);
}

void ABaseCharacter::BuyHealPotionFromShop()
{
    BuyShopItem(EShopItemType::HealPotion);
}

void ABaseCharacter::BuyHPPotionFromShop()
{
    BuyShopItem(EShopItemType::HPPotion);
}

void ABaseCharacter::BuyAttackPotionFromShop()
{
    BuyShopItem(EShopItemType::AttackPotion);
}

bool ABaseCharacter::GetPurchasedItem(int32 Index, FShopInventoryItem& Item) const
{
    if (!PurchasedItems.IsValidIndex(Index))
    {
        return false;
    }

    Item = PurchasedItems[Index];
    return true;
}

void ABaseCharacter::MovePurchasedItem(int32 FromIndex, int32 ToIndex)
{
    if (HasAuthority())
    {
        ServerMovePurchasedItem_Implementation(FromIndex, ToIndex);
        return;
    }

    ServerMovePurchasedItem(FromIndex, ToIndex);
}

void ABaseCharacter::AssignPurchasedItemToSlot4(int32 ItemIndex)
{
    if (HasAuthority())
    {
        ServerAssignPurchasedItemToSlot4_Implementation(ItemIndex);
        return;
    }

    ServerAssignPurchasedItemToSlot4(ItemIndex);
}

void ABaseCharacter::UsePurchasedItemAtIndex(int32 ItemIndex)
{
    if (HasAuthority())
    {
        ServerUsePurchasedItem_Implementation(ItemIndex);
        return;
    }

    ServerUsePurchasedItem(ItemIndex);
}

void ABaseCharacter::ServerAssignPurchasedItemToSlot4_Implementation(int32 ItemIndex)
{
    if (!PurchasedItems.IsValidIndex(ItemIndex))
    {
        return;
    }

    const EShopItemType Type = PurchasedItems[ItemIndex].ItemType;
    if (Type != EShopItemType::HPPotion && Type != EShopItemType::AttackPotion)
    {
        return;
    }

    Slot4PurchasedItemIndex = ItemIndex;
    OnRep_Slot4PurchasedItemIndex();
    ForceNetUpdate();
}

void ABaseCharacter::ServerMovePurchasedItem_Implementation(int32 FromIndex, int32 ToIndex)
{
    if (FromIndex == ToIndex || !PurchasedItems.IsValidIndex(FromIndex) || !PurchasedItems.IsValidIndex(ToIndex))
    {
        return;
    }

    PurchasedItems.Swap(FromIndex, ToIndex);

    if (Slot4PurchasedItemIndex == FromIndex)
    {
        Slot4PurchasedItemIndex = ToIndex;
    }
    else if (Slot4PurchasedItemIndex == ToIndex)
    {
        Slot4PurchasedItemIndex = FromIndex;
    }

    OnRep_PurchasedItems();
    OnRep_Slot4PurchasedItemIndex();
    ForceNetUpdate();
}

void ABaseCharacter::ServerBuyShopItem_Implementation(EShopItemType ItemType)
{
    int32 Price = 0;
    UTexture2D* Icon = nullptr;
    FText Name;

    switch (ItemType)
    {
    case EShopItemType::HealPotion:
        Price = PotionPrice;
        Icon = PotionIcon;
        Name = FText::FromString(TEXT("회복 포션"));
        break;
    case EShopItemType::HPPotion:
        Price = HPPotionPrice;
        Icon = HPPotionIcon;
        Name = FText::FromString(TEXT("체력 포션"));
        break;
    case EShopItemType::AttackPotion:
        Price = AttackPotionPrice;
        Icon = AttackPotionIcon;
        Name = FText::FromString(TEXT("공격 포션"));
        break;
    default:
        return;
    }

    if (Price < 0 || Coin < Price)
    {
        return;
    }

    Coin -= Price;

    int32 ItemIndex = PurchasedItems.IndexOfByPredicate(
        [ItemType](const FShopInventoryItem& Item)
        {
            return Item.ItemType == ItemType;
        });

    if (ItemIndex == INDEX_NONE)
    {
        ItemIndex = PurchasedItems.Add({ ItemType, Name, Icon, 1 });
    }
    else
    {
        ++PurchasedItems[ItemIndex].Quantity;
    }

    if (ItemType == EShopItemType::HPPotion || ItemType == EShopItemType::AttackPotion)
    {
        Slot4PurchasedItemIndex = ItemIndex;
        OnRep_Slot4PurchasedItemIndex();
    }

    // The existing quick-use action consumes only healing potions.
    if (ItemType == EShopItemType::HealPotion)
    {
        ++PotionCount;
        OnRep_PotionCount();
    }

    OnRep_Coin();
    OnRep_PurchasedItems();
    ForceNetUpdate();
}

void ABaseCharacter::OnRep_PurchasedItems()
{
    RefreshInventoryWidget();
}

void ABaseCharacter::OnRep_Slot4PurchasedItemIndex()
{
    Slot4Icon = PurchasedItems.IsValidIndex(Slot4PurchasedItemIndex)
        ? PurchasedItems[Slot4PurchasedItemIndex].Icon.Get()
        : EmptySlotIcon;
}

void ABaseCharacter::RefreshInventoryWidget()
{
    if (!InventoryWidget)
    {
        return;
    }

    UGridPanel* InventoryGrid = Cast<UGridPanel>(InventoryWidget->GetWidgetFromName(TEXT("GridPanel_0")));
    if (!InventoryGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_Inventory is missing GridPanel_0."));
        return;
    }

    // Replace the old placeholder WBP_ItemSlot children with slots that own
    // their drag/drop logic.  Array index == visible slot order at all times.
    InventoryGrid->ClearChildren();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    constexpr int32 Columns = 4;
    for (int32 ItemIndex = 0; ItemIndex < PurchasedItems.Num(); ++ItemIndex)
    {
        const FShopInventoryItem& Item = PurchasedItems[ItemIndex];
        UInventoryItemSlotWidget* Slot = CreateWidget<UInventoryItemSlotWidget>(PC, UInventoryItemSlotWidget::StaticClass());
        if (!Slot)
        {
            continue;
        }

        const FText SlotText = FText::FromString(FString::Printf(
            TEXT("%s x%d"),
            *Item.DisplayName.ToString(),
            Item.Quantity));
        Slot->Configure(this, ItemIndex, SlotText, Item.Icon.Get());
        InventoryGrid->AddChildToGrid(Slot, ItemIndex / Columns, ItemIndex % Columns);
    }
}

void ABaseCharacter::BindShopButtons()
{
    if (!ShopWidget)
    {
        return;
    }

    if (UButton* Button = Cast<UButton>(ShopWidget->GetWidgetFromName(TEXT("Btn_Buy_Heal_Potion"))))
    {
        // WBP_Shop was copied from the title screen and still carries its old
        // Blueprint click handlers (including Quit).  They must not run in
        // parallel with a purchase click.
        Button->OnClicked.Clear();
        Button->OnClicked.AddDynamic(this, &ABaseCharacter::BuyHealPotionFromShop);
    }
    if (UButton* Button = Cast<UButton>(ShopWidget->GetWidgetFromName(TEXT("Btn_Buy_HP_Potion"))))
    {
        Button->OnClicked.Clear();
        Button->OnClicked.AddDynamic(this, &ABaseCharacter::BuyHPPotionFromShop);
    }
    if (UButton* Button = Cast<UButton>(ShopWidget->GetWidgetFromName(TEXT("Btn_Buy_ATK_Potion"))))
    {
        Button->OnClicked.Clear();
        Button->OnClicked.AddDynamic(this, &ABaseCharacter::BuyAttackPotionFromShop);
    }
}

void ABaseCharacter::ToggleSkillTree()
{
    if (!SkillTreeWidgetClass)
        return;

    if (!bSkillTreeOpen)
    {
        SkillTreeWidget =
            CreateWidget<USkillTreeWidget>(
                GetWorld(),
                SkillTreeWidgetClass
            );

        if (SkillTreeWidget)
        {
            SkillTreeWidget->AddToViewport();
        }

        bSkillTreeOpen = true;
    }
    else
    {
        if (SkillTreeWidget)
        {
            SkillTreeWidget->RemoveFromParent();
        }

        bSkillTreeOpen = false;
    }
}

void ABaseCharacter::TakePlayerDamage(float Damage)
{
    if (bIsDead) return;

    float FinalDamage = Damage;

    if (CharacterType == ECharacterType::Paladin)
    {
        FinalDamage *= (1.f - DefenseRate);
    }

    // 보호막 먼저 감소
    if (ShieldHP > 0)
    {
        float UsedShield = FMath::Min(ShieldHP, FinalDamage);

        ShieldHP -= UsedShield;

        FinalDamage -= UsedShield;
    }

    // 남은 데미지만 체력 감소
    CurrentHP -= FinalDamage;

    ForceNetUpdate();

    if (CurrentHP > 0)
    {
        // 살아있을 때만 Hit
        PlayAnimMontage(HitMontage);
    }
    else
    {
        // 죽을 때만 Death
        Die();
    }
}

void ABaseCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsUsingSkill = false;
    bIsAttacking = false;
}

void ABaseCharacter::EquipWeapon(TSubclassOf<AActor> WeaponClass, FName SocketName, AActor*& OutWeapon)
{
    if (!WeaponClass) return;

    OutWeapon = GetWorld()->SpawnActor<AActor>(WeaponClass);

    AWeaponBase* Weapon = Cast<AWeaponBase>(OutWeapon);

    if (Weapon)
    {
        Weapon->OwnerCharacter = this;
    }

    if (OutWeapon)
    {
        OutWeapon->AttachToComponent(
            GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            SocketName
        );
    }
}

bool ABaseCharacter::UpgradeSkill(FSkillUpgradeData UpgradeData)
{
	// Combat damage and cooldowns are authoritative on the server. Do not only
	// upgrade the local copy, otherwise the UI changes but gameplay does not.
	if (!HasAuthority())
	{
		ServerUpgradeSkill(UpgradeData);
		return true;
	}

    if (SkillPoints <= 0)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.f,
            FColor::Red,
            TEXT("Not Enough Skill Points")
        );

        return false;
    }

    // 최대 레벨 제한
    if (SkillLevels[UpgradeData.SkillType] >= 4)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.f,
            FColor::Red,
            TEXT("Max Level")
        );

        return false;
    }


    SkillPoints--;

    SkillLevels[UpgradeData.SkillType]++;

    ApplySkillUpgrade(UpgradeData);

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->NotifySkillPointSpent();
    }

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->SkillPoints = SkillPoints;
        GI->SkillLevels = SkillLevels;
    }


	ForceNetUpdate();

	UE_LOG(LogTemp, Log,
		TEXT("Skill upgraded on server: Character=%d Type=%d Level=%d Points=%d Q=%.2f E=%.2f WReduction=%.1f RBonus=%.2f"),
		static_cast<int32>(CharacterType),
		static_cast<int32>(UpgradeData.SkillType),
		SkillLevels.FindRef(UpgradeData.SkillType),
		SkillPoints,
		QMultiplier,
		EMultiplier,
		WarriorWCooldownReduction,
		WarriorRDamageBonus);

    GEngine->AddOnScreenDebugMessage(
        -1,
        2.f,
        FColor::Green,
        TEXT("Skill Upgraded")
    );

    return true;
}

void ABaseCharacter::ServerUpgradeSkill_Implementation(FSkillUpgradeData UpgradeData)
{
	const bool bSuccess = UpgradeSkill(UpgradeData);
	ClientConfirmSkillUpgrade(
		bSuccess,
		UpgradeData.SkillType,
		SkillLevels.FindRef(UpgradeData.SkillType),
		SkillPoints);
}

void ABaseCharacter::ClientConfirmSkillUpgrade_Implementation(
	bool bSuccess,
	ESkillType SkillType,
	int32 NewLevel,
	int32 NewSkillPoints)
{
	SkillPoints = NewSkillPoints;

	if (!bSuccess)
	{
		if (SkillTreeWidget)
		{
			SkillTreeWidget->UpdateSkillPointText();
			SkillTreeWidget->UpdateSkillLevelText();
		}
		return;
	}

	SkillLevels.FindOrAdd(SkillType) = NewLevel;

	FSkillUpgradeData UpgradeData;
	UpgradeData.SkillType = SkillType;
	ApplySkillUpgrade(UpgradeData);

	if (SkillTreeWidget)
	{
		SkillTreeWidget->UpdateSkillPointText();
		SkillTreeWidget->UpdateSkillLevelText();
	}
}

void ABaseCharacter::ApplySkillUpgrade(FSkillUpgradeData UpgradeData)
{
    int32 SkillLevel =
        SkillLevels[UpgradeData.SkillType];

    switch (CharacterType)
    {
    case ECharacterType::Paladin:

        switch (UpgradeData.SkillType)
        {
        case ESkillType::Q:

            if (SkillLevel == 2)
            {
                QMultiplier = 1.2f;
            }
            else if (SkillLevel == 3)
            {
                QMultiplier = 1.5f;
            }
            else if (SkillLevel == 4)
            {
                QMultiplier = 1.8f;
                QCooldown -= 3.f;
            }

            break;

        case ESkillType::W:

            if (SkillLevel == 2)
            {
                DefenseRate = 0.2f;
            }
            else if (SkillLevel == 3)
            {
                DefenseRate = 0.4f;
            }
            else if (SkillLevel == 4)
            {
                DefenseRate = 0.6f;
            }

            break;

        case ESkillType::E:

            if (SkillLevel == 2)
            {
                HealAmount = 0.1f;
            }
            else if (SkillLevel == 3)
            {
                HealAmount = 0.15f;
            }
            else if (SkillLevel == 4)
            {
                HealAmount = 0.2f;
            }

            break;

        case ESkillType::R:

            if (SkillLevel == 2)
            {
                RHealAmount = 0.2f;
            }
            else if (SkillLevel == 3)
            {
                RHealAmount = 0.3f;
            }
            else if (SkillLevel == 4)
            {
                RHealAmount = 0.5f;
            }

            break;
        }

        break;

    case ECharacterType::Archer:

        switch (UpgradeData.SkillType)
        {
        case ESkillType::Q:

            if (SkillLevel == 2)
            {
                QMultiplier = 1.5f;
            }
            else if (SkillLevel == 3)
            {
                QMultiplier = 1.8f;
            }
            else if (SkillLevel == 4)
            {
                QMultiplier = 2.0f;
                QCooldown -= 3.f;
            }

            break;

        case ESkillType::W:

            if (SkillLevel == 2)
            {
                BuffAttackSpeed = 1.5f;
            }
            else if (SkillLevel == 3)
            {
                BuffAttackSpeed = 2.0f;
            }
            else if (SkillLevel == 4)
            {
                BuffAttackSpeed = 2.5f;
            }

            break;

        case ESkillType::E:

            if (SkillLevel == 2)
            {
                EMultiplier = 1.5f;
            }
            else if (SkillLevel == 3)
            {
                EMultiplier = 1.8f;
            }
            else if (SkillLevel == 4)
            {
                EMultiplier = 2.0f;
                ERadius += 100.f;
            }

            break;

        case ESkillType::R:

            if (SkillLevel == 2)
            {
                RMultiplier = 3.0f;
            }
            else if (SkillLevel == 3)
            {
                RMultiplier = 3.0f;
                bRBonusDamage = true;
            }
            else if (SkillLevel == 4)
            {
                RMultiplier = 4.0f;
                bRBonusDamage = true;
            }

            break;
        }

        break;

    case ECharacterType::Warrior:

        switch (UpgradeData.SkillType)
        {
        case ESkillType::Q:
			QMultiplier =
				(SkillLevel >= 4) ? 1.8f :
				(SkillLevel == 3) ? 1.5f :
				(SkillLevel == 2) ? 1.2f : 1.0f;

            break;

        case ESkillType::W:
			WarriorWCooldownReduction =
				(SkillLevel >= 4) ? 4.0f :
				(SkillLevel == 3) ? 3.0f :
				(SkillLevel == 2) ? 2.0f : 0.0f;

            break;

        case ESkillType::E:
			EMultiplier =
				(SkillLevel >= 4) ? 1.8f :
				(SkillLevel == 3) ? 1.5f :
				(SkillLevel == 2) ? 1.2f : 1.0f;

            break;

        case ESkillType::R:
			WarriorRDamageBonus =
				(SkillLevel >= 4) ? 1.0f :
				(SkillLevel == 3) ? 0.8f :
				(SkillLevel == 2) ? 0.5f : 0.0f;

            break;
        }

        break;
    }
}

void ABaseCharacter::SetNearbyLantern(ALantern* Lantern)
{
    NearbyLantern = Lantern;
}

void ABaseCharacter::Interact(const FInputActionValue& Value)
{
    if (NearbyAltar)
    {
        ServerInteractAltar();
        return;
    }

    if (NearbyPortal)
    {
        ServerInteractPortal(NearbyPortal);
        return;
    }

    if (NearbyDungeonPortal)
    {
        ServerInteractDungeonPortal();
        return;
    }

    if (bDarknessDebuff && bPrismEquipped)
    {
        bDarknessDebuff = false;

        UE_LOG(LogTemp, Warning,
            TEXT("Darkness Cleared"));

        return;
    }

    if (NearbyLantern)
    {
        ServerPickupLantern(NearbyLantern);
        return;
    }

    if (NearbyPrism)
    {
        ServerPickupPrism(NearbyPrism);
        return;
    }
}

void ABaseCharacter::InteractWithQuestGiver()
{
    if (NearbyQuestGiver)
    {
        ServerInteractQuestGiver(NearbyQuestGiver);
    }
}

void ABaseCharacter::UseSlot1(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    UE_LOG(LogTemp, Warning,
        TEXT("UseSlot1 HasLantern=%d Equipped=%d"),
        bHasLantern,
        bLanternEquipped);

    if (bPrismEquipped)
    {
        return;
    }

    if (!bHasLantern)
    {
        return;
    }

    ServerUseSlot1();
}

void ABaseCharacter::UseSlot2(const FInputActionValue& Value)
{
    ServerUsePotion();
}

void ABaseCharacter::ServerUsePotion_Implementation()
{
    if (bIsUsingPotion || PotionCount <= 0 || CurrentHP >= MaxHP || bIsDead)
    {
        return;
    }

    bIsUsingPotion = true;
    MulticastStartPotionUse();
    const float AnimationDuration = GetPotionAnimation()
        ? GetPotionAnimation()->GetPlayLength()
        : PotionUseDuration;
    GetWorldTimerManager().SetTimer(PotionUseTimer, this, &ABaseCharacter::FinishPotionUse,
        FMath::Max(AnimationDuration, KINDA_SMALL_NUMBER), false);
    ForceNetUpdate();
}

UAnimSequenceBase* ABaseCharacter::GetPotionAnimation() const
{
    switch (CharacterType)
    {
    case ECharacterType::Paladin: return PaladinPotionAnimation;
    case ECharacterType::Archer: return ArcherPotionAnimation;
    case ECharacterType::Warrior: return WarriorPotionAnimation;
    default: return nullptr;
    }
}

void ABaseCharacter::MulticastStartPotionUse_Implementation()
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (UAnimSequenceBase* PotionAnimation = GetPotionAnimation())
        {
            ActivePotionMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(PotionAnimation, TEXT("DefaultSlot"));
        }
    }

    if (HealEffect)
    {
        PotionHealEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            HealEffect, GetRootComponent(), NAME_None, FVector::ZeroVector,
            FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
    }
}

void ABaseCharacter::FinishPotionUse()
{
    if (!bIsUsingPotion)
    {
        return;
    }

    bIsUsingPotion = false;
    --PotionCount;
    HealPlayer(MaxHP * PotionHealPercent);
    OnRep_PotionCount();
    MulticastCancelPotionUse();
    ForceNetUpdate();
}

void ABaseCharacter::ServerCancelPotionUse_Implementation()
{
    CancelPotionUse();
}

void ABaseCharacter::CancelPotionUse()
{
    if (!bIsUsingPotion)
    {
        return;
    }

    bIsUsingPotion = false;
    GetWorldTimerManager().ClearTimer(PotionUseTimer);
    MulticastCancelPotionUse();
    ForceNetUpdate();
}

void ABaseCharacter::MulticastCancelPotionUse_Implementation()
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (ActivePotionMontage)
        {
            AnimInstance->Montage_Stop(0.15f, ActivePotionMontage);
        }
    }
    ActivePotionMontage = nullptr;
    if (PotionHealEffectComponent)
    {
        PotionHealEffectComponent->DeactivateImmediate();
        PotionHealEffectComponent = nullptr;
    }
}

void ABaseCharacter::UseSlot3(const FInputActionValue& Value)
{
    ServerCancelPotionUse();
    if (bLanternEquipped)
    {
        return;
    }

    if (!bHasPrism)
    {
        return;
    }

    ServerUseSlot3();
}

void ABaseCharacter::UseSlot4(const FInputActionValue& Value)
{
    ServerUseSlot4();
}

void ABaseCharacter::ServerUseSlot4_Implementation()
{
    ServerUsePurchasedItem_Implementation(Slot4PurchasedItemIndex);
}

void ABaseCharacter::ServerUsePurchasedItem_Implementation(int32 ItemIndex)
{
    if (!PurchasedItems.IsValidIndex(ItemIndex) || PurchasedItems[ItemIndex].Quantity <= 0)
    {
        return;
    }

    const EShopItemType ItemType = PurchasedItems[ItemIndex].ItemType;
    if (ItemType == EShopItemType::HPPotion)
    {
        const float BonusHP = MaxHP * 0.2f;
        MaxHP += BonusHP;
        CurrentHP += BonusHP;
        OnRep_CurrentHP();
    }
    else if (ItemType == EShopItemType::AttackPotion)
    {
        AttackPower *= 1.2f;
    }
    else
    {
        return;
    }

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->bHasShopStatBonuses = true;
        PS->SavedMaxHP = MaxHP;
        PS->SavedAttackPower = AttackPower;
    }

    --PurchasedItems[ItemIndex].Quantity;
    if (PurchasedItems[ItemIndex].Quantity <= 0)
    {
        PurchasedItems.RemoveAt(ItemIndex);

        if (Slot4PurchasedItemIndex == ItemIndex)
        {
            Slot4PurchasedItemIndex = INDEX_NONE;
        }
        else if (Slot4PurchasedItemIndex > ItemIndex)
        {
            --Slot4PurchasedItemIndex;
        }
    }

    OnRep_PurchasedItems();
    OnRep_Slot4PurchasedItemIndex();
    ForceNetUpdate();
}

void ABaseCharacter::OnLanternLightBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ABaseCharacter* Player =
        Cast<ABaseCharacter>(OtherActor);

    if (!Player)
    {
        return;
    }

    Player->bInsideLanternLight = true;
}

void ABaseCharacter::OnLanternLightEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ABaseCharacter* Player =
        Cast<ABaseCharacter>(OtherActor);

    if (!Player)
    {
        return;
    }

    Player->bInsideLanternLight = false;
}

void ABaseCharacter::OnLanternEquipped()
{
}

void ABaseCharacter::OnLanternUnequipped()
{
}

void ABaseCharacter::OnLanternUnequipFinished()
{
}

void ABaseCharacter::EnableWeaponCollision()
{
    AWeaponBase* Weapon = Cast<AWeaponBase>(RightHandWeapon);

    if (Weapon)
    {
        Weapon->EnableCollision();

        UE_LOG(LogTemp, Warning, TEXT("Collision ON"));
    }
}

void ABaseCharacter::DisableWeaponCollision()
{
    AWeaponBase* Weapon = Cast<AWeaponBase>(RightHandWeapon);

    if (Weapon)
    {
        Weapon->DisableCollision();

        UE_LOG(LogTemp, Warning, TEXT("Collision OFF"));
    }
}

void ABaseCharacter::SpawnArrow()
{
    if (CharacterType != ECharacterType::Archer)
    {
        return;
    }

    AWeaponBase* Bow = Cast<AWeaponBase>(LeftHandWeapon);

    if (!Bow || !ArrowClass)
    {
        return;
    }

    FVector SpawnLocation = Bow->Mesh->GetSocketLocation(TEXT("ArrowSocket"));

    FRotator SpawnRotation = GetActorForwardVector().Rotation();

    AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(ArrowClass, SpawnLocation, SpawnRotation);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;
        Arrow->DamageMultiplier = 1.f;
        Arrow->SetupTrail();
    }
}

void ABaseCharacter::SpawnQArrow()
{
    if (CharacterType != ECharacterType::Archer)
    {
        return;
    }

    AWeaponBase* Bow = Cast<AWeaponBase>(LeftHandWeapon);

    if (!Bow || !ArrowClass)
    {
        return;
    }

    FVector SpawnLocation = Bow->Mesh->GetSocketLocation(TEXT("ArrowSocket"));

    FVector Forward = GetActorForwardVector();

    const int32 ArrowCount = 5;
    const float SpreadAngle = 30.f;
    const int32 VolleyId = ++NextArcherQVolleyId;

    for (int32 i = 0; i < ArrowCount; i++)
    {
        float Angle = -SpreadAngle * 0.5f + (SpreadAngle / (ArrowCount - 1)) * i;

        FVector Direction = FRotator(0.f, Angle, 0.f).RotateVector(Forward);

        FRotator Rotation = Direction.Rotation();

        AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(ArrowClass, SpawnLocation, Rotation);

        if (Arrow)
        {
            Arrow->OwnerCharacter = this;
            Arrow->ArrowType = EArrowType::QExplosive;
            Arrow->QVolleyId = VolleyId;
            Arrow->SetupTrail();

            Arrow->DamageMultiplier = QMultiplier;
        }
    }
}

void ABaseCharacter::SpawnRArrow()
{
    if (CharacterType != ECharacterType::Archer)
    {
        return;
    }

    AWeaponBase* Bow = Cast<AWeaponBase>(LeftHandWeapon);

    if (!Bow || !ArrowClass)
    {
        return;
    }

    FVector SpawnLocation = Bow->Mesh->GetSocketLocation(TEXT("ArrowSocket"));

    FRotator SpawnRotation = GetActorForwardVector().Rotation();

    AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(ArrowClass, SpawnLocation, SpawnRotation);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;

        Arrow->ArrowType = EArrowType::Pierce;

        Arrow->DamageMultiplier = RMultiplier;

        Arrow->SetupTrail();

        Arrow->SetActorScale3D(FVector(3.f, 3.f, 3.f));
    }
}

void ABaseCharacter::SpawnEArrow()
{
    if (CharacterType != ECharacterType::Archer)
    {
        return;
    }

    AWeaponBase* Bow = Cast<AWeaponBase>(LeftHandWeapon);

    if (!Bow || !ArrowClass)
    {
        return;
    }

    FVector SpawnLocation = Bow->Mesh->GetSocketLocation(TEXT("ArrowSocket"));

    FRotator SpawnRotation = GetActorForwardVector().Rotation();

    AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(ArrowClass, SpawnLocation, SpawnRotation);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;

        Arrow->ArrowType = EArrowType::Explosive;
        Arrow->SetupTrail();

        Arrow->DamageMultiplier = EMultiplier;

        FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * 300.f;

        FVector LaunchVelocity;

        bool bSuccess = UGameplayStatics::SuggestProjectileVelocity(this, LaunchVelocity, SpawnLocation, TargetLocation, 1200.f, false, 0.f, 0.f, ESuggestProjVelocityTraceOption::DoNotTrace);

        if (bSuccess)
        {
            Arrow->ProjectileMovement->ProjectileGravityScale = 1.f;
            Arrow->ProjectileMovement->Velocity = LaunchVelocity;

            if (HasAuthority())
            {
                ArcherERainCenter = TargetLocation;
                GetWorldTimerManager().ClearTimer(ArcherERainDamageTimer);
                GetWorldTimerManager().ClearTimer(ArcherERainEndTimer);
                ApplyArcherERainDamage();
                GetWorldTimerManager().SetTimer(ArcherERainDamageTimer, this, &ABaseCharacter::ApplyArcherERainDamage, 1.f, true);
                GetWorldTimerManager().SetTimer(ArcherERainEndTimer, this, &ABaseCharacter::EndArcherERainDamage, ArcherERainDuration, false);
            }
        }
    }
}

void ABaseCharacter::ApplyArcherERainDamage()
{
    if (!HasAuthority()) return;

    for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
    {
        ADragonBoss* Dragon = *It;
        if (Dragon && FVector::DistSquared(Dragon->GetActorLocation(), ArcherERainCenter) <= FMath::Square(ERadius))
        {
            Dragon->TakeBossDamage(ArcherERainDamagePerSecond);
        }
    }
}

void ABaseCharacter::EndArcherERainDamage()
{
    GetWorldTimerManager().ClearTimer(ArcherERainDamageTimer);
}

bool ABaseCharacter::IsDead() const
{
    return bIsDead;
}

void ABaseCharacter::SetNearbyPrism(
    ADungeonPrism* Prism)
{
    NearbyPrism = Prism;
}

void ABaseCharacter::EndAttackSpeedBuff()
{
    AttackSpeed = DefaultAttackSpeed;

    UE_LOG(LogTemp, Warning,
        TEXT("Attack Speed Buff End"));
}

void ABaseCharacter::RotateToMouseCursor()
{
    if (bIsDead)
    {
        return;
    }

    if (bIsUsingSkill || bIsAttacking)
    {
        return;
    }

    APlayerController* PC =
        Cast<APlayerController>(GetController());

    if (!PC)
    {
        return;
    }

    GetCharacterMovement()->StopMovementImmediately();
    PC->StopMovement();

    FHitResult Hit;

    PC->GetHitResultUnderCursor(
        ECC_Visibility,
        false,
        Hit);

    FVector Direction =
        Hit.Location - GetActorLocation();

    Direction.Z = 0.f;

    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();

        SetActorRotation(NewRotation);

        if (!HasAuthority())
        {
            ServerRotate(NewRotation);
        }
    }
}

bool ABaseCharacter::CanUseCombatAction() const
{
    return
        !bLanternEquipped &&
        !bPrismEquipped &&
        !bIsEquippingLantern &&
        !bIsEquippingPrism &&
        !bIsDead &&
        !bIsUsingSkill &&
        !bIsAttacking;
}

void ABaseCharacter::OnPrismEquipped()
{
}

void ABaseCharacter::OnPrismUnequipped()
{
}

void ABaseCharacter::OnPrismUnequipFinished()
{
}

void ABaseCharacter::EndArcherWBuff()
{
    AttackSpeed = DefaultAttackSpeed;

    if (WAreaComponent)
    {
        WAreaComponent->DestroyComponent();
        WAreaComponent = nullptr;
    }
}

void ABaseCharacter::RestoreSkillUpgrades()
{
    for (const auto& Pair : SkillLevels)
    {
		FSkillUpgradeData Data;
		Data.SkillType = Pair.Key;
		ApplySkillUpgrade(Data);
    }
}

void ABaseCharacter::SetNearbyPortal(APortal* Portal)
{
    NearbyPortal = Portal;
}

void ABaseCharacter::SetNearbyDungeonPortal(ADungeonPortal* Portal)
{
    NearbyDungeonPortal = Portal;
}

void ABaseCharacter::ServerUseQ_Implementation()
{
    MulticastPlayQ();

    UseQ();

    ClientStartSkillCooldown(ESkillType::Q, QCooldown);

    ForceNetUpdate();

    ExecuteQDamage();
}

void ABaseCharacter::MulticastPlayQ_Implementation()
{
    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    if (CharacterType == ECharacterType::Archer)
    {
        if (QMontage)
        {
            PlayAnimMontage(QMontage, AttackSpeed);
        }
    }
    else
    {
        if (QMontage)
        {
            PlayAnimMontage(QMontage);
        }
        else
        {
            bIsUsingSkill = false;
        }
    }

    if (QSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            QSkillEffect,
            GetMesh(),
            TEXT("SwordSocket"),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
}

void ABaseCharacter::UseQ()
{
    if (CharacterType == ECharacterType::Archer)
    {
        bCanUseQ = false;

        QRemainingCooldown = QCooldown;

        GetWorldTimerManager().SetTimer(
            QCooldownTimer,
            this,
            &ABaseCharacter::ResetQCooldown,
            QCooldown,
            false);

        return;
    }

    bCanUseQ = false;

    QRemainingCooldown = QCooldown;

    GetWorldTimerManager().SetTimer(
        QCooldownTimer,
        this,
        &ABaseCharacter::ResetQCooldown,
        QCooldown,
        false);
}

void ABaseCharacter::ExecuteQDamage()
{
    UE_LOG(LogTemp, Warning, TEXT("ExecuteQDamage"));

    FVector Start = GetActorLocation();

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(QRadius);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        Start,
        FQuat::Identity,
        ECC_Pawn,
        Sphere);

    UE_LOG(LogTemp, Warning, TEXT("Hit : %d"), bHit);

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            AMonster* Monster = Cast<AMonster>(Result.GetActor());

            UE_LOG(LogTemp, Warning, TEXT("Overlap : %s"),
                *Result.GetActor()->GetName());

            if (Monster)
            {
                Monster->TakeMonsterDamage(AttackPower * QMultiplier);

                MulticastQImpact(
                    Monster->GetActorLocation() + FVector(0, 0, 80)
                );
            }

            ADragonBoss* Dragon = Cast<ADragonBoss>(Result.GetActor());

            if (Dragon)
            {
                Dragon->TakeBossDamage(AttackPower * QMultiplier);

                if (QImpactEffect)
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                        GetWorld(),
                        QImpactEffect,
                        Dragon->GetActorLocation() + FVector(0, 0, 120));
                }

                UE_LOG(LogTemp, Warning, TEXT("Dragon Hit By Q"));
            }
        }
    }
}

void ABaseCharacter::ServerUseW_Implementation()
{
    bCanUseW = false;

    WRemainingCooldown = WCooldown;

    GetWorldTimerManager().SetTimer(
        WCooldownTimer,
        this,
        &ABaseCharacter::ResetWCooldown,
        WCooldown,
        false
    );

	if (CharacterType == ECharacterType::Warrior && WarriorWCooldownReduction > 0.0f)
	{
		auto ReduceCooldown = [this](
			FTimerHandle& TimerHandle,
			float& RemainingCooldown,
			void (ABaseCharacter::*ResetFunction)())
		{
			if (!GetWorldTimerManager().IsTimerActive(TimerHandle))
			{
				return;
			}

			const float NewRemaining = FMath::Max(
				0.0f,
				GetWorldTimerManager().GetTimerRemaining(TimerHandle) - WarriorWCooldownReduction);

			GetWorldTimerManager().ClearTimer(TimerHandle);
			RemainingCooldown = NewRemaining;

			if (NewRemaining <= 0.0f)
			{
				(this->*ResetFunction)();
			}
			else
			{
				GetWorldTimerManager().SetTimer(
					TimerHandle,
					this,
					ResetFunction,
					NewRemaining,
					false);
			}
		};

		ReduceCooldown(QCooldownTimer, QRemainingCooldown, &ABaseCharacter::ResetQCooldown);
		ReduceCooldown(ECooldownTimer, ERemainingCooldown, &ABaseCharacter::ResetECooldown);
		ReduceCooldown(RCooldownTimer, RRemainingCooldown, &ABaseCharacter::ResetRCooldown);
	}

    ClientStartSkillCooldown(ESkillType::W, WCooldown);

    ForceNetUpdate();

	UE_LOG(LogTemp, Log,
		TEXT("Use W: Character=%d Level=%d Reduction=%.1f WCooldown=%.1f QRemain=%.1f ERemain=%.1f RRemain=%.1f"),
		static_cast<int32>(CharacterType),
		SkillLevels.FindRef(ESkillType::W),
		WarriorWCooldownReduction,
		WCooldown,
		QRemainingCooldown,
		ERemainingCooldown,
		RRemainingCooldown);

    MulticastPlayW();
}

void ABaseCharacter::MulticastPlayW_Implementation()
{
    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    if (WMontage)
    {
        PlayAnimMontage(WMontage);
    }

    if (WSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            WSkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
    bIsUsingSkill = false;
}

void ABaseCharacter::ServerUseE_Implementation()
{
    MulticastPlayE();

    ExecuteE();

    ClientStartSkillCooldown(ESkillType::E, ECooldown);

    ForceNetUpdate();
}

void ABaseCharacter::MulticastPlayE_Implementation()
{
    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    if (EMontage)
    {
        PlayAnimMontage(EMontage);
    }

    if (ESkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            ESkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
}

void ABaseCharacter::ExecuteE()
{
    if (!HasAuthority())
    {
        return;
    }

    if (CharacterType == ECharacterType::Archer)
    {
        bCanUseE = false;

        ERemainingCooldown = ECooldown;

        GetWorldTimerManager().SetTimer(
            ECooldownTimer,
            this,
            &ABaseCharacter::ResetECooldown,
            ECooldown,
            false);

        return;
    }

    bCanUseE = false;

    ERemainingCooldown = ECooldown;

    GetWorldTimerManager().SetTimer(
        ECooldownTimer,
        this,
        &ABaseCharacter::ResetECooldown,
        ECooldown,
        false
    );

	if (CharacterType == ECharacterType::Warrior)
	{
		TArray<FOverlapResult> Overlaps;
		const FCollisionShape DamageSphere = FCollisionShape::MakeSphere(WarriorERadius);

		GetWorld()->OverlapMultiByChannel(
			Overlaps,
			GetActorLocation(),
			FQuat::Identity,
			ECC_Pawn,
			DamageSphere);

		TSet<AActor*> DamagedActors;
		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || DamagedActors.Contains(HitActor))
			{
				continue;
			}

			if (AMonster* Monster = Cast<AMonster>(HitActor))
			{
				Monster->TakeMonsterDamage(AttackPower * EMultiplier);
				DamagedActors.Add(HitActor);
			}
			else if (ADragonBoss* Dragon = Cast<ADragonBoss>(HitActor))
			{
				Dragon->TakeBossDamage(AttackPower * EMultiplier);
				DamagedActors.Add(HitActor);
			}
		}

		return;
	}

    if (CharacterType == ECharacterType::Paladin)
    {
        ShieldHP = MaxHP * 0.1f;

        UE_LOG(LogTemp, Warning,
            TEXT("Shield : %f"),
            ShieldHP);
    }

    if (CharacterType == ECharacterType::Paladin && HealAmount > 0.f)
    {
        HealTickCount = 0;

        GetWorldTimerManager().SetTimer(
            HealOverTimeHandle,
            this,
            &ABaseCharacter::HealOverTimeTick,
            1.f,
            true);
    }
}

void ABaseCharacter::ServerUseR_Implementation()
{
    if (!bCanUseR || !CanUseCombatAction())
    {
        return;
    }

    ExecuteR();

    ClientStartSkillCooldown(ESkillType::R, RCooldown);

    ForceNetUpdate();

    MulticastPlayR();
}

void ABaseCharacter::ClientStartSkillCooldown_Implementation(
    ESkillType SkillType,
    float Duration)
{
    if (HasAuthority())
    {
        return;
    }

    const float CooldownDuration = FMath::Max(0.f, Duration);

    switch (SkillType)
    {
    case ESkillType::Q:
        bCanUseQ = false;
        QRemainingCooldown = CooldownDuration;
        break;

    case ESkillType::W:
        bCanUseW = false;
        WRemainingCooldown = CooldownDuration;
        break;

    case ESkillType::E:
        bCanUseE = false;
        ERemainingCooldown = CooldownDuration;
        break;

    case ESkillType::R:
        bCanUseR = false;
        RRemainingCooldown = CooldownDuration;
        break;
    }
}

void ABaseCharacter::MulticastPlayR_Implementation()
{
    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    if (RMontage)
    {
        PlayAnimMontage(RMontage);
    }

    if (RSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            RSkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
}

void ABaseCharacter::MulticastPlayRHealEffect_Implementation(
    FVector Location)
{
    if (!HealEffect)
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        HealEffect,
        Location,
        FRotator::ZeroRotator,
        FVector::OneVector,
        true,
        true
    );
}

void ABaseCharacter::ExecuteR()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!bCanUseR)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    if (CharacterType == ECharacterType::Archer)
    {
        bCanUseR = false;

        RRemainingCooldown = RCooldown;

        GetWorldTimerManager().SetTimer(
            RCooldownTimer,
            this,
            &ABaseCharacter::ResetRCooldown,
            RCooldown,
            false
        );

        return;
    }

    bCanUseR = false;

    RRemainingCooldown = RCooldown;

    GetWorldTimerManager().SetTimer(
        RCooldownTimer,
        this,
        &ABaseCharacter::ResetRCooldown,
        RCooldown,
        false
    );

	if (CharacterType == ECharacterType::Warrior)
	{
		// Recasting/resetting can never multiply an already buffed value.
		AttackPower = BaseAttackPower * (1.0f + WarriorRDamageBonus);

		GetWorldTimerManager().ClearTimer(WarriorRBuffHandle);
		GetWorldTimerManager().SetTimer(
			WarriorRBuffHandle,
			this,
			&ABaseCharacter::EndWarriorRBuff,
			10.0f,
			false);

		UE_LOG(LogTemp, Log,
			TEXT("Warrior R: Level %d, Damage Bonus %.0f%% for 10 seconds"),
			SkillLevels.FindRef(ESkillType::R),
			WarriorRDamageBonus * 100.0f);
		return;
	}

    TArray<AActor*> Players;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ABaseCharacter::StaticClass(),
        Players);

    for (AActor* Actor : Players)
    {
        ABaseCharacter* Player =
            Cast<ABaseCharacter>(Actor);

        if (!Player)
        {
            continue;
        }

        float Heal = Player->MaxHP * RHealAmount;

        UE_LOG(LogTemp, Warning,
            TEXT("%s HP : %.0f -> %.0f"),
            *Player->GetName(),
            Player->CurrentHP,
            FMath::Min(Player->CurrentHP + Heal, Player->MaxHP));

        Player->HealPlayer(Heal);

        MulticastPlayRHealEffect(
            Player->GetActorLocation());

        UE_LOG(LogTemp, Warning,
            TEXT("R Heal : %s"),
            *Player->GetName());
    }

    TArray<AActor*> Monsters;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AMonster::StaticClass(),
        Monsters);

    for (AActor* Actor : Monsters)
    {
        AMonster* Monster = Cast<AMonster>(Actor);

        if (!Monster)
        {
            continue;
        }

        if (Monster->bIsDead)
        {
            continue;
        }

        if (!Monster->bIsChasing)
        {
            continue;
        }

        Monster->ApplyTaunt(this);
    }
}

void ABaseCharacter::EndWarriorRBuff()
{
	AttackPower = BaseAttackPower;
}

void ABaseCharacter::ServerAttack_Implementation()
{
    MulticastAttack();

    ExecuteAttack();
}

void ABaseCharacter::MulticastAttack_Implementation()
{
    bIsAttacking = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    if (AttackMontage)
    {
        PlayAnimMontage(
            AttackMontage,
            AttackSpeed);
    }
}

void ABaseCharacter::ExecuteAttack()
{
    if (!HasAuthority())
    {
        return;
    }

    APlayerController* PC =
        Cast<APlayerController>(GetController());

    if (PC)
    {
        FHitResult Hit;

        PC->GetHitResultUnderCursor(
            ECC_Visibility,
            false,
            Hit);

        FVector LookDirection =
            Hit.Location - GetActorLocation();

        LookDirection.Z = 0.f;

        FRotator TargetRotation =
            LookDirection.Rotation();

        SetActorRotation(TargetRotation);
    }

    FVector Start = GetActorLocation();

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere =
        FCollisionShape::MakeSphere(150.f);

    bool bHit =
        GetWorld()->OverlapMultiByChannel(
            Overlaps,
            Start,
            FQuat::Identity,
            ECC_Pawn,
            Sphere);

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            AMonster* Monster =
                Cast<AMonster>(Result.GetActor());

            if (Monster)
            {
                Monster->TakeMonsterDamage(
                    AttackPower);

                UE_LOG(LogTemp, Warning,
                    TEXT("Basic Attack Hit"));
            }

            ADragonBoss* Dragon =
                Cast<ADragonBoss>(Result.GetActor());

            if (Dragon)
            {
                Dragon->TakeBossDamage(
                    AttackPower);

                UE_LOG(LogTemp, Warning,
                    TEXT("Basic Attack Hit Dragon"));
            }
        }
    }
}

void ABaseCharacter::MulticastQImpact_Implementation(FVector Location)
{
    if (!QImpactEffect)
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        QImpactEffect,
        Location
    );
}

void ABaseCharacter::ServerRotate_Implementation(FRotator NewRotation)
{
    if (bIsDead)
    {
        return;
    }

    GetCharacterMovement()->StopMovementImmediately();

    if (AController* CurrentController = GetController())
    {
        CurrentController->StopMovement();
    }

    SetActorRotation(NewRotation);

    ForceNetUpdate();
}

void ABaseCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaseCharacter, CurrentHP);
    DOREPLIFETIME(ABaseCharacter, MaxHP);
    DOREPLIFETIME(ABaseCharacter, bIsDead);
	DOREPLIFETIME(ABaseCharacter, CharacterType);
    DOREPLIFETIME(ABaseCharacter, bHasLantern);
    DOREPLIFETIME(ABaseCharacter, bLanternEquipped);
    DOREPLIFETIME(ABaseCharacter, bLanternPoseActive);
    DOREPLIFETIME(ABaseCharacter, bHasPrism);
    DOREPLIFETIME(ABaseCharacter, bPrismEquipped);
    DOREPLIFETIME(ABaseCharacter, bPrismPoseActive);
	DOREPLIFETIME(ABaseCharacter, Coin);
	DOREPLIFETIME_CONDITION(ABaseCharacter, PotionCount, COND_OwnerOnly);
	DOREPLIFETIME(ABaseCharacter, bIsUsingPotion);
	DOREPLIFETIME_CONDITION(ABaseCharacter, PurchasedItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABaseCharacter, Slot4PurchasedItemIndex, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABaseCharacter, SkillPoints, COND_OwnerOnly);
    DOREPLIFETIME(ABaseCharacter, bDarknessDebuff);
    DOREPLIFETIME_CONDITION(ABaseCharacter, QRemainingCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, WRemainingCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, ERemainingCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, RRemainingCooldown, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, bCanUseQ, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, bCanUseW, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, bCanUseE, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ABaseCharacter, bCanUseR, COND_OwnerOnly);
}

void ABaseCharacter::OnRep_CurrentHP()
{
    UE_LOG(LogTemp, Warning,
        TEXT("Current HP : %f"),
        CurrentHP);
}

void ABaseCharacter::HealPlayer(float Amount)
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentHP = FMath::Clamp(
        CurrentHP + Amount,
        0.f,
        MaxHP);

    ForceNetUpdate();
}

void ABaseCharacter::SetNearbyAltar(AAltar* Altar)
{
    NearbyAltar = Altar;

    UE_LOG(LogTemp, Warning,
        TEXT("Nearby Altar : %s"),
        Altar ? TEXT("SET") : TEXT("NULL"));
}

void ABaseCharacter::ServerInteractAltar_Implementation()
{
    if (!NearbyAltar)
    {
        return;
    }

    if (AltarBeingPlaced.IsValid())
    {
        return;
    }

    if (!NearbyAltar->bLanternPlaced)
    {
        if (bHasLantern && bLanternEquipped)
        {
            AltarBeingPlaced = NearbyAltar;
            ClientShowAltarPlacementProgress(3.f);
            GetWorldTimerManager().SetTimer(
                AltarPlacementTimer, this, &ABaseCharacter::FinishAltarPlacement, 3.f, false);
        }
    }
    else
    {
        NearbyAltar->RemoveLantern(this);
    }
}

void ABaseCharacter::FinishAltarPlacement()
{
    AAltar* Altar = AltarBeingPlaced.Get();
    AltarBeingPlaced.Reset();
    ClientHideAltarPlacementProgress();

    if (Altar && !Altar->bLanternPlaced && bHasLantern && bLanternEquipped)
    {
        Altar->PlaceLantern(this);
    }
}

void ABaseCharacter::ClientShowAltarPlacementProgress_Implementation(float Duration)
{
    if (!AltarProgressWidget)
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            AltarProgressWidget = CreateWidget<UAltarProgressWidget>(PC, UAltarProgressWidget::StaticClass());
            if (AltarProgressWidget)
            {
                AltarProgressWidget->AddToViewport(100);
            }
        }
    }
    if (AltarProgressWidget)
    {
        AltarProgressWidget->StartProgress(Duration);
    }
}

void ABaseCharacter::ClientHideAltarPlacementProgress_Implementation()
{
    if (AltarProgressWidget)
    {
        AltarProgressWidget->StopProgress();
    }
}

void ABaseCharacter::ServerPickupLantern_Implementation(ALantern* Lantern)
{
    if (!Lantern)
    {
        return;
    }

    bHasLantern = true;

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
		CharacterType = PS->SelectedCharacter;

        PS->bHasLantern = true;

        UE_LOG(LogTemp, Warning,
            TEXT("PlayerState Lantern Saved : %d"),
            PS->bHasLantern);
    }

    OnRep_HasLantern();

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bWorldLanternDestroyed = true;
    }

    Lantern->Destroy();

    NearbyLantern = nullptr;
}

void ABaseCharacter::OnRep_LanternEquipped()
{
    UE_LOG(LogTemp, Warning,
        TEXT("OnRep_LanternEquipped %d"),
        bLanternEquipped);

    EquippedLanternMesh->SetVisibility(bLanternEquipped);

    LanternLight->SetVisibility(bLanternEquipped);

    // Tick will resolve the portal and activate the local guide effect. Hide
    // immediately on unequip so it never lingers for one update interval.
    if (!bLanternEquipped && LanternDirectionEffectComponent)
    {
        LanternDirectionEffectComponent->Deactivate();
        LanternDirectionEffectComponent->SetVisibility(false);
    }

    LanternLightSphere->SetCollisionEnabled(
        bLanternEquipped
        ? ECollisionEnabled::QueryOnly
        : ECollisionEnabled::NoCollision);

    if (CharacterType == ECharacterType::Paladin)
    {
        if (RightHandWeapon)
        {
            RightHandWeapon->SetActorHiddenInGame(
                bLanternEquipped);

            RightHandWeapon->SetActorEnableCollision(
                !bLanternEquipped);
        }
    }

    bLanternPoseActive = bLanternEquipped;
}

void ABaseCharacter::RefreshLanternState()
{
    OnRep_LanternEquipped();

    ForceNetUpdate();
}

void ABaseCharacter::ServerUseSlot1_Implementation()
{
    UE_LOG(LogTemp, Warning,
        TEXT("ServerUseSlot1 Start Has=%d Equipped=%d Role=%d"),
        bHasLantern,
        bLanternEquipped,
        (int32)GetLocalRole());

    if (bPrismEquipped)
        return;

    if (!bHasLantern)
        return;

    if (bIsEquippingLantern)
        return;

    bIsEquippingLantern = true;

    const bool bEquip = !bLanternEquipped;

    bLanternEquipped = bEquip;

    UE_LOG(LogTemp, Warning,
        TEXT("ServerUseSlot1 After Equipped=%d"),
        bLanternEquipped);

    bLanternPoseActive = bEquip;

    bIsEquippingLantern = false;

    OnRep_LanternEquipped();

    ForceNetUpdate();

    bIsEquippingLantern = false;

    MulticastPlayLanternMontage(bEquip);
}

void ABaseCharacter::MulticastPlayLanternMontage_Implementation(bool bEquip)
{
    if (bEquip)
    {
        if (LanternEquipMontage)
        {
            PlayAnimMontage(LanternEquipMontage);
        }
    }
    else
    {
        if (LanternUnequipMontage)
        {
            PlayAnimMontage(LanternUnequipMontage);
        }
    }
}

void ABaseCharacter::ServerUseSlot3_Implementation()
{
    if (bLanternEquipped)
        return;

    if (!bHasPrism)
        return;

    if (bIsEquippingPrism)
        return;

    bIsEquippingPrism = true;

    const bool bEquip = !bPrismEquipped;

    bPrismEquipped = bEquip;
    bPrismPoseActive = bEquip;

    bIsEquippingPrism = false;

    RefreshPrismState();

    ForceNetUpdate();

    MulticastPlayPrismMontage(bEquip);
}

void ABaseCharacter::OnRep_PrismEquipped()
{
    EquippedPrismMesh->SetVisibility(bPrismEquipped);

    bPrismPoseActive = bPrismEquipped;

    if (bPrismEquipped)
    {
        EquippedLanternMesh->SetVisibility(false);
        LanternLight->SetVisibility(false);
    }

    if (CharacterType == ECharacterType::Paladin)
    {
        if (RightHandWeapon)
        {
            RightHandWeapon->SetActorHiddenInGame(
                bPrismEquipped);

            RightHandWeapon->SetActorEnableCollision(
                !bPrismEquipped);
        }
    }
}

void ABaseCharacter::RefreshPrismState()
{
    OnRep_PrismEquipped();

    ForceNetUpdate();
}

void ABaseCharacter::MulticastPlayPrismMontage_Implementation(bool bEquip)
{
    if (bEquip)
    {
        if (PrismEquipMontage)
        {
            PlayAnimMontage(PrismEquipMontage);
        }
    }
    else
    {
        if (PrismUnequipMontage)
        {
            PlayAnimMontage(PrismUnequipMontage);
        }
    }
}

void ABaseCharacter::ServerPickupPrism_Implementation(ADungeonPrism* Prism)
{
    if (!Prism)
    {
        return;
    }

    bHasPrism = true;

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->bHasPrism = true;

        UE_LOG(LogTemp, Warning,
            TEXT("PlayerState Prism Saved : %d"),
            PS->bHasPrism);
    }

    OnRep_HasPrism();

    int32 RaidClearCount = INDEX_NONE;
    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bHasPrism = true;

        GI->ClearCurrentDungeon();
        RaidClearCount = GI->ClearedDungeonCount;
    }

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->NotifyPrismCollected();
        // GameInstance owns the authoritative raid count across map travel.
        if (RaidClearCount != INDEX_NONE)
        {
            PS->ClearedDungeonCount = RaidClearCount;
        }
        ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
    }

    Prism->SpawnReturnPortal();

    Prism->Destroy();

    NearbyPrism = nullptr;
}

void ABaseCharacter::ServerInteractPortal_Implementation(APortal* Portal)
{
    if (!Portal)
    {
        return;
    }

    if (Portal->PortalType == EPortalType::Boss)
    {
        if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
        {
            PS->NotifyBossPortalEntered();
        }
    }
    else if (Portal->PortalType == EPortalType::ReturnVillage)
    {
        if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
        {
            PS->NotifyReturnedToVillage();
        }
    }

    Portal->Interact(this);
}

void ABaseCharacter::ServerInteractDungeonPortal_Implementation()
{
    UE_LOG(LogTemp, Warning,
        TEXT("ServerInteractDungeonPortal"));

	if (!NearbyDungeonPortal)
	{
		return;
	}

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->NotifyDungeonEntered();
    }

	PrepareForPortalTravel();
	NearbyDungeonPortal->ServerEnterDungeon();
}

void ABaseCharacter::ServerInteractQuestGiver_Implementation(AQuestGiver* QuestGiver)
{
    if (QuestGiver)
    {
        QuestGiver->Interact(this);
    }
}

void ABaseCharacter::ClientShowQuestMessage_Implementation(const FString& Message)
{
    EnsureQuestWidget();
    if (QuestWidget)
    {
        QuestWidget->ShowObjective(FText::FromString(Message));
    }
}

void ABaseCharacter::EnsureQuestWidget()
{
    if (QuestWidget)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    // Character Blueprints can preserve old null defaults after a new native
    // property is added.  Load the authored WBP explicitly in that case.
    if (!QuestWidgetClass)
    {
        QuestWidgetClass = LoadClass<UQuestWidget>(
            nullptr,
            TEXT("/Game/BP_Character/WBP_Quest.WBP_Quest_C"));
    }

    TSubclassOf<UQuestWidget> ClassToCreate = QuestWidgetClass;
    if (!ClassToCreate)
    {
        // The native widget builds the same layout, so the objective remains
        // usable even if the asset was moved or failed to load.
        ClassToCreate = UQuestWidget::StaticClass();
    }

    QuestWidget = CreateWidget<UQuestWidget>(PC, ClassToCreate);
    if (QuestWidget)
    {
        QuestWidget->AddToViewport(50);
    }
}

void ABaseCharacter::SetNearbyQuestGiver(AQuestGiver* QuestGiver)
{
    NearbyQuestGiver = QuestGiver;
}

void ABaseCharacter::PrepareForPortalTravel()
{
	if (!HasAuthority())
	{
		return;
	}

	bLanternEquipped = false;
	bLanternPoseActive = false;
	bIsEquippingLantern = false;
	if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		PS->bLanternEquipped = false;
	}

	OnRep_LanternEquipped();
	ForceNetUpdate();
}

void ABaseCharacter::ServerPickupCoin_Implementation(ACoin* CoinActor)
{
    if (!CoinActor)
    {
        return;
    }

    Coin++;

    OnRep_Coin();

    CoinActor->Destroy();
}

void ABaseCharacter::OnRep_Coin()
{
    if (HUDWidget)
    {
        HUDWidget->UpdateCoin(Coin);
    }
}

void ABaseCharacter::OnRep_PotionCount()
{
    Slot2Icon = PotionCount > 0 ? PotionIcon : EmptySlotIcon;
}

void ABaseCharacter::OnRep_HasLantern()
{
    if (bHasLantern)
    {
        Slot1Icon = LanternIcon;
    }
    else
    {
        Slot1Icon = EmptySlotIcon;
    }

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bHasLantern = bHasLantern;
    }
}

void ABaseCharacter::OnRep_HasPrism()
{
    if (bHasPrism)
    {
        Slot3Icon = PrismIcon;
    }
    else
    {
        Slot3Icon = EmptySlotIcon;
    }

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bHasPrism = bHasPrism;
    }
}

void ABaseCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        bHasLantern = PS->bHasLantern;
        bLanternEquipped = PS->bLanternEquipped;

        bHasPrism = PS->bHasPrism;
        bPrismEquipped = PS->bPrismEquipped;

        UE_LOG(LogTemp, Warning,
            TEXT("Restore Item From PlayerState Lantern=%d Equipped=%d"),
            bHasLantern,
            bLanternEquipped);

        UE_LOG(LogTemp, Warning,
            TEXT("OnRep_PlayerState Finished"));
    }

    if (bHasLantern)
    {
        TArray<AActor*> Lanterns;

        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ALantern::StaticClass(),
            Lanterns);

        for (AActor* Actor : Lanterns)
        {
            Actor->Destroy();
        }

        UE_LOG(LogTemp, Warning,
            TEXT("Removed World Lantern"));
    }
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        bHasLantern = PS->bHasLantern;
        bLanternEquipped = PS->bLanternEquipped;

        bHasPrism = PS->bHasPrism;
        bPrismEquipped = PS->bPrismEquipped;

        UE_LOG(LogTemp, Warning,
            TEXT("Server Restore Item Lantern=%d"),
            bHasLantern);
    }

    if (bHasLantern)
    {
        TArray<AActor*> Lanterns;

        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ALantern::StaticClass(),
            Lanterns);

        for (AActor* Actor : Lanterns)
        {
            Actor->Destroy();
        }
    }
}

void ABaseCharacter::OnRep_DarknessDebuff()
{
    UE_LOG(LogTemp, Warning,
        TEXT("Darkness : %d"),
        bDarknessDebuff);
}

void ABaseCharacter::DebugBossPattern1()
{
    ServerDebugBossPattern(0);
}

void ABaseCharacter::DebugBossPattern2()
{
    ServerDebugBossPattern(1);
}

void ABaseCharacter::DebugBossPattern3()
{
    ServerDebugBossPattern(2);
}

void ABaseCharacter::DebugBossPattern4()
{
    ServerDebugBossPattern(3);
}

void ABaseCharacter::DebugTeleportToDungeonPortal()
{
    ServerDebugTeleportToDungeonPortal();
}

void ABaseCharacter::ServerDebugTeleportToDungeonPortal_Implementation()
{
    ADungeonPortal* ClosestPortal = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();

    for (TActorIterator<ADungeonPortal> It(GetWorld()); It; ++It)
    {
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestPortal = *It;
        }
    }

    if (!ClosestPortal)
    {
        UE_LOG(LogTemp, Warning, TEXT("Debug portal teleport failed: no dungeon portal exists."));
        return;
    }

    const FVector Destination = ClosestPortal->GetActorLocation()
        + FVector(350.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    TeleportTo(Destination, GetActorRotation(), false, true);
    UE_LOG(LogTemp, Warning, TEXT("Debug teleported beside dungeon portal: %s"), *Destination.ToString());
}

void ABaseCharacter::ServerDebugBossPattern_Implementation(uint8 PatternIndex)
{
    TArray<AActor*> Bosses;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ADragonBoss::StaticClass(),
        Bosses);

    if (Bosses.Num() == 0)
    {
        return;
    }

    ADragonBoss* Boss =
        Cast<ADragonBoss>(Bosses[0]);

    if (!Boss)
    {
        return;
    }

    switch (PatternIndex)
    {
    case 0:
        Boss->DebugBite();
        break;

    case 1:
        Boss->DebugCloseBreath();
        break;

    case 2:
        Boss->DebugBreath();
        break;

    case 3:
        Boss->DebugDebuff();
        break;
    }
}

void ABaseCharacter::HealOverTimeTick()
{
    HealTickCount++;

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere =
        FCollisionShape::MakeSphere(ERadius);

    GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere);

    TSet<ABaseCharacter*> Players;

    for (auto& Result : Overlaps)
    {
        ABaseCharacter* Player =
            Cast<ABaseCharacter>(Result.GetActor());

        if (!Player)
            continue;

        if (Players.Contains(Player))
            continue;

        Players.Add(Player);

        constexpr float TotalHealTicks = 5.f;
        const float Heal = Player->MaxHP * HealAmount / TotalHealTicks;

        Player->HealPlayer(Heal);
    }

    if (HealTickCount >= 5)
    {
        GetWorldTimerManager().ClearTimer(
            HealOverTimeHandle);

        HealTickCount = 0;
    }
}

void ABaseCharacter::CheckDarknessDamage()
{
    if (!HasAuthority())
    {
        return;
    }

    if (IsDead())
    {
        return;
    }

    if (bInsideLanternLight)
    {
        return;
    }

    const float Damage = MaxHP * 0.02f;

    TakePlayerDamage(Damage);
}

