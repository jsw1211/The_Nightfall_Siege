// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
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
#include "DungeonManager.h"
#include "QuestGiver.h"
#include "QuestWidget.h"
#include "QuestDialogueWidget.h"
#include "AltarProgressWidget.h"
#include "Animation/AnimSequenceBase.h"
#include "Coin.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "InventoryItemSlotWidget.h"
#include "BasePlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "The_Nightfall_SiegeGameMode.h"
#include "VillageManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

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

    // Keep the authored quest dialogue widget as a hard reference. The
    // runtime LoadClass fallback below is not discoverable by the cooker, so
    // without this reference the widget can be absent from packaged builds.
    static ConstructorHelpers::FClassFinder<UQuestDialogueWidget> QuestDialogueWidgetBP(
        TEXT("/Game/BP_Character/WBP_QuestDialogue"));
    if (QuestDialogueWidgetBP.Succeeded())
    {
        QuestDialogueWidgetClass = QuestDialogueWidgetBP.Class;
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

    EquippedLanternMesh->SetCastShadow(false);

    EquippedPrismMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedPrismMesh"));

    EquippedPrismMesh->SetupAttachment(GetMesh(), TEXT("PrismSocket"));

    EquippedPrismMesh->SetVisibility(false);

    LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));

    LanternLight->SetupAttachment(EquippedLanternMesh);

	// This light is enabled while the item is equipped, so it must be dynamic.
	LanternLight->SetMobility(EComponentMobility::Movable);

    LanternLight->SetCastShadows(true);

	LanternSafeZoneDecal = CreateDefaultSubobject<UDecalComponent>(
		TEXT("LanternSafeZoneDecal"));
	LanternSafeZoneDecal->SetupAttachment(GetCapsuleComponent());
	LanternSafeZoneDecal->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	LanternSafeZoneDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	LanternSafeZoneDecal->DecalSize = FVector(200.f, 1800.f, 1800.f);
	LanternSafeZoneDecal->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LanternSafeZoneMaterial(
		TEXT("/Game/Effects/Lantern/M_Lantern_Sanctuary.M_Lantern_Sanctuary"));
	if (LanternSafeZoneMaterial.Succeeded())
	{
		LanternSafeZoneDecal->SetDecalMaterial(LanternSafeZoneMaterial.Object);
	}

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

    LanternLightSphere->SetSphereRadius(720.f);

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

    // Keep the visual light exactly aligned with the gameplay safe zone.
    // LanternLightSphere is the authoritative darkness-protection radius.
    LanternLight->SetIntensity(50.f);

    LanternLight->SetAttenuationRadius(1680.f);

    LanternLight->SetUseInverseSquaredFalloff(false);

    LanternLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.0f));

    UE_LOG(LogTemp, Warning, TEXT("%s"),
        *LanternLight->GetLightColor().ToString());

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealEffectAsset(
        TEXT("/Game/Effects/1_Heal/NS_Heal.NS_Heal"));

    if (HealEffectAsset.Succeeded())
    {
        HealEffect = HealEffectAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HPBuffEffectAsset(
        TEXT("/Game/Effects/Buff/HP/NS_HP_Buff"));

    if (HPBuffEffectAsset.Succeeded())
    {
        HPBuffEffect = HPBuffEffectAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> AttackBuffEffectAsset(
        TEXT("/Game/Effects/Buff/attack/NS_Attack_Buff"));

    if (AttackBuffEffectAsset.Succeeded())
    {
        AttackBuffEffect = AttackBuffEffectAsset.Object;
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

	// Blueprint defaults must not desynchronise the visible lantern radius from
	// the sphere that grants darkness protection.
	LanternLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.0f));
	LanternLight->SetAttenuationRadius(1680.f);
	const float LanternSafeRadius = LanternLightSphere->GetScaledSphereRadius();
	const FVector LanternDecalScale = LanternSafeZoneDecal->GetComponentScale();
	LanternSafeZoneDecal->DecalSize = FVector(
		200.f,
		LanternSafeRadius / FMath::Max(FMath::Abs(LanternDecalScale.Y), KINDA_SMALL_NUMBER),
		LanternSafeRadius / FMath::Max(FMath::Abs(LanternDecalScale.Z), KINDA_SMALL_NUMBER));

    // A retry travels to the village and creates a fresh pawn.  Restore every
    // local/server movement and input restriction that death may have set.
    bIsDead = false;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    // Cover the opposite spawn order from AMonster::BeginPlay. Movement-ignore
    // lists are local state, so this intentionally runs on server and clients.
    for (TActorIterator<AMonster> It(GetWorld()); It; ++It)
    {
        MoveIgnoreActorAdd(*It);
        It->MoveIgnoreActorAdd(this);
    }

    if (APlayerController* RespawnController = Cast<APlayerController>(GetController()))
    {
        EnableInput(RespawnController);
        if (ABaseController* BaseRespawnController = Cast<ABaseController>(RespawnController))
        {
            BaseRespawnController->ClearDeathRestrictions();
        }
    }

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
        QRadius = 120.f;

        // W
        DefenseRate = 0.f;
        PaladinWDefenseRate = 0.f;

        // E
        HealAmount = 0.1f;

        // R
        RHealAmount = 0.2f;

        break;

    case ECharacterType::Archer:

        MaxHP = 300.f;
        AttackPower = 200.f;

        QMultiplier = 1.5f;
        EMultiplier = 1.0f;
        RMultiplier = 3.0f;

        AttackSpeed = 1.0f;

        DefaultAttackSpeed = 1.0f;

        break;

    case ECharacterType::Warrior:

        MaxHP = 400.f;
        AttackPower = 300.f;

		QMultiplier = 1.0f;
		EMultiplier = 1.0f;
		QRadius = 120.f;
		WarriorERadius = 300.f;

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
    const bool bIsLobbyMap = MapName.Contains(TEXT("Lvl_Lobby"));

    // The lobby is exclusively for character selection and ready status.
    // Gameplay HUD and quest progress begin only after travelling to the game.
    if (!bIsLobbyMap && HUDWidgetClass)
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

    if (!bIsLobbyMap)
    {
        EnsureQuestWidget();

        // Controllers survive lobby-to-village travel, but the lobby UI may
        // leave its input focus behind.  Reapply the cursor-directed gameplay
        // mode after this newly spawned pawn becomes the controlled character.
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (PC->IsLocalController())
            {
                FInputModeGameOnly InputMode;
                InputMode.SetConsumeCaptureMouseDown(false);
                PC->SetInputMode(InputMode);
                PC->SetShowMouseCursor(true);
                PC->SetIgnoreMoveInput(false);
                PC->SetIgnoreLookInput(true);
            }
        }
    }

    Slot1Icon = EmptySlotIcon;
    Slot2Icon = EmptySlotIcon;
    Slot3Icon = EmptySlotIcon;
    Slot4Icon = EmptySlotIcon;

    if (HasAuthority())
    {
        if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
        {
            PotionCount = PS->PotionCount;
			PurchasedItems = PS->PurchasedItems;
			Slot4PurchasedItemIndex = PurchasedItems.IsValidIndex(PS->Slot4PurchasedItemIndex)
				? PS->Slot4PurchasedItemIndex
				: INDEX_NONE;
			Coin = PS->Coin;
        }
    }

    OnRep_PotionCount();
	OnRep_PurchasedItems();
	OnRep_Slot4PurchasedItemIndex();
	OnRep_Coin();

	if (ABasePlayerState* SkillPS = GetPlayerState<ABasePlayerState>())
	{
		SkillPoints = SkillPS->SkillPoints;
		SkillPS->CopySkillLevelsTo(SkillLevels);
	}
	else if (UTheNightfallSiegeInstance* GI =
		Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
	{
		// Temporary pre-possession fallback only. PlayerState becomes the
		// authoritative per-player store as soon as the pawn is possessed.
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
        bLanternPoseActive = true;
        SetItemAnimationState(EItemAnimationState::LanternIdle);
    }

    if (bPrismEquipped)
    {
        OnRep_PrismEquipped();
        bPrismPoseActive = true;
        SetItemAnimationState(EItemAnimationState::PrismIdle);
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

    const FString DarknessMapName = GetWorld()->GetMapName();
    if (HasAuthority() &&
        (DarknessMapName.Contains(TEXT("Village")) || DarknessMapName.Contains(TEXT("Dungeon"))))
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

    if (HasAuthority())
    {
        if (AThe_Nightfall_SiegeGameMode* GameMode = GetWorld()->GetAuthGameMode<AThe_Nightfall_SiegeGameMode>())
        {
            GameMode->HandlePlayerDeath(this);
        }
    }

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

        if (ABaseController* BaseController = Cast<ABaseController>(PC))
        {
            BaseController->ShowDeathScreen(false);
        }
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

bool ABaseCharacter::CanBeBaseForCharacter(APawn* Pawn) const
{
    // Reject only the two enemy character families requested here. Keeping the
    // capsule's normal slope rules lets CharacterMovement land momentarily and
    // invoke its built-in JumpOff path instead of hovering on an unwalkable top.
    if (Pawn && (Pawn->IsA<AMonster>() || Pawn->IsA<ADragonBoss>()))
    {
        return false;
    }

    return Super::CanBeBaseForCharacter(Pawn);
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// This game is mouse-directed, so never hide the cursor after UI transitions.
	if (IsLocallyControlled())
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
    // Niagara component activation is not replicated automatically. Let the
    // server choose one shared target and multicast the visual state instead
    // of relying on each client's local Tick to activate the component.
    if (!HasAuthority() || !LanternDirectionEffectComponent)
    {
        return;
    }

    const bool bIsVillage = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("Village_Forest"));
    const bool bIsDungeon = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("Dungeon"));

    // Guide one target only: the village portal, or the nearest unfinished
    // altar in a dungeon.
    // Every machine updates the cosmetic guide for replicated characters.
    // Restricting this to IsLocallyControlled() made the listen-server host's
    // guide invisible on remote clients.
    const bool bCanGuide = bLanternEquipped
		&& bLanternGuideReady
        && (bIsVillage || bIsDungeon);

    if (!bCanGuide)
    {
        if (LanternDirectionEffectComponent->IsActive()
            || LanternDirectionEffectComponent->IsVisible())
        {
            MulticastSetLanternDirectionEffect(false, FRotator::ZeroRotator);
        }
        LanternDirectionUpdateElapsed = 0.f;
        return;
    }

    LanternDirectionUpdateElapsed += DeltaTime;
    if (LanternDirectionUpdateElapsed < 0.15f)
    {
        return;
    }
    LanternDirectionUpdateElapsed = 0.f;

    AActor* GuideTarget = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();

    if (bIsVillage)
    {
        // A boss portal always wins over ordinary dungeon portals. This is
        // deliberately independent of whether old dungeon portals remain.
        for (TActorIterator<APortal> It(GetWorld()); It; ++It)
        {
            if (It->PortalType != EPortalType::Boss)
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                GuideTarget = *It;
            }
        }

        // Only look for ordinary dungeon portals while no boss portal exists.
        if (!GuideTarget)
        {
        for (TActorIterator<ADungeonPortal> It(GetWorld()); It; ++It)
        {
            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                GuideTarget = *It;
            }
        }
        }
    }
    else if (bIsDungeon)
    {
        for (TActorIterator<AAltar> It(GetWorld()); It; ++It)
        {
            if (!It->IsAvailableNavigationTarget()) continue;

            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                GuideTarget = *It;
            }
        }
    }

    if (!GuideTarget)
    {
        if (LanternDirectionEffectComponent->IsActive()
            || LanternDirectionEffectComponent->IsVisible())
        {
            MulticastSetLanternDirectionEffect(false, FRotator::ZeroRotator);
        }
        return;
    }

    // Calculate from the lantern itself so the visible trail points from the
    // held light directly to the selected portal or altar.
    FVector Direction = GuideTarget->GetActorLocation()
        - LanternDirectionEffectComponent->GetComponentLocation();
    Direction.Z = 0.f;
    if (Direction.IsNearlyZero())
    {
        return;
    }

    MulticastSetLanternDirectionEffect(
        true,
        Direction.Rotation() + LanternDirectionRotationOffset);
}

void ABaseCharacter::MulticastSetLanternDirectionEffect_Implementation(
    bool bVisible,
    FRotator WorldRotation)
{
    if (!LanternDirectionEffectComponent)
    {
        return;
    }

    if (!bVisible)
    {
        LanternDirectionEffectComponent->Deactivate();
        LanternDirectionEffectComponent->SetVisibility(false);
        return;
    }

    LanternDirectionEffectComponent->SetWorldRotation(WorldRotation);
    LanternDirectionEffectComponent->SetVisibility(true, true);
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

        if (IsDeveloperHost())
        {
            EnhancedInput->BindAction(IA_Debug1, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern1);
            EnhancedInput->BindAction(IA_Debug2, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern2);
            EnhancedInput->BindAction(IA_Debug3, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern3);
            EnhancedInput->BindAction(IA_Debug4, ETriggerEvent::Started, this, &ABaseCharacter::DebugBossPattern4);
        }
    }

    // The shop is deliberately bound directly so it works without requiring a
    // new Input Action asset to be configured in every character Blueprint.
    PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABaseCharacter::ToggleShop);
    PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ABaseCharacter::InteractWithQuestGiver);
#if !UE_BUILD_SHIPPING
    PlayerInputComponent->BindKey(
        EKeys::B,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugBossCenterMechanic
    );

    PlayerInputComponent->BindKey(
        EKeys::Equals,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugTeleportToDungeonPortal
    );

    PlayerInputComponent->BindKey(
        EKeys::Hyphen,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugCompleteRaid
    );

    PlayerInputComponent->BindKey(
        EKeys::Nine,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugEnableMoveSpeed
    );

    PlayerInputComponent->BindKey(
        EKeys::Zero,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugResetMoveSpeed
    );

    PlayerInputComponent->BindKey(
        EKeys::Backslash,
        IE_Pressed,
        this,
        &ABaseCharacter::DebugSetGold1000
    );
#endif

    if (bIsDead) return; // 죽으면 입력 등록 안함
}

void ABaseCharacter::Attack(const FInputActionValue& Value)
{
    if (bInventoryOpen) return;
    ServerCancelPotionUse();
    if (!CanUseCombatAction())
    {
        return;
    }

    RotateToMouseCursor();

    FRotator TargetRotation = GetActorRotation();
    ServerAttack(TargetRotation);

}

void ABaseCharacter::Q(const FInputActionValue& Value)
{
    if (bInventoryOpen) return;
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
    if (bInventoryOpen) return;
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
    if (bInventoryOpen) return;
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
    if (bInventoryOpen) return;
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
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                InputMode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(InputMode);
                PC->SetShowMouseCursor(true);
                // The inventory is a mouse-only overlay.  Do not give it
                // keyboard focus or it consumes WASD and leaves the player
                // unexpectedly unable to move.
                PC->SetIgnoreMoveInput(false);
                PC->SetIgnoreLookInput(false);
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
                FInputModeGameOnly InputMode;
                InputMode.SetConsumeCaptureMouseDown(false);
                PC->SetInputMode(InputMode);
                PC->SetShowMouseCursor(true);
				// Closing the inventory must restore exactly the same movement
				// state as opening it: inventory never owns a movement lock.
				PC->SetIgnoreMoveInput(false);
				PC->SetIgnoreLookInput(false);
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
        // The shop is available only while standing in NPC1's interaction range.
        if (!NearbyQuestGiver || !NearbyQuestGiver->bIsShopkeeper)
        {
            return;
        }

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
        UpdateShopGold();
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

    // The shop no longer owns input, so restore gameplay focus immediately.
    FInputModeGameOnly InputMode;
    InputMode.SetConsumeCaptureMouseDown(false);
    PC->SetInputMode(InputMode);
    PC->SetShowMouseCursor(true);
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
	if (bIsUsingPotion || !PurchasedItems.IsValidIndex(ItemIndex))
    {
        return;
    }

    const EShopItemType Type = PurchasedItems[ItemIndex].ItemType;
    if (Type != EShopItemType::HPPotion && Type != EShopItemType::AttackPotion)
    {
        return;
    }

    Slot4PurchasedItemIndex = ItemIndex;
	SaveInventoryToPlayerState();
    OnRep_Slot4PurchasedItemIndex();
    ForceNetUpdate();
}

void ABaseCharacter::ServerMovePurchasedItem_Implementation(int32 FromIndex, int32 ToIndex)
{
	if (bIsUsingPotion || FromIndex == ToIndex || !PurchasedItems.IsValidIndex(FromIndex) || !PurchasedItems.IsValidIndex(ToIndex))
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

	SaveInventoryToPlayerState();

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

    // The pawn is replaced on map travel.  Keep the authoritative PlayerState
    // in sync so the balance follows this player into the next map.
    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->Coin = Coin;
        PS->ForceNetUpdate();
    }

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
        if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
        {
            PS->PotionCount = PotionCount;
        }
        OnRep_PotionCount();
    }

	SaveInventoryToPlayerState();

    OnRep_Coin();
    OnRep_PurchasedItems();
    ForceNetUpdate();
}

void ABaseCharacter::OnRep_PurchasedItems()
{
    RefreshInventoryWidget();
}

void ABaseCharacter::SaveInventoryToPlayerState()
{
	if (!HasAuthority())
	{
		return;
	}

	if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		PS->PotionCount = PotionCount;
		PS->PurchasedItems = PurchasedItems;
		PS->Slot4PurchasedItemIndex = Slot4PurchasedItemIndex;
		PS->ForceNetUpdate();
	}
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

    constexpr int32 SlotCount = 12;

    for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
    {
        const FString ImageName =
            FString::Printf(TEXT("Image_%d"), SlotIndex);

        const FString TextName =
            FString::Printf(TEXT("Text_%d"), SlotIndex);

        const FString SlotInputName =
            FString::Printf(TEXT("SlotInput_%d"), SlotIndex);

        UImage* SlotImage =
            Cast<UImage>(
                InventoryWidget->GetWidgetFromName(*ImageName)
            );

        UTextBlock* SlotText =
            Cast<UTextBlock>(
                InventoryWidget->GetWidgetFromName(*TextName)
            );

        UInventoryItemSlotWidget* SlotInput =
            Cast<UInventoryItemSlotWidget>(
                InventoryWidget->GetWidgetFromName(*SlotInputName)
            );

        if (!SlotImage || !SlotText || !SlotInput)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Inventory slot %d setup failed. Image=%s Text=%s SlotInput=%s"),
                SlotIndex,
                SlotImage ? TEXT("OK") : TEXT("MISSING"),
                SlotText ? TEXT("OK") : TEXT("MISSING"),
                SlotInput ? TEXT("OK") : TEXT("MISSING")
            );

            continue;
        }

        if (PurchasedItems.IsValidIndex(SlotIndex))
        {
            const FShopInventoryItem& Item =
                PurchasedItems[SlotIndex];

            const FText SlotName =
                FText::FromString(
                    FString::Printf(
                        TEXT("%s x%d"),
                        *Item.DisplayName.ToString(),
                        Item.Quantity
                    )
                );

            // 슬롯의 드래그/더블클릭/드롭 데이터 설정
            SlotInput->Configure(this, SlotIndex, Item.Icon.Get());

            // WBP에서 직접 만든 Image/Text에 데이터만 넣음
            SlotImage->SetBrushFromTexture(Item.Icon.Get());
            SlotText->SetText(SlotName);

            SlotImage->SetVisibility(ESlateVisibility::Visible);
            SlotText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            // 빈 슬롯
            SlotInput->ConfigureEmpty();

            SlotImage->SetBrushFromTexture(nullptr);
            SlotText->SetText(FText::GetEmpty());

            SlotImage->SetVisibility(ESlateVisibility::Collapsed);
            SlotText->SetVisibility(ESlateVisibility::Collapsed);
        }
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

void ABaseCharacter::UpdateShopGold()
{
    if (!ShopWidget)
    {
        return;
    }

    if (UTextBlock* GoldText = Cast<UTextBlock>(
        ShopWidget->GetWidgetFromName(TEXT("GoldText"))))
    {
        GoldText->SetText(FText::AsNumber(Coin));
    }
}

void ABaseCharacter::ToggleSkillTree()
{
    if (!SkillTreeWidgetClass)
        return;

    if (!bSkillTreeOpen)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (!PC || !PC->IsLocalController())
        {
            return;
        }

        SkillTreeWidget = CreateWidget<USkillTreeWidget>(PC, SkillTreeWidgetClass);

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
    // All combat damage is server-authoritative.  This also protects against
    // an accidental client-side caller changing predicted HP.
    if (!HasAuthority() || bIsDead) return;

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

    CompleteHeldItemAnimation(Montage, bInterrupted);

    if (IsLocallyControlled() && Montage == LanternEquipMontage && !bInterrupted && bLanternEquipped)
    {
        bLanternGuideReady = true;
        LanternDirectionUpdateElapsed = 0.15f;
    }
}

void ABaseCharacter::SetItemAnimationState(EItemAnimationState NewState)
{
    ItemAnimationState = NewState;

    OnRep_ItemAnimationState();

    if (HasAuthority())
    {
        ForceNetUpdate();
    }
}

void ABaseCharacter::OnRep_ItemAnimationState()
{
    // ItemAnimationState is replicated, so it is also the animation-complete
    // signal for simulated proxies. Re-evaluating the complete condition here
    // and in OnRep_LanternEquipped makes this independent of property arrival
    // order on clients.
    bLanternGuideReady = bLanternEquipped
        && ItemAnimationState == EItemAnimationState::LanternIdle;

    if (bLanternGuideReady)
    {
        LanternDirectionUpdateElapsed = 0.15f;
    }
}

void ABaseCharacter::CompleteHeldItemAnimation(UAnimMontage* Montage, bool bInterrupted)
{
    // Gameplay and mesh visibility are authoritative here. The AnimBP has
    // already played its matching transition sequence while this completion
    // advances its state machine to Idle or None.
    if (Montage == LanternEquipMontage)
    {
        bIsEquippingLantern = false;
        if (bLanternEquipped)
        {
            bLanternPoseActive = true;
            SetItemAnimationState(EItemAnimationState::LanternIdle);
        }
    }
    else if (Montage == LanternUnequipMontage)
    {
        bIsEquippingLantern = false;
        if (!bLanternEquipped)
        {
            bLanternPoseActive = false;
            SetItemAnimationState(EItemAnimationState::None);
        }
    }
    else if (Montage == PrismEquipMontage)
    {
        bIsEquippingPrism = false;
        if (bPrismEquipped)
        {
            bPrismPoseActive = true;
            SetItemAnimationState(EItemAnimationState::PrismIdle);
        }
    }
    else if (Montage == PrismUnequipMontage)
    {
        bIsEquippingPrism = false;
        if (!bPrismEquipped)
        {
            bPrismPoseActive = false;
            SetItemAnimationState(EItemAnimationState::None);
        }
    }

    // An interrupted held-item montage still needs a coherent final pose. The
    // equipped state above is authoritative, rather than the interruption.
    (void)bInterrupted;
}

void ABaseCharacter::CompleteLanternItemAnimation()
{
    CompleteHeldItemAnimation(
        bLanternEquipped ? LanternEquipMontage : LanternUnequipMontage,
        false);
}

void ABaseCharacter::CompletePrismItemAnimation()
{
    CompleteHeldItemAnimation(
        bPrismEquipped ? PrismEquipMontage : PrismUnequipMontage,
        false);
}

void ABaseCharacter::StartLanternItemAnimationTimer()
{
    UAnimMontage* ActiveMontage = bLanternEquipped
        ? LanternEquipMontage
        : LanternUnequipMontage;

    if (!ActiveMontage)
    {
        CompleteLanternItemAnimation();
        return;
    }

    GetWorldTimerManager().SetTimer(
        LanternItemAnimationTimer,
        this,
        &ABaseCharacter::CompleteLanternItemAnimation,
        ActiveMontage->GetPlayLength(),
        false);
}

void ABaseCharacter::StartPrismItemAnimationTimer()
{
    UAnimMontage* ActiveMontage = bPrismEquipped
        ? PrismEquipMontage
        : PrismUnequipMontage;

    if (!ActiveMontage)
    {
        CompletePrismItemAnimation();
        return;
    }

    GetWorldTimerManager().SetTimer(
        PrismItemAnimationTimer,
        this,
        &ABaseCharacter::CompletePrismItemAnimation,
        ActiveMontage->GetPlayLength(),
        false);
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

void ABaseCharacter::RefreshRightHandWeaponForHeldItem()
{
    if (!RightHandWeapon)
    {
        return;
    }

    // Lantern and prism states are replicated. Calling this from both OnRep
    // handlers applies the same visibility to the host, owning client, and
    // simulated proxies, regardless of which property arrives first.
    const bool bHideWeapon = bLanternEquipped || bPrismEquipped;
    RightHandWeapon->SetActorHiddenInGame(bHideWeapon);
    RightHandWeapon->SetActorEnableCollision(!bHideWeapon);
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
		PS->SkillPoints = SkillPoints;
		PS->SetSkillLevelsFrom(SkillLevels);
        PS->NotifySkillPointSpent();
		PS->ForceNetUpdate();
    }

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
		// GameInstance is process-wide on a listen server. Only mirror the
		// locally controlled host here; remote clients persist in PlayerState.
		if (IsLocallyControlled())
		{
			GI->SkillPoints = SkillPoints;
			GI->SkillLevels = SkillLevels;
		}
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
                PaladinWDefenseRate = 0.2f;
            }
            else if (SkillLevel == 3)
            {
                PaladinWDefenseRate = 0.4f;
            }
            else if (SkillLevel == 4)
            {
                PaladinWDefenseRate = 0.6f;
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
                ERadius = 700.f * 1.2f;
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
	HandleWorldInteraction();
}

void ABaseCharacter::HandleWorldInteraction()
{
    // F키 한 번으로 여러 상호작용이 연속 처리되는 것을 방지
    if (bInteractionLocked)
    {
        return;
    }

    // 이번 F 입력을 상호작용으로 소비
    bInteractionLocked = true;

    GetWorldTimerManager().SetTimer(
        InteractionLockTimer,
        this,
        &ABaseCharacter::ResetInteractionLock,
        0.2f,
        false);

    if (bIsPlacingLantern)
    {
        return;
    }

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
        if (HasAuthority())
        {
            RequestGroupPrismCleanse();
        }
        else
        {
            ServerRequestGroupPrismCleanse();
        }

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
        return;
    }

    // F is also the direct keyboard interaction key.  Keep quest dialogue as
    // its highest-priority use, but let the same key reach the normal world
    // interaction path when no quest giver is in range (including the dragon
    // blackout prism cleanse).
    HandleWorldInteraction();
}

void ABaseCharacter::UseSlot1(const FInputActionValue& Value)
{
    if (bInventoryOpen) return;
    ServerCancelPotionUse();
    UE_LOG(LogTemp, Warning,
        TEXT("UseSlot1 HasLantern=%d Equipped=%d"),
        bHasLantern,
        bLanternEquipped);

    // A prism transition—including Unequipping—must finish before a lantern
    // action or an item switch can begin.
    if (bPrismEquipped || bIsEquippingPrism)
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
    if (bInventoryOpen) return;
    ServerUsePotion();
}

void ABaseCharacter::ServerUsePotion_Implementation()
{
    // Potions require both hands.  The player must put away the lantern or
    // prism before drinking, just as other held-item actions are gated.
    if (bIsUsingPotion || PotionCount <= 0 || CurrentHP >= MaxHP || bIsDead
        || bLanternEquipped || bPrismEquipped || bIsEquippingLantern || bIsEquippingPrism)
    {
        return;
    }

    bIsUsingPotion = true;
	SetItemAnimationState(EItemAnimationState::PotionUsing);
	PendingPurchasedPotionIndex = INDEX_NONE;
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
    // Keep movement enabled, but hide equipped weapons for the drinking pose.
    // They are restored when the potion completes or is interrupted.
    for (AActor* Weapon : { RightHandWeapon, LeftHandWeapon })
    {
        if (Weapon)
        {
            Weapon->SetActorHiddenInGame(true);
            Weapon->SetActorEnableCollision(false);
        }
    }

    // PotionSlot exists inside the Potion Pose layer, so this event-driven
    // montage restarts from frame zero without replacing the locomotion pose.
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (UAnimSequenceBase* PotionAnimation = GetPotionAnimation())
        {
            ActivePotionMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
                PotionAnimation,
                TEXT("PotionSlot"));
        }
    }
}

void ABaseCharacter::FinishPotionUse()
{
    if (!bIsUsingPotion)
    {
        return;
    }

    bIsUsingPotion = false;
	SetItemAnimationState(EItemAnimationState::None);
	const bool bIsHealingPotion = PendingPurchasedPotionIndex == INDEX_NONE;

	if (PendingPurchasedPotionIndex != INDEX_NONE)
	{
		const int32 ItemIndex = PendingPurchasedPotionIndex;
		PendingPurchasedPotionIndex = INDEX_NONE;

		// The item can only be consumed after its drinking animation completes.
		if (PurchasedItems.IsValidIndex(ItemIndex)
			&& PurchasedItems[ItemIndex].ItemType == PendingPurchasedPotionType
			&& PurchasedItems[ItemIndex].Quantity > 0)
		{
            if (PendingPurchasedPotionType == EShopItemType::HPPotion)
            {
                const float BonusHP = MaxHP * 0.2f;
                MaxHP += BonusHP;
                CurrentHP += BonusHP;
                OnRep_CurrentHP();

                if (HPBuffEffect)
                {
                    MulticastPlayHPBuffEffect(GetActorLocation());
                }
            }
            else if (PendingPurchasedPotionType == EShopItemType::AttackPotion)
            {
				// AttackPower can temporarily contain Warrior R's multiplier.  The
				// potion is permanent, so grow the unbuffed value and then rebuild
				// the currently visible value instead of persisting a temporary buff.
				BaseAttackPower *= 1.2f;
				const bool bWarriorRActive = CharacterType == ECharacterType::Warrior
					&& GetWorldTimerManager().IsTimerActive(WarriorRBuffHandle);
				AttackPower = BaseAttackPower * (bWarriorRActive
					? 1.0f + WarriorRDamageBonus
					: 1.0f);

                if (AttackBuffEffect)
                {
                    MulticastPlayAttackBuffEffect(GetActorLocation());
                }
            }

			if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
			{
				PS->bHasShopStatBonuses = true;
				PS->SavedMaxHP = MaxHP;
				PS->SavedAttackPower = BaseAttackPower;
				PS->ForceNetUpdate();
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
			SaveInventoryToPlayerState();
		}
	}
	else
	{
		--PotionCount;

		const int32 HealingItemIndex = PurchasedItems.IndexOfByPredicate(
			[](const FShopInventoryItem& Item)
			{
				return Item.ItemType == EShopItemType::HealPotion;
			});
		if (PurchasedItems.IsValidIndex(HealingItemIndex))
		{
			--PurchasedItems[HealingItemIndex].Quantity;
			if (PurchasedItems[HealingItemIndex].Quantity <= 0)
			{
				PurchasedItems.RemoveAt(HealingItemIndex);
				if (Slot4PurchasedItemIndex == HealingItemIndex)
				{
					Slot4PurchasedItemIndex = INDEX_NONE;
				}
				else if (Slot4PurchasedItemIndex > HealingItemIndex)
				{
					--Slot4PurchasedItemIndex;
				}
			}
		}

		if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
		{
			PS->PotionCount = PotionCount;
		}

		// 포션 애니메이션이 끝나는 순간 체력 회복
		HealPlayer(MaxHP * PotionHealPercent);
		OnRep_PotionCount();
		OnRep_PurchasedItems();
		OnRep_Slot4PurchasedItemIndex();
		SaveInventoryToPlayerState();
	}

	// The healing Niagara effect belongs only to the healing potion.  Stat
	// potions still use the drinking animation but do not visually heal.
	if (bIsHealingPotion && HealEffect)
	{
		MulticastPlayRHealEffect(GetActorLocation());
	}

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
	SetItemAnimationState(EItemAnimationState::None);
	PendingPurchasedPotionIndex = INDEX_NONE;
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

    for (AActor* Weapon : { RightHandWeapon, LeftHandWeapon })
    {
        if (Weapon)
        {
            Weapon->SetActorHiddenInGame(false);
            Weapon->SetActorEnableCollision(true);
        }
    }
}

void ABaseCharacter::UseSlot3(const FInputActionValue& Value)
{
    if (bInventoryOpen) return;
    ServerCancelPotionUse();
    if (bLanternEquipped || bIsEquippingLantern)
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
    if (bInventoryOpen) return;
    ServerUseSlot4();
}

void ABaseCharacter::ServerUseSlot4_Implementation()
{
    ServerUsePurchasedItem_Implementation(Slot4PurchasedItemIndex);
}

void ABaseCharacter::ServerUsePurchasedItem_Implementation(int32 ItemIndex)
{
	if (bIsUsingPotion || bIsDead || bLanternEquipped || bPrismEquipped
		|| bIsEquippingLantern || bIsEquippingPrism
		|| !PurchasedItems.IsValidIndex(ItemIndex) || PurchasedItems[ItemIndex].Quantity <= 0)
    {
        return;
    }

    const EShopItemType ItemType = PurchasedItems[ItemIndex].ItemType;
    if (ItemType != EShopItemType::HPPotion && ItemType != EShopItemType::AttackPotion)
    {
        return;
    }

	// Stat potions share the normal potion's animation, weapon hiding,
	// movement behavior, and cancellation rules.  Their effect is delayed
	// until the drink animation completes.
	bIsUsingPotion = true;
	SetItemAnimationState(EItemAnimationState::PotionUsing);
	PendingPurchasedPotionIndex = ItemIndex;
	PendingPurchasedPotionType = ItemType;
	MulticastStartPotionUse();
	const float AnimationDuration = GetPotionAnimation()
		? GetPotionAnimation()->GetPlayLength()
		: PotionUseDuration;
	GetWorldTimerManager().SetTimer(PotionUseTimer, this, &ABaseCharacter::FinishPotionUse,
		FMath::Max(AnimationDuration, KINDA_SMALL_NUMBER), false);
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

void ABaseCharacter::ResumeLanternGuidanceAfterAltar()
{
    if (bLanternEquipped)
    {
        bLanternGuideReady = true;
        LanternDirectionUpdateElapsed = 0.15f;
    }
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

    const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
    AArrowProjectile* Arrow = GetWorld()->SpawnActorDeferred<AArrowProjectile>(
        ArrowClass,
        SpawnTransform,
        this,
        this,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;
        Arrow->DamageMultiplier = 1.f;
        Arrow->FinishSpawning(SpawnTransform);
        if (IsValid(Arrow))
        {
            Arrow->SetupTrail();
        }
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

        const FTransform SpawnTransform(Rotation, SpawnLocation);
        AArrowProjectile* Arrow = GetWorld()->SpawnActorDeferred<AArrowProjectile>(
            ArrowClass,
            SpawnTransform,
            this,
            this,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

        if (Arrow)
        {
            Arrow->OwnerCharacter = this;
            Arrow->ArrowType = EArrowType::QExplosive;
            Arrow->QVolleyId = VolleyId;
            Arrow->DamageMultiplier = QMultiplier;
            // Direction is already in world space. ProjectileMovement otherwise
            // rotates it by the actor transform again during FinishSpawning.
            Arrow->ProjectileMovement->bInitialVelocityInLocalSpace = false;
            Arrow->ProjectileMovement->InitialSpeed = 0.f;
            Arrow->ProjectileMovement->MaxSpeed = 1000.f;
            Arrow->ProjectileMovement->Velocity = Direction * 1000.f;
            Arrow->FinishSpawning(SpawnTransform);
            if (IsValid(Arrow))
            {
                Arrow->SetupTrail();
            }
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

    const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
    AArrowProjectile* Arrow = GetWorld()->SpawnActorDeferred<AArrowProjectile>(
        ArrowClass,
        SpawnTransform,
        this,
        this,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;
        Arrow->ArrowType = EArrowType::Pierce;
        Arrow->DamageMultiplier = RMultiplier;
        Arrow->FinishSpawning(SpawnTransform);
        if (IsValid(Arrow))
        {
            Arrow->SetActorScale3D(FVector(3.f, 3.f, 3.f));
            Arrow->SetupTrail();
        }
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

    const FVector SpawnLocation = Bow->Mesh->GetSocketLocation(TEXT("ArrowSocket"));
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector TargetLocation = GetActorLocation() +
        Forward * ArcherEProjectileRange;

    FVector LaunchVelocity = FVector::ZeroVector;
    const float LaunchSpeed = FMath::Max(ArcherEProjectileSpeed, 1.f);
    UGameplayStatics::FSuggestProjectileVelocityParameters ProjectileParams(
        this,
        SpawnLocation,
        TargetLocation,
        LaunchSpeed);
    ProjectileParams.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;
    const bool bHasSuggestedVelocity = UGameplayStatics::SuggestProjectileVelocity(
        ProjectileParams,
        LaunchVelocity);

    if (!bHasSuggestedVelocity)
    {
        // A fixed-speed ballistic solution can fail when its requested range is
        // physically unreachable. Keep E deterministic instead of silently
        // using ProjectileMovement's 3000-unit straight default.
        const FVector ToTarget = TargetLocation - SpawnLocation;
        const float HorizontalDistance = ToTarget.Size2D();
        const float FlightTime = FMath::Max(HorizontalDistance / LaunchSpeed, 0.1f);
        LaunchVelocity = FVector(
            ToTarget.X / FlightTime,
            ToTarget.Y / FlightTime,
            (ToTarget.Z - 0.5f * GetWorld()->GetGravityZ() * FMath::Square(FlightTime)) /
                FlightTime);
    }

    if (LaunchVelocity.IsNearlyZero())
    {
        LaunchVelocity = Forward * LaunchSpeed;
    }

    const FTransform SpawnTransform(LaunchVelocity.Rotation(), SpawnLocation);
    AArrowProjectile* Arrow = GetWorld()->SpawnActorDeferred<AArrowProjectile>(
        ArrowClass,
        SpawnTransform,
        this,
        this,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (Arrow)
    {
        Arrow->OwnerCharacter = this;
        Arrow->ArrowType = EArrowType::Explosive;
        Arrow->DamageMultiplier = EMultiplier;
        Arrow->EExplosionRadius = 450.f;
        Arrow->TargetLocation = TargetLocation;
        Arrow->ProjectileMovement->ProjectileGravityScale = 1.f;
        // LaunchVelocity is already a world-space ballistic solution. Disable
        // both the implicit local rotation and InitialSpeed magnitude override.
        Arrow->ProjectileMovement->bInitialVelocityInLocalSpace = false;
        Arrow->ProjectileMovement->InitialSpeed = 0.f;
        Arrow->ProjectileMovement->MaxSpeed = 0.f;
        Arrow->ProjectileMovement->Velocity = LaunchVelocity;

        Arrow->FinishSpawning(SpawnTransform);
        if (IsValid(Arrow))
        {
            Arrow->SetupTrail();
        }
    }
}

void ABaseCharacter::ApplyArcherERainDamage(FVector RainCenter)
{
    if (!HasAuthority()) return;

    // Archer E scales from the character's current attack power, including
    // shop/stat-potion bonuses. At skill level 1 EMultiplier is 1.0, so the
    // rain deals exactly one attack-power hit.
    const float Damage = AttackPower * EMultiplier;

    for (TActorIterator<AMonster> It(GetWorld()); It; ++It)
    {
        AMonster* Monster = *It;
        // This is a ground field, so its circular range is measured in XY from
        // the monster/ground point that actually stopped the E arrow.
        if (Monster && FVector::DistSquared2D(Monster->GetActorLocation(), RainCenter) <= FMath::Square(ERadius))
        {
            Monster->TakeMonsterDamage(Damage);
        }
    }

    for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
    {
        ADragonBoss* Dragon = *It;
        if (Dragon && Dragon->IsWithinDamageRadius(RainCenter, ERadius))
        {
            Dragon->TakeBossDamage(Damage);
        }
    }
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
    ForceNetUpdate();

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
    if (!bCanUseQ || !CanUseCombatAction())
    {
        return;
    }

    bCanUseQ = false;

    QRemainingCooldown = QCooldown;

    GetWorldTimerManager().SetTimer(
        QCooldownTimer,
        this,
        &ABaseCharacter::ResetQCooldown,
        QCooldown,
        false
    );

    // 팔라딘 / 워리어 Q는 0.8초 후 데미지 적용
    if (CharacterType == ECharacterType::Paladin ||
        CharacterType == ECharacterType::Warrior)
    {
        GetWorldTimerManager().SetTimer(
            QDamageTimer,
            this,
            &ABaseCharacter::ExecuteQDamage,
            0.8f,
            false
        );
    }
    else
    {
        // 아처는 기존처럼 투사체가 데미지를 처리하므로 여기서 Q 데미지를 적용하지 않음.
        ExecuteQDamage();
    }

    ClientStartSkillCooldown(ESkillType::Q, QCooldown);
    ForceNetUpdate();
    MulticastPlayQ();
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

    // Archer Q is projectile-driven. The old point-blank sphere dealt a
    // second hit before the volley reached the same target.
    if (CharacterType == ECharacterType::Archer)
    {
        return;
    }

    FVector Start = GetActorLocation();

    TArray<FOverlapResult> Overlaps;

    // Melee Q range is measured from the outer edge of this character's
    // capsule, matching the monster attack-range calculation.
    const bool bMeleeCharacter =
        CharacterType == ECharacterType::Warrior || CharacterType == ECharacterType::Paladin;
    const float QCollisionRadius = bMeleeCharacter
        ? QRadius + GetCapsuleComponent()->GetScaledCapsuleRadius()
        : QRadius;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(QCollisionRadius);

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

    if (CharacterType == ECharacterType::Paladin)
    {
        DefenseRate = PaladinWDefenseRate;
        GetWorldTimerManager().ClearTimer(PaladinWBuffHandle);
        GetWorldTimerManager().SetTimer(
            PaladinWBuffHandle,
            this,
            &ABaseCharacter::EndPaladinWBuff,
            WBuffDuration,
            false);
    }
    else if (CharacterType == ECharacterType::Archer)
    {
        AttackSpeed = BuffAttackSpeed;
        GetWorldTimerManager().ClearTimer(AttackSpeedBuffHandle);
        GetWorldTimerManager().SetTimer(
            AttackSpeedBuffHandle,
            this,
            &ABaseCharacter::EndArcherWBuff,
            WBuffDuration,
            false);
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
        // The Niagara system already contains its authored size. Do not apply
        // a world-scale multiplier here: it made Paladin's shield E effect
        // five times larger in packaged builds.
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
        GetWorldTimerManager().SetTimer(
            EDamageTimer,
            this,
            &ABaseCharacter::ExecuteWarriorEDamage,
            0.65f,
            false
        );

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

void ABaseCharacter::ExecuteWarriorEDamage()
{
    if (!HasAuthority() || CharacterType != ECharacterType::Warrior)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    const FCollisionShape DamageSphere =
        FCollisionShape::MakeSphere(WarriorERadius);

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

void ABaseCharacter::MulticastPlayHPBuffEffect_Implementation(
    FVector Location)
{
    if (!HPBuffEffect)
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        HPBuffEffect,
        Location,
        FRotator::ZeroRotator,
        FVector::OneVector,
        true,
        true
    );
}

void ABaseCharacter::MulticastPlayAttackBuffEffect_Implementation(
    FVector Location)
{
    if (!AttackBuffEffect)
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        AttackBuffEffect,
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


    // Paladin R's heal lands when its animation finishes, matching potion
    // timing.  The actual HP change stays on the server.
    if (CharacterType == ECharacterType::Paladin)
    {
        const float HealDelay = RMontage ? RMontage->GetPlayLength() : 0.f;
        GetWorldTimerManager().SetTimer(
            PaladinRHealTimer,
            this,
            &ABaseCharacter::ApplyPaladinRHeal,
            FMath::Max(HealDelay, KINDA_SMALL_NUMBER),
            false);
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

void ABaseCharacter::ApplyPaladinRHeal()
{
    if (!HasAuthority() || CharacterType != ECharacterType::Paladin)
    {
        return;
    }

    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

    for (AActor* Actor : Players)
    {
        ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);
        if (!Player || Player->IsDead() || Player->CurrentHP >= Player->MaxHP)
        {
            continue;
        }

        const float Heal = Player->MaxHP * RHealAmount;
        Player->HealPlayer(Heal);

        // Reuse the same NS_Heal effect spawned when a potion finishes.
        MulticastPlayRHealEffect(Player->GetActorLocation());
    }
}

void ABaseCharacter::EndWarriorRBuff()
{
	AttackPower = BaseAttackPower;
	ForceNetUpdate();
}

void ABaseCharacter::EndPaladinWBuff()
{
    DefenseRate = 0.f;
    ForceNetUpdate();
}

void ABaseCharacter::ServerAttack_Implementation(FRotator TargetRotation) 
{
    SetActorRotation(TargetRotation);
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

    

    // Archer basic attacks are resolved by AArrowProjectile. Keeping the
    // melee overlap here caused a close target to take one instant hit and a
    // second hit from the arrow.
    if (CharacterType == ECharacterType::Archer)
    {
        return;
    }

    FVector Start = GetActorLocation();

    TArray<FOverlapResult> Overlaps;

    // Warrior and Paladin use the same 120-unit capsule-edge range as a
    // monster attack.
    const float AttackRange = 120.f + GetCapsuleComponent()->GetScaledCapsuleRadius();
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRange);

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
    DOREPLIFETIME(ABaseCharacter, AttackSpeed);
    DOREPLIFETIME(ABaseCharacter, bIsDead);
	DOREPLIFETIME(ABaseCharacter, CharacterType);
    DOREPLIFETIME(ABaseCharacter, bHasLantern);
    DOREPLIFETIME(ABaseCharacter, bLanternEquipped);
    DOREPLIFETIME(ABaseCharacter, bLanternPoseActive);
    DOREPLIFETIME(ABaseCharacter, bHasPrism);
    DOREPLIFETIME(ABaseCharacter, bPrismEquipped);
    DOREPLIFETIME(ABaseCharacter, bPrismPoseActive);
    DOREPLIFETIME(ABaseCharacter, ItemAnimationState);
	DOREPLIFETIME(ABaseCharacter, Coin);
	DOREPLIFETIME_CONDITION(ABaseCharacter, PotionCount, COND_OwnerOnly);
	DOREPLIFETIME(ABaseCharacter, bIsUsingPotion);
	DOREPLIFETIME(ABaseCharacter, bIsPlacingLantern);
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
    DOREPLIFETIME(ABaseCharacter, AttackPower);
}

void ABaseCharacter::OnRep_CurrentHP()
{
    UE_LOG(LogTemp, Warning,
        TEXT("Current HP : %f"),
        CurrentHP);

	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(CurrentHP, MaxHP);
	}
}

void ABaseCharacter::OnRep_MaxHP()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(CurrentHP, MaxHP);
	}
}

void ABaseCharacter::OnRep_AttackPower()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateAttackPower(AttackPower);
	}
}

void ABaseCharacter::OnRep_SkillPoints()
{
	if (SkillTreeWidget)
	{
		SkillTreeWidget->UpdateSkillPointText();
	}
}

void ABaseCharacter::OnRep_SkillLevels()
{
	if (SkillTreeWidget)
	{
		SkillTreeWidget->UpdateSkillLevelText();
	}
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
            bIsPlacingLantern = true;

            GetCharacterMovement()->StopMovementImmediately();
            GetCharacterMovement()->DisableMovement();

            ClientShowAltarPlacementProgress(3.f);

            GetWorldTimerManager().SetTimer(
                AltarPlacementTimer,
                this,
                &ABaseCharacter::FinishAltarPlacement,
                3.f,
                false);
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
    bIsPlacingLantern = false;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    ClientHideAltarPlacementProgress();

    if (Altar && !Altar->bLanternPlaced && bHasLantern && bLanternEquipped)
    {
        Altar->PlaceLantern(this);
    }
}

void ABaseCharacter::ClientShowAltarPlacementProgress_Implementation(float Duration)
{
    GetCharacterMovement()->StopMovementImmediately();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

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
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
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

    // The guide effect waits for the replicated idle animation state. Check
    // both values here as their OnRep callbacks can arrive in either order.
    bLanternGuideReady = bLanternEquipped
        && ItemAnimationState == EItemAnimationState::LanternIdle;

    EquippedLanternMesh->SetVisibility(bLanternEquipped);

    LanternLight->SetVisibility(bLanternEquipped, true);

    // The decal is sized from LanternLightSphere in BeginPlay, the same
    // sphere used for the authoritative darkness-protection overlap.  Show
    // it on every map where darkness damage is active so the visible range
    // always matches gameplay, including dungeons.
    const FString CurrentMapName = GetWorld() ? GetWorld()->GetMapName() : FString();
    const bool bUsesDarkness = CurrentMapName.Contains(TEXT("Village"))
        || CurrentMapName.Contains(TEXT("Dungeon"));

    LanternSafeZoneDecal->SetVisibility(false);

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

    // Any character with a right-hand weapon (including Warrior) must put it
    // away while holding the lantern. The combined helper also prevents one
    // held item's OnRep from restoring the weapon while the other is active.
    RefreshRightHandWeaponForHeldItem();

    // bLanternEquipped is the gameplay/mesh state.  Do not activate the held
    // idle layer here: this function is called as soon as replication arrives,
    // which is earlier than the equip montage finishing.
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

    // The server is authoritative for this lock as well: a prism that is
    // unequipping cannot be replaced until its Unequipping -> None transition
    // has completed.
    if (bPrismEquipped || bIsEquippingPrism)
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

    // The dedicated pose state machine now plays the transition sequence
    // itself, so it must be visible from the first equip frame through the
    // final unequip frame. It is disabled only after Unequipping -> None.
    bLanternPoseActive = true;
    SetItemAnimationState(bEquip
        ? EItemAnimationState::LanternEquipping
        : EItemAnimationState::LanternUnequipping);

    OnRep_LanternEquipped();

    ForceNetUpdate();

    StartLanternItemAnimationTimer();

    MulticastPlayLanternMontage(bEquip);
}

void ABaseCharacter::MulticastPlayLanternMontage_Implementation(bool bEquip)
{
    if (bEquip)
    {
        bLanternGuideReady = false;
        // The AnimBP's layered state machine owns this animation. Playing the
        // same montage through DefaultSlot would override locomotion.
    }
    else
    {
        // Completion is driven by the authoritative timer started on server.
    }
}

void ABaseCharacter::ServerUseSlot3_Implementation()
{
    // Symmetric lock for prism input while lantern state is not settled.
    if (bLanternEquipped || bIsEquippingLantern)
        return;

    if (!bHasPrism)
        return;

    if (bIsEquippingPrism)
        return;

    bIsEquippingPrism = true;

    const bool bEquip = !bPrismEquipped;

    bPrismEquipped = bEquip;
    bPrismPoseActive = true;
    SetItemAnimationState(bEquip
        ? EItemAnimationState::PrismEquipping
        : EItemAnimationState::PrismUnequipping);

    RefreshPrismState();

    ForceNetUpdate();

    StartPrismItemAnimationTimer();

    MulticastPlayPrismMontage(bEquip);
}

void ABaseCharacter::ServerRequestGroupPrismCleanse_Implementation()
{
    RequestGroupPrismCleanse();
}

void ABaseCharacter::RequestGroupPrismCleanse()
{
    if (!bDarknessDebuff || !bHasPrism || !bPrismEquipped || IsDead())
    {
        return;
    }

    for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
    {
        It->RegisterPrismCleanseParticipant(this);
        return;
    }
}

void ABaseCharacter::OnRep_PrismEquipped()
{
    EquippedPrismMesh->SetVisibility(bPrismEquipped);

    // The layered state machine is enabled at transition start; its internal
    // ItemAnimationState rule selects Equipping, Idle, or Unequipping.

    if (bPrismEquipped)
    {
        EquippedLanternMesh->SetVisibility(false);
        LanternLight->SetVisibility(false);
    }

    // Apply to every right-hand-weapon character, not only Paladin.
    RefreshRightHandWeaponForHeldItem();
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
        // See MulticastPlayLanternMontage: the layered state machine owns the
        // held-item sequence, preserving the locomotion base pose.
    }
    else
    {
        // Completion is driven by the authoritative timer started on server.
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
        // The client or host can collect the prism; either way every party
        // member must advance to the same return-to-village objective.
        PS->SyncQuestProgressToParty();
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

	NearbyDungeonPortal->ServerEnterDungeon();
}

void ABaseCharacter::ServerInteractQuestGiver_Implementation(AQuestGiver* QuestGiver)
{
    // A client can press the interaction key before the NPC's actor channel is
    // fully established.  In that case Unreal serializes the RPC actor
    // parameter as null.  Resolve the nearby replicated NPC on the server so
    // the interaction remains server-authoritative instead of being dropped.
    if (!QuestGiver || !QuestGiver->CanInteractWith(this))
    {
        QuestGiver = nullptr;
        for (TActorIterator<AQuestGiver> It(GetWorld()); It; ++It)
        {
            if (It->CanInteractWith(this))
            {
                QuestGiver = *It;
                break;
            }
        }
    }

    if (QuestGiver)
    {
        UE_LOG(LogTemp, Log, TEXT("Quest interaction accepted: Player=%s, Giver=%s"),
            *GetNameSafe(this), *GetNameSafe(QuestGiver));
        QuestGiver->Interact(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Quest interaction rejected: no quest giver in range for %s"),
            *GetNameSafe(this));
    }
}

void ABaseCharacter::ServerSubmitQuestDecision_Implementation(AQuestGiver* QuestGiver, bool bAccepted)
{
    if (!QuestGiver || !QuestGiver->CanInteractWith(this))
    {
        QuestGiver = nullptr;
        for (TActorIterator<AQuestGiver> It(GetWorld()); It; ++It)
        {
            if (It->CanInteractWith(this))
            {
                QuestGiver = *It;
                break;
            }
        }
    }

    if (QuestGiver)
    {
        UE_LOG(LogTemp, Log, TEXT("Quest decision received: Player=%s, Accepted=%d"),
            *GetNameSafe(this), bAccepted);
        QuestGiver->ResolveQuestDecision(this, bAccepted);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Quest decision rejected: no quest giver in range for %s"),
            *GetNameSafe(this));
    }
}

void ABaseCharacter::ClientOpenQuestDialogue_Implementation(AQuestGiver* QuestGiver, const TArray<FText>& DialogueLines, const FText& SpeakerName, bool bRequiresQuestDecision)
{
    EnsureQuestDialogueWidget();
    if (!QuestDialogueWidget)
    {
        return;
    }

    QuestDialogueWidget->ConfigureDialogue(QuestGiver, DialogueLines, SpeakerName, bRequiresQuestDecision);
    QuestDialogueWidget->SetVisibility(ESlateVisibility::Visible);
    QuestDialogueWidget->SetKeyboardFocus();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(QuestDialogueWidget->TakeWidget());
        PC->SetInputMode(InputMode);
    }
}

void ABaseCharacter::ClientFinishQuestDialogue_Implementation(bool bAccepted, const FText& ResultMessage)
{
    if (QuestDialogueWidget)
    {
        QuestDialogueWidget->RemoveFromParent();
        QuestDialogueWidget = nullptr;
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(false);
        PC->SetInputMode(InputMode);
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
    }

    ClientShowQuestMessage(ResultMessage.ToString());
}

void ABaseCharacter::CloseQuestDialogue()
{
    if (QuestDialogueWidget)
    {
        QuestDialogueWidget->RemoveFromParent();
        QuestDialogueWidget = nullptr;
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(false);
        PC->SetInputMode(InputMode);
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
    }
}

void ABaseCharacter::GrantQuestLantern()
{
    if (!HasAuthority() || bHasLantern)
    {
        return;
    }

    bHasLantern = true;
    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->bHasLantern = true;
    }
    if (UTheNightfallSiegeInstance* GI = Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bHasLantern = true;
        GI->bWorldLanternDestroyed = true;
    }

    // The quest reward replaces the old world pickup, preventing a second
    // party member from acquiring another slot-1 lantern afterwards.
    TArray<AActor*> WorldLanterns;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALantern::StaticClass(), WorldLanterns);
    for (AActor* WorldLantern : WorldLanterns)
    {
        WorldLantern->Destroy();
    }

    OnRep_HasLantern();
    ForceNetUpdate();
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

void ABaseCharacter::EnsureQuestDialogueWidget()
{
    if (QuestDialogueWidget)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    // WBP_QuestDialogue can be authored later; this native class remains a
    // usable fallback until the asset is created.
    if (!QuestDialogueWidgetClass)
    {
        QuestDialogueWidgetClass = LoadClass<UQuestDialogueWidget>(
            nullptr,
            TEXT("/Game/BP_Character/WBP_QuestDialogue.WBP_QuestDialogue_C"));
    }

    TSubclassOf<UQuestDialogueWidget> ClassToCreate = QuestDialogueWidgetClass;
    if (!ClassToCreate)
    {
        ClassToCreate = UQuestDialogueWidget::StaticClass();
    }
    QuestDialogueWidget = CreateWidget<UQuestDialogueWidget>(PC, ClassToCreate);
    if (QuestDialogueWidget)
    {
        QuestDialogueWidget->AddToViewport(100);
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
	bPrismEquipped = false;
	bPrismPoseActive = false;
	bIsEquippingPrism = false;
	CancelPotionUse();
	SetItemAnimationState(EItemAnimationState::None);
	if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		PS->bLanternEquipped = false;
		PS->bPrismEquipped = false;
		PS->Coin = Coin;
		PS->PotionCount = PotionCount;
		PS->PurchasedItems = PurchasedItems;
		PS->Slot4PurchasedItemIndex = Slot4PurchasedItemIndex;
		PS->ForceNetUpdate();
	}

	OnRep_LanternEquipped();
	OnRep_PrismEquipped();
	ForceNetUpdate();
}

void ABaseCharacter::ServerPickupCoin_Implementation(ACoin* CoinActor)
{
    if (!CoinActor)
    {
        return;
    }

    Coin += 5;

    if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
    {
        PS->Coin = Coin;
        PS->ForceNetUpdate();
    }

    OnRep_Coin();

    CoinActor->Destroy();
}

void ABaseCharacter::OnRep_Coin()
{
    if (HUDWidget)
    {
        HUDWidget->UpdateCoin(Coin);
    }

    UpdateShopGold();
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

	// A remote pawn can receive its PlayerState after BeginPlay.  Create the
	// local gameplay UI here as a second lifecycle-safe entry point so clients
	// never depend on the host's spawn/possession timing.
	const bool bIsLobbyMap = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("Lvl_Lobby"));
	if (!bIsLobbyMap)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController());
			PC && PC->IsLocalController())
		{
			if (!HUDWidget && HUDWidgetClass)
			{
				HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, HUDWidgetClass);
				if (HUDWidget)
				{
					HUDWidget->AddToViewport();
				}
			}
			EnsureQuestWidget();
		}
	}

	if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		RefreshReplicatedStatsFromPlayerState(PS);
		RefreshReplicatedSkillStateFromPlayerState(PS);
		Coin = PS->Coin;
		PotionCount = PS->PotionCount;
		SkillPoints = PS->SkillPoints;
		PurchasedItems = PS->PurchasedItems;
		Slot4PurchasedItemIndex = PurchasedItems.IsValidIndex(PS->Slot4PurchasedItemIndex)
			? PS->Slot4PurchasedItemIndex
			: INDEX_NONE;
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

		OnRep_Coin();
		OnRep_PotionCount();
		OnRep_SkillPoints();
		OnRep_PurchasedItems();
		OnRep_Slot4PurchasedItemIndex();
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
		RestoreAuthoritativeStateFromPlayerState(PS);
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

void ABaseCharacter::ResetSkillUpgradeEffects()
{
	// Rebuilding a pawn after travel must be absolute. In particular, level-4
	// Q subtracts cooldown, so applying it twice without a reset corrupts it.
	QCooldown = 5.f;
	WCooldown = 20.f;
	ECooldown = 10.f;
	RCooldown = 20.f;
	QMultiplier = 1.f;
	WMultiplier = 1.f;
	EMultiplier = 1.2f;
	RMultiplier = 3.f;
	HealAmount = 0.1f;
	RHealAmount = 0.2f;
	PaladinWDefenseRate = 0.f;
	BuffAttackSpeed = 1.5f;
	WarriorWCooldownReduction = 0.f;
	WarriorRDamageBonus = 0.f;
	bRBonusDamage = false;
	ERadius = 700.f;

	switch (CharacterType)
	{
	case ECharacterType::Paladin:
		QMultiplier = 1.f;
		break;
	case ECharacterType::Archer:
		QMultiplier = 1.5f;
		EMultiplier = 1.f;
		break;
	case ECharacterType::Warrior:
		QMultiplier = 1.f;
		EMultiplier = 1.f;
		break;
	default:
		break;
	}
}

void ABaseCharacter::RestoreAuthoritativeStateFromPlayerState(ABasePlayerState* PS)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}

	// BeginPlay can run before GameMode possesses a freshly spawned pawn.  That
	// ordering is common for remote players after seamless dungeon travel, so
	// BeginPlay's one-shot PlayerState restore is not sufficient for clients.
	CharacterType = PS->SelectedCharacter;
	switch (CharacterType)
	{
	case ECharacterType::Paladin:
		MaxHP = 500.f;
		AttackPower = 100.f;
		break;
	case ECharacterType::Archer:
		MaxHP = 300.f;
		AttackPower = 200.f;
		break;
	case ECharacterType::Warrior:
		MaxHP = 400.f;
		AttackPower = 300.f;
		break;
	default:
		break;
	}

	if (PS->bHasShopStatBonuses)
	{
		// Guard old/incomplete saves so one missing value cannot zero a stat.
		if (PS->SavedMaxHP > 0.f)
		{
			MaxHP = PS->SavedMaxHP;
		}
		if (PS->SavedAttackPower > 0.f)
		{
			AttackPower = PS->SavedAttackPower;
		}
	}

	CurrentHP = MaxHP;
	BaseAttackPower = AttackPower;
	Coin = PS->Coin;
	PotionCount = PS->PotionCount;
	SkillPoints = PS->SkillPoints;
	PS->CopySkillLevelsTo(SkillLevels);
	SkillLevels.FindOrAdd(ESkillType::Q) = FMath::Clamp(SkillLevels.FindRef(ESkillType::Q), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::W) = FMath::Clamp(SkillLevels.FindRef(ESkillType::W), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::E) = FMath::Clamp(SkillLevels.FindRef(ESkillType::E), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::R) = FMath::Clamp(SkillLevels.FindRef(ESkillType::R), 1, 4);
	PS->SetSkillLevelsFrom(SkillLevels);
	PS->ForceNetUpdate();
	ResetSkillUpgradeEffects();
	RestoreSkillUpgrades();
	PurchasedItems = PS->PurchasedItems;
	Slot4PurchasedItemIndex = PurchasedItems.IsValidIndex(PS->Slot4PurchasedItemIndex)
		? PS->Slot4PurchasedItemIndex
		: INDEX_NONE;

	OnRep_CurrentHP();
	OnRep_Coin();
	OnRep_PotionCount();
	OnRep_SkillPoints();
	OnRep_SkillLevels();
	OnRep_PurchasedItems();
	OnRep_Slot4PurchasedItemIndex();
	ForceNetUpdate();
}

void ABaseCharacter::RefreshReplicatedStatsFromPlayerState(const ABasePlayerState* PS)
{
	if (HasAuthority() || !PS)
	{
		return;
	}

	// PlayerState properties and the new pawn can arrive in either order on a
	// travelling client.  Rebuild the local replicated cache from the durable
	// PlayerState so HUD Tick never falls back to the new pawn's class defaults.
	CharacterType = PS->SelectedCharacter;
	switch (CharacterType)
	{
	case ECharacterType::Paladin:
		MaxHP = 500.f;
		AttackPower = 100.f;
		break;
	case ECharacterType::Archer:
		MaxHP = 300.f;
		AttackPower = 200.f;
		break;
	case ECharacterType::Warrior:
		MaxHP = 400.f;
		AttackPower = 300.f;
		break;
	default:
		break;
	}

	if (PS->bHasShopStatBonuses)
	{
		if (PS->SavedMaxHP > 0.f)
		{
			MaxHP = PS->SavedMaxHP;
		}
		if (PS->SavedAttackPower > 0.f)
		{
			AttackPower = PS->SavedAttackPower;
		}
	}

	BaseAttackPower = AttackPower;
	OnRep_MaxHP();
	OnRep_AttackPower();
}

void ABaseCharacter::RefreshReplicatedSkillStateFromPlayerState(const ABasePlayerState* PS)
{
	if (HasAuthority() || !PS)
	{
		return;
	}

	SkillPoints = FMath::Max(0, PS->SkillPoints);
	PS->CopySkillLevelsTo(SkillLevels);
	SkillLevels.FindOrAdd(ESkillType::Q) = FMath::Clamp(SkillLevels.FindRef(ESkillType::Q), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::W) = FMath::Clamp(SkillLevels.FindRef(ESkillType::W), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::E) = FMath::Clamp(SkillLevels.FindRef(ESkillType::E), 1, 4);
	SkillLevels.FindOrAdd(ESkillType::R) = FMath::Clamp(SkillLevels.FindRef(ESkillType::R), 1, 4);
	OnRep_SkillPoints();
	OnRep_SkillLevels();
}

void ABaseCharacter::OnRep_DarknessDebuff()
{
    UE_LOG(LogTemp, Warning,
        TEXT("Darkness : %d"),
        bDarknessDebuff);

    if (!bDarknessDebuff && DarknessCleanseEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            DarknessCleanseEffect,
            GetActorLocation()
        );
    }
}

void ABaseCharacter::DebugBossPattern1()
{
    if (IsDeveloperHost()) ServerDebugBossPattern(0);
}

void ABaseCharacter::DebugBossPattern2()
{
    if (IsDeveloperHost()) ServerDebugBossPattern(1);
}

void ABaseCharacter::DebugBossPattern3()
{
    if (IsDeveloperHost()) ServerDebugBossPattern(2);
}

void ABaseCharacter::DebugBossPattern4()
{
    if (IsDeveloperHost()) ServerDebugBossPattern(3);
}

void ABaseCharacter::DebugBossCenterMechanic()
{
    if (IsDeveloperHost()) ServerDebugBossPattern(4);
}

void ABaseCharacter::DebugTeleportToDungeonPortal()
{
    if (IsDeveloperHost()) ServerDebugTeleportToDungeonPortal();
}

void ABaseCharacter::DebugCompleteRaid()
{
    if (IsDeveloperHost()) ServerDebugCompleteRaid();
}

void ABaseCharacter::DebugEnableMoveSpeed()
{
    if (IsDeveloperHost()) ServerSetDebugMoveSpeed(true);
}

void ABaseCharacter::DebugResetMoveSpeed()
{
    if (IsDeveloperHost()) ServerSetDebugMoveSpeed(false);
}

bool ABaseCharacter::IsDeveloperHost() const
{
    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    return HasAuthority() && PlayerController && PlayerController->IsLocalController();
}

void ABaseCharacter::ServerDebugCompleteRaid_Implementation()
{
    if (!IsDeveloperHost())
    {
        return;
    }

    UTheNightfallSiegeInstance* GI = Cast<UTheNightfallSiegeInstance>(GetGameInstance());
    if (!GI)
    {
        return;
    }

    if (GetWorld()->GetMapName().Contains(TEXT("Dungeon")))
    {
        ADungeonManager* DungeonManager = Cast<ADungeonManager>(
            UGameplayStatics::GetActorOfClass(GetWorld(), ADungeonManager::StaticClass()));
        const int32 DefeatedMonsterCount = DungeonManager
            ? DungeonManager->DebugClearDungeon(this)
            : 0;
        if (DefeatedMonsterCount <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Debug dungeon clear failed: no living dungeon monsters found."));
            return;
        }

        // Match the normal dungeon-clear reward, but award it here rather
        // than depending on the final monster callback during a bulk clear.
        TArray<AActor*> Players;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
        for (AActor* Actor : Players)
        {
            if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
            {
                if (ABasePlayerState* PartyPlayerState = Player->GetPlayerState<ABasePlayerState>())
                {
					PartyPlayerState->SkillPoints = FMath::Max(0, PartyPlayerState->SkillPoints) + 2;
					Player->SkillPoints = PartyPlayerState->SkillPoints;
                    PartyPlayerState->ForceNetUpdate();
                }
				Player->OnRep_SkillPoints();
                Player->ForceNetUpdate();
            }
        }

        const int32 GoldReward = DefeatedMonsterCount * 5;
        for (AActor* Actor : Players)
        {
            if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
            {
                Player->Coin += GoldReward;
                if (ABasePlayerState* PartyPlayerState = Player->GetPlayerState<ABasePlayerState>())
                {
                    PartyPlayerState->Coin = Player->Coin;
                    PartyPlayerState->ForceNetUpdate();
                }
                Player->OnRep_Coin();
                Player->ForceNetUpdate();
            }
        }

        if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
        {
            PS->Coin = Coin;
            // Do not rely only on the per-monster callbacks for the debug
            // shortcut.  Mark the objective complete explicitly so its UI
            // and every party member advance to the prism collection step.
            PS->DungeonMonsterTotalCount = DungeonManager->TotalMonsterCount;
            PS->DungeonMonsterKillCount = DungeonManager->TotalMonsterCount;
            PS->NotifyDungeonCleared();
            PS->SyncQuestProgressToParty();
            PS->ForceNetUpdate();
            ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
        }
        UE_LOG(LogTemp, Warning, TEXT("Debug dungeon clear: %d monsters defeated and %d gold granted."),
            DefeatedMonsterCount, GoldReward);
        return;
    }

    GI->RemainingDungeons.Empty();
    GI->ClearedDungeonCount = 3;
    GI->CurrentDungeon = NAME_None;
    GI->bHasPrism = true;
    GI->bPrismEquipped = false;

    // The debug shortcut transitions directly to the boss phase, so ordinary
    // dungeon portals must not remain usable alongside the boss portal.
    for (TActorIterator<ADungeonPortal> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
    for (AActor* Actor : Players)
    {
        ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);
        if (!Player) continue;

        Player->Coin += 50;
        Player->bHasPrism = true;
        Player->bPrismEquipped = false;
        Player->bPrismPoseActive = false;
        Player->OnRep_HasPrism();
        Player->RefreshPrismState();
        Player->ForceNetUpdate();

        if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
        {
			PS->SkillPoints = FMath::Max(0, PS->SkillPoints) + 8;
			Player->SkillPoints = PS->SkillPoints;
            PS->bHasPrism = true;
            PS->bPrismEquipped = false;
            PS->ClearedDungeonCount = 3;
            PS->QuestStage = EQuestStage::FindBossPortal;
            PS->SkillPoints = Player->SkillPoints;
            PS->Coin = Player->Coin;
            PS->ForceNetUpdate();
        }
        Player->OnRep_Coin();
        Player->ForceNetUpdate();
    }

    if (!GetWorld()->GetMapName().Contains(TEXT("Village_Forest")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Debug raid complete set. Return to the village for the boss portal."));
        return;
    }

    if (!GI->bBossPortalSpawned)
    {
        for (TActorIterator<AVillageManager> It(GetWorld()); It; ++It)
        {
            It->SpawnBossPortal();
            break;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Debug raid complete: dungeon portals removed, +8 skill points and +50 gold granted, and the boss portal is ready."));
}

void ABaseCharacter::ServerDebugTeleportToDungeonPortal_Implementation()
{
	if (!IsDeveloperHost())
	{
		return;
	}

	auto TeleportParty = [this](const FVector& Destination, const FRotator& Rotation)
	{
		TArray<AActor*> Players;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

		int32 PlayerIndex = 0;
		for (AActor* Actor : Players)
		{
			ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);
			if (!Player)
			{
				continue;
			}

			const int32 Row = PlayerIndex / 3;
			const int32 Column = PlayerIndex % 3;
			const FVector FormationOffset(Row * 180.f, (Column - 1) * 180.f, 0.f);
			Player->TeleportTo(Destination + FormationOffset, Rotation, false, true);
			Player->ForceNetUpdate();
			++PlayerIndex;
		}
	};

	for (TActorIterator<ADragonBoss> It(GetWorld()); It; ++It)
	{
		ADragonBoss* Dragon = *It;
		if (!IsValid(Dragon))
		{
			continue;
		}

		constexpr float DragonFrontTeleportDistance = 1200.f;
		FVector Destination = Dragon->GetActorLocation()
			+ Dragon->GetActorForwardVector() * DragonFrontTeleportDistance;
		Destination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		TeleportParty(Destination, Dragon->GetActorForwardVector().Rotation());
		UE_LOG(LogTemp, Warning, TEXT("Debug teleported party in front of dragon: %s"), *Destination.ToString());
		return;
	}

	AActor* ClosestPortal = nullptr;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();

    if (GetWorld()->GetMapName().Contains(TEXT("Dungeon")))
    {
        for (TActorIterator<AAltar> It(GetWorld()); It; ++It)
        {
            if (It->bCleared)
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                ClosestPortal = *It;
            }
        }
    }

    // Once the boss portal exists, the shortcut should lead to it instead of
    // an ordinary dungeon portal.
    if (!ClosestPortal)
    {
        for (TActorIterator<APortal> It(GetWorld()); It; ++It)
        {
            if (It->PortalType != EPortalType::Boss)
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                ClosestPortal = *It;
            }
        }
    }

    // Before the boss phase, preserve the existing dungeon-portal behavior.
    if (!ClosestPortal)
    {
        for (TActorIterator<ADungeonPortal> It(GetWorld()); It; ++It)
        {
            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                ClosestPortal = *It;
            }
        }
    }

    if (!ClosestPortal)
    {
        UE_LOG(LogTemp, Warning, TEXT("Debug portal teleport failed: no boss or dungeon portal exists."));
        return;
    }

    const FVector Destination = ClosestPortal->GetActorLocation()
        + FVector(350.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    TeleportParty(Destination, GetActorRotation());
    UE_LOG(LogTemp, Warning, TEXT("Debug teleported party beside dungeon portal: %s"), *Destination.ToString());
}

void ABaseCharacter::ServerSetDebugMoveSpeed_Implementation(bool bEnable)
{
    if (!IsDeveloperHost())
    {
        return;
    }

    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
    for (AActor* Actor : Players)
    {
        ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);
        UCharacterMovementComponent* Movement = Player ? Player->GetCharacterMovement() : nullptr;
        if (!Movement)
        {
            continue;
        }

        if (Player->DebugDefaultWalkSpeed <= 0.f)
        {
            Player->DebugDefaultWalkSpeed = Movement->MaxWalkSpeed;
        }

        const float NewMaxWalkSpeed = bEnable
            ? Player->DebugDefaultWalkSpeed * 2.f
            : Player->DebugDefaultWalkSpeed;
        Movement->MaxWalkSpeed = NewMaxWalkSpeed;
        Player->ClientSetDebugMoveSpeed(NewMaxWalkSpeed);
        Player->ForceNetUpdate();
    }
}

void ABaseCharacter::ClientSetDebugMoveSpeed_Implementation(float NewMaxWalkSpeed)
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = NewMaxWalkSpeed;
    }
}

void ABaseCharacter::ServerDebugBossPattern_Implementation(uint8 PatternIndex)
{
    if (!IsDeveloperHost())
    {
        return;
    }

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

    case 4:
        Boss->DebugCenterMechanic();
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
        if (Player->IsDead() || Player->CurrentHP >= Player->MaxHP)
        {
            continue;
        }

        const float Heal = Player->MaxHP * HealAmount / TotalHealTicks;
        Player->HealPlayer(Heal);

        // Use the potion's heal visual at the exact time this tick restores HP.
        MulticastPlayRHealEffect(Player->GetActorLocation());
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

    // Village safe zone: no darkness damage inside the settlement bounds.
    // X: 12530 ~ 19800, Y: 9600 ~ 16400
    const FVector Location = GetActorLocation();
    const bool bInsideVillageSafeZone =
        Location.X >= 12530.f && Location.X <= 19800.f &&
        Location.Y >= 9600.f && Location.Y <= 16400.f;
    if (bInsideVillageSafeZone)
    {
        return;
    }

    // A lantern placed on an altar creates the same safe circle in which its
    // assigned monsters become vulnerable. Use the altar's authoritative
    // radius here as well, rather than a separate overlap volume, so players
    // receive no darkness tick damage anywhere inside that visible zone.
    for (TActorIterator<AAltar> AltarIt(GetWorld()); AltarIt; ++AltarIt)
    {
        if (AltarIt->IsInsideActiveLightZone(Location))
        {
            return;
        }
    }

    // Held lanterns use the same ground-plane circle as their visible decals.
    // This avoids the old 3D sphere result becoming smaller at different
    // heights or being offset toward the hand socket. It also recomputes the
    // answer every damage tick, so stale begin/end overlap state cannot hurt a
    // player who is visibly standing inside any party member's lantern zone.
    for (TActorIterator<ABaseCharacter> PlayerIt(GetWorld()); PlayerIt; ++PlayerIt)
    {
        if (PlayerIt->IsInsideActiveLanternSafeZone(Location))
        {
            return;
        }
    }

    const float Damage = MaxHP * 0.02f;

    TakePlayerDamage(Damage);
}

void ABaseCharacter::DebugSetGold1000()
{
    if (!IsDeveloperHost())
    {
        return;
    }

    AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
    if (!GameState)
    {
        return;
    }

    for (APlayerState* TargetPlayerState : GameState->PlayerArray)
    {
        ABasePlayerState* PS = Cast<ABasePlayerState>(TargetPlayerState);
        if (!PS)
        {
            continue;
        }

        // PlayerState의 골드를 먼저 증가
        PS->Coin += 1000;
        PS->ForceNetUpdate();

        // 해당 PlayerState의 현재 Pawn(Character)도 같이 갱신
        if (ABaseCharacter* Player = Cast<ABaseCharacter>(PS->GetPawn()))
        {
            Player->Coin = PS->Coin;
            Player->OnRep_Coin();
            Player->ForceNetUpdate();
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("DEBUG GOLD: %s -> %d"),
            *PS->GetPlayerName(),
            PS->Coin
        );
    }
}

bool ABaseCharacter::IsInsideActiveLanternSafeZone(const FVector& WorldLocation) const
{
    if (!bLanternEquipped || !LanternLightSphere || !LanternSafeZoneDecal)
    {
        return false;
    }

    FVector ToLocation = WorldLocation - LanternSafeZoneDecal->GetComponentLocation();
    ToLocation.Z = 0.f;
    const float SafeRadius = LanternLightSphere->GetScaledSphereRadius();
    return ToLocation.SizeSquared() <= FMath::Square(SafeRadius);
}

void ABaseCharacter::ResetInteractionLock()
{
    bInteractionLocked = false;
}
