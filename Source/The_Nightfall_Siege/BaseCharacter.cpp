// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
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


// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    SpringArm->TargetArmLength = 800.f;
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

    LanternLight->SetVisibility(false);

    LanternLight->SetIntensity(5000.f);

    LanternLight->SetAttenuationRadius(1200.f);
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

    switch (CharacterType)
    {
    case ECharacterType::Paladin:

        MaxHP = 1000000.f;
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

        MaxHP = 1000000.f;
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

        QMultiplier = 1.2f;
        EMultiplier = 1.2f;

        break;
    }

    CurrentHP = MaxHP;
	
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

    if (HUDWidgetClass)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());

        if (PC)
        {
            HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, HUDWidgetClass);

            if (HUDWidget)
            {
                HUDWidget->AddToViewport();
            }
        }
    }

    Slot1Icon = EmptySlotIcon;
    Slot2Icon = EmptySlotIcon;
    Slot3Icon = EmptySlotIcon;
    Slot4Icon = EmptySlotIcon;

    PotionCount = 5;
    Slot2Icon = PotionIcon;

    UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance());

    if (GI)
    {
        SkillPoints = GI->SkillPoints;
        SkillLevels = GI->SkillLevels;

        RestoreSkillUpgrades();

        bHasLantern = GI->bHasLantern;
        bLanternEquipped = GI->bLanternEquipped;

        bHasPrism = GI->bHasPrism;
        bPrismEquipped = GI->bPrismEquipped;

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
            OnLanternEquipped();
        }

        if (bPrismEquipped)
        {
            OnPrismEquipped();
        }
    }
}

void ABaseCharacter::Die()
{
    bIsDead = true;

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

    QRemainingCooldown = FMath::Max(0.f, QRemainingCooldown - DeltaTime);

    WRemainingCooldown = FMath::Max(0.f, WRemainingCooldown - DeltaTime);

    ERemainingCooldown = FMath::Max(0.f, ERemainingCooldown - DeltaTime);

    RRemainingCooldown = FMath::Max(0.f, RRemainingCooldown - DeltaTime);

    RotateToMouseCursor();
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
        //EnhancedInput->BindAction(IA_Slot4, ETriggerEvent::Started, this, &ABaseCharacter::UseSlot4);
    }

    if (bIsDead) return; // 죽으면 입력 등록 안함
}

void ABaseCharacter::Attack(
    const FInputActionValue& Value)
{
    if (!CanUseCombatAction())
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

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    RotateToMouseCursor();

    bIsAttacking = true;

    if (AttackMontage)
    {
        PlayAnimMontage(
            AttackMontage,
            AttackSpeed);
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

void ABaseCharacter::Q(const FInputActionValue& Value)
{
    if (!bCanUseQ)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    if (CharacterType == ECharacterType::Archer)
    {
        GetCharacterMovement()->StopMovementImmediately();

        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            AICon->StopMovement();
        }

        RotateToMouseCursor();

        bIsUsingSkill = true;

        if (QMontage)
        {
            PlayAnimMontage(
                QMontage,
                AttackSpeed
            );
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

        return;
    }

    RotateToMouseCursor();

    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    UE_LOG(LogTemp, Warning, TEXT("Q"));
    UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), AttackPower * QMultiplier);

    if (QMontage)
    {
        PlayAnimMontage(QMontage);
    }
    else
    {
        bIsUsingSkill = false;
    }

    if (QSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            QSkillEffect,
            GetMesh(),
            TEXT("SwordSocket"), // 방패 쪽
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    } // VFX    

    bCanUseQ = false;

    QRemainingCooldown = QCooldown;

    GetWorldTimerManager().SetTimer(
        QCooldownTimer,
        this,
        &ABaseCharacter::ResetQCooldown,
        QCooldown,
        false
    );

    FVector Start = GetActorLocation();
    float Radius = 150.f;

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(QRadius);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        Start,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            AMonster* Monster =
                Cast<AMonster>(Result.GetActor());

            if (Monster)
            {
                Monster->TakeMonsterDamage(
                    AttackPower * QMultiplier);
                if (QImpactEffect)
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                        GetWorld(),
                        QImpactEffect,
                        Monster->GetActorLocation() + FVector(0, 0, 80));
                }
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

                UE_LOG(LogTemp, Warning,
                    TEXT("Dragon Hit By Q"));
            }
        }
    }
}

void ABaseCharacter::W(const FInputActionValue& Value)
{
    if (!bCanUseW)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    if (CharacterType == ECharacterType::Archer)
    {
        AttackSpeed = BuffAttackSpeed;

        if (WSkillEffect)
        {
            WAreaComponent =
                UNiagaraFunctionLibrary::SpawnSystemAttached(
                    WSkillEffect,
                    GetRootComponent(),
                    NAME_None,
                    FVector::ZeroVector,
                    FRotator::ZeroRotator,
                    EAttachLocation::KeepRelativeOffset,
                    true
                );
        }

        UE_LOG(LogTemp, Warning,
            TEXT("Attack Speed Buff Start : %f"),
            AttackSpeed);

        GetWorldTimerManager().ClearTimer(
            AttackSpeedBuffHandle);

        GetWorldTimerManager().SetTimer(
            AttackSpeedBuffHandle,
            this,
            &ABaseCharacter::EndArcherWBuff,
            5.f,
            false);

        bCanUseW = false;

        WRemainingCooldown = WCooldown;

        GetWorldTimerManager().SetTimer(
            WCooldownTimer,
            this,
            &ABaseCharacter::ResetWCooldown,
            WCooldown,
            false);

        return;
    }

    if (bLanternEquipped) return;

    if (bIsDead) return;

    if (!bCanUseW || bIsUsingSkill)
        return;

    RotateToMouseCursor();

    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    UE_LOG(LogTemp, Warning, TEXT("W"));
    UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), AttackPower * WMultiplier);

    if (WMontage)
    {
        PlayAnimMontage(WMontage);
    }
    else
    {
        bIsUsingSkill = false;
    }

    if (WSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            WSkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"), //검 쪽
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    } // VFX

    bCanUseW = false;

    WRemainingCooldown = WCooldown;

    GetWorldTimerManager().SetTimer(
        WCooldownTimer,
        this,
        &ABaseCharacter::ResetWCooldown,
        WCooldown,
        false
    );
}

void ABaseCharacter::E(const FInputActionValue& Value)
{
    if (!bCanUseE)
    {
        return;
    }

    if (!CanUseCombatAction())
    {
        return;
    }

    if (CharacterType == ECharacterType::Archer)
    {
        GetCharacterMovement()->StopMovementImmediately();

        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            AICon->StopMovement();
        }

        RotateToMouseCursor();

        bIsUsingSkill = true;

        if (EMontage)
        {
            PlayAnimMontage(
                EMontage,
                AttackSpeed
            );
        }

        bCanUseE = false;

        ERemainingCooldown = ECooldown;

        GetWorldTimerManager().SetTimer(ECooldownTimer, this, &ABaseCharacter::ResetECooldown, ECooldown, false);

        return;
    }

    RotateToMouseCursor();

    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    UE_LOG(LogTemp, Warning, TEXT("E"));
    UE_LOG(LogTemp, Warning, TEXT("Damage: %f"), AttackPower * EMultiplier);

    if (EMontage)
    {
        PlayAnimMontage(EMontage);
    }
    else
    {
        bIsUsingSkill = false;
    }

    if (ESkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            ESkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"), //검 쪽
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    } // VFX

    bCanUseE = false;

    ERemainingCooldown = ECooldown;

    GetWorldTimerManager().SetTimer(
        ECooldownTimer,
        this,
        &ABaseCharacter::ResetECooldown,
        ECooldown,
        false
    );

    if (CharacterType == ECharacterType::Paladin)
    {
        ShieldHP = MaxHP * 0.1f;

        UE_LOG(LogTemp, Warning,
            TEXT("Shield : %f"),
            ShieldHP);
    }

    FVector Start = GetActorLocation();

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere =
        FCollisionShape::MakeSphere(ERadius);

    bool bHit =
        GetWorld()->OverlapMultiByChannel(
            Overlaps,
            Start,
            FQuat::Identity,
            ECC_Pawn,
            Sphere);

    if (bHit)
    {
        TSet<ABaseCharacter*> HealedPlayers;

        for (auto& Result : Overlaps)
        {
            ABaseCharacter* Player =
                Cast<ABaseCharacter>(Result.GetActor());

            if (!Player)
            {
                continue;
            }

            // 이미 처리한 플레이어면 건너뜀
            if (HealedPlayers.Contains(Player))
            {
                continue;
            }

            HealedPlayers.Add(Player);

            float Heal = Player->MaxHP * HealAmount;

            UE_LOG(LogTemp, Warning,
                TEXT("%s HP : %.0f -> %.0f"),
                *Player->GetName(),
                Player->CurrentHP,
                FMath::Min(Player->CurrentHP + Heal, Player->MaxHP));

            Player->CurrentHP =
                FMath::Min(
                    Player->CurrentHP + Heal,
                    Player->MaxHP);
        }
    }
}

void ABaseCharacter::R(const FInputActionValue& Value)
{
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
        GetCharacterMovement()->StopMovementImmediately();

        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            AICon->StopMovement();
        }

        RotateToMouseCursor();

        bIsUsingSkill = true;

        if (RMontage)
        {
            PlayAnimMontage(
                RMontage,
                AttackSpeed
            );
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

        return;
    }

    RotateToMouseCursor();

    bIsUsingSkill = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
    }

    UE_LOG(LogTemp, Warning, TEXT("R"));

    if (RMontage)
    {
        PlayAnimMontage(RMontage);
    }
    else
    {
        bIsUsingSkill = false;
    }

    if (RSkillEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            RSkillEffect,
            GetMesh(),
            TEXT("RightHandSocket"), //검 쪽
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    } // VFX

    bCanUseR = false;

    RRemainingCooldown = RCooldown;

    GetWorldTimerManager().SetTimer(
        RCooldownTimer,
        this,
        &ABaseCharacter::ResetRCooldown,
        RCooldown,
        false
    );

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

        Player->CurrentHP = FMath::Min(
            Player->CurrentHP + Heal,
            Player->MaxHP);

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

void ABaseCharacter::ResetQCooldown()
{
    bCanUseQ = true;
}

void ABaseCharacter::ResetWCooldown()
{
    bCanUseW = true;
}

void ABaseCharacter::ResetECooldown()
{
    bCanUseE = true;
}

void ABaseCharacter::ResetRCooldown()
{
    bCanUseR = true;
}

void ABaseCharacter::ToggleInventory()
{
    UE_LOG(LogTemp, Warning, TEXT("Inventory Toggle"));

    if (!InventoryWidgetClass) return;

    if (!bInventoryOpen)
    {
        InventoryWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);

        if (InventoryWidget)
        {
            InventoryWidget->AddToViewport();
        }

        bInventoryOpen = true;
    }
    else
    {
        if (InventoryWidget)
        {
            InventoryWidget->RemoveFromParent();
        }

        bInventoryOpen = false;
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

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->SkillPoints = SkillPoints;
        GI->SkillLevels = SkillLevels;
    }

    GEngine->AddOnScreenDebugMessage(
        -1,
        2.f,
        FColor::Green,
        TEXT("Skill Upgraded")
    );

    return true;
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
            }

            break;

        case ESkillType::W:

            if (SkillLevel == 2)
            {
                QCooldown -= 2.f;
                ECooldown -= 2.f;
                RCooldown -= 2.f;
            }
            else if (SkillLevel == 3)
            {
                QCooldown -= 3.f;
                ECooldown -= 3.f;
                RCooldown -= 3.f;
            }
            else if (SkillLevel == 4)
            {
                QCooldown -= 4.f;
                ECooldown -= 4.f;
                RCooldown -= 4.f;
            }

            break;

        case ESkillType::E:

            if (SkillLevel == 2)
            {
                EMultiplier = 1.2f;
            }
            else if (SkillLevel == 3)
            {
                EMultiplier = 1.5f;
            }
            else if (SkillLevel == 4)
            {
                EMultiplier = 1.8f;
            }

            break;

        case ESkillType::R:

            if (SkillLevel == 2)
            {
                AttackPower *= 1.5f;
            }
            else if (SkillLevel == 3)
            {
                AttackPower *= 1.8f;
            }
            else if (SkillLevel == 4)
            {
                AttackPower *= 2.0f;
            }

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
    if (NearbyPortal)
    {
        NearbyPortal->Interact(this);

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
        bHasLantern = true;

        Slot1Icon = LanternIcon;

        if (UTheNightfallSiegeInstance* GI =
            Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
        {
            GI->bHasLantern = true;
        }

        NearbyLantern->Destroy();

        NearbyLantern = nullptr;
    }

    if (NearbyPrism)
    {
        bHasPrism = true;

        Slot3Icon = PrismIcon;

        if (UTheNightfallSiegeInstance* GI =
            Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
        {
            GI->bHasPrism = true;
        }

        NearbyPrism->SpawnReturnPortal();

        NearbyPrism->Destroy();

        NearbyPrism = nullptr;
    }
}

void ABaseCharacter::UseSlot1(const FInputActionValue& Value)
{
    if (bPrismEquipped)
    {
        return;
    }

    if (!bHasLantern)
        return;

    if (bIsEquippingLantern)
        return;

    bIsEquippingLantern = true;

    if (!bLanternEquipped)
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
            bLanternPoseActive = false;

            PlayAnimMontage(LanternUnequipMontage);
        }
    }
}

void ABaseCharacter::UseSlot2(const FInputActionValue& Value)
{
    if (PotionCount <= 0)
    {
        return;
    }

    if (CurrentHP >= MaxHP)
    {
        return;
    }

    PotionCount--;

    CurrentHP = FMath::Min(
        CurrentHP + MaxHP * 0.3f,
        MaxHP);

    UE_LOG(LogTemp, Warning,
        TEXT("Potion Used"));

    UE_LOG(LogTemp, Warning,
        TEXT("Potion Left : %d"),
        PotionCount);

    if (PotionCount <= 0)
    {
        Slot2Icon = EmptySlotIcon;
    }
}

void ABaseCharacter::UseSlot3(
    const FInputActionValue& Value)
{
    if (bLanternEquipped)
    {
        return;
    }

    if (!bHasPrism)
        return;

    if (bIsEquippingPrism)
        return;

    bIsEquippingPrism = true;

    if (!bPrismEquipped)
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
            bPrismPoseActive = false;

            PlayAnimMontage(PrismUnequipMontage);
        }
    }
}

void ABaseCharacter::OnLanternEquipped()
{
    EquippedPrismMesh->SetVisibility(false);

    bPrismEquipped = false;

    bLanternEquipped = true;

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bLanternEquipped = true;
    }

    bPrismPoseActive = false;

    bIsEquippingLantern = false;

    bLanternPoseActive = true;

    EquippedLanternMesh->SetVisibility(true);

    LanternLight->SetVisibility(true);

    if (CharacterType == ECharacterType::Paladin)
    {
        if (RightHandWeapon)
        {
            RightHandWeapon->SetActorHiddenInGame(true);
            RightHandWeapon->SetActorEnableCollision(false);
        }
    }

    bIsEquippingLantern = false;

    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ABaseCharacter::OnLanternUnequipped()
{
    UE_LOG(LogTemp, Warning, TEXT("UNEQUIP"));

    bLanternEquipped = false;

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bLanternEquipped = false;
    }

    bLanternPoseActive = false;

    bIsEquippingLantern = false;

    EquippedLanternMesh->SetVisibility(false);

    LanternLight->SetVisibility(false);

    if (CharacterType == ECharacterType::Paladin)
    {
        if (RightHandWeapon)
        {
            RightHandWeapon->SetActorHiddenInGame(false);
            RightHandWeapon->SetActorEnableCollision(true);
        }
    }
}

void ABaseCharacter::OnLanternUnequipFinished()
{
    UE_LOG(LogTemp, Warning, TEXT("UNEQUIP FINISHED"));

    bLanternEquipped = false;
    bIsEquippingLantern = false;

    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
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
        SetActorRotation(
            Direction.Rotation());
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
    EquippedLanternMesh->SetVisibility(false);
    LanternLight->SetVisibility(false);

    bLanternEquipped = false;
    bLanternPoseActive = false;

    bPrismEquipped = true;

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bPrismEquipped = true;
    }

    bPrismPoseActive = true;

    bLanternPoseActive = false;

    EquippedPrismMesh->SetVisibility(true);

    bIsEquippingPrism = false;

    GetCharacterMovement()->SetMovementMode(
        MOVE_Walking);
}

void ABaseCharacter::OnPrismUnequipped()
{
    EquippedPrismMesh->SetVisibility(false);
}

void ABaseCharacter::OnPrismUnequipFinished()
{
    bPrismEquipped = false;

    if (UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        GI->bPrismEquipped = false;
    }

    bIsEquippingPrism = false;

    bPrismPoseActive = false;

    GetCharacterMovement()->SetMovementMode(
        MOVE_Walking);
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
        ESkillType SkillType = Pair.Key;
        int32 Level = Pair.Value;

        if (Level <= 1)
        {
            continue;
        }

        for (int32 i = 2; i <= Level; i++)
        {
            FSkillUpgradeData Data;
            Data.SkillType = SkillType;

            SkillLevels[SkillType] = i;

            ApplySkillUpgrade(Data);
        }
    }
}

void ABaseCharacter::SetNearbyPortal(APortal* Portal)
{
    NearbyPortal = Portal;
}

