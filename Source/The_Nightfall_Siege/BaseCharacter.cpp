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

// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
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

}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

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
}

void ABaseCharacter::Die()
{
    bIsDead = true;

    // 이동 멈춤
    GetCharacterMovement()->DisableMovement();

    // 입력 완전 차단
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        DisableInput(PC);
    }

    // 충돌 비활성화
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(IA_LeftPunch, ETriggerEvent::Started, this, &ABaseCharacter::LeftPunch);
        EnhancedInput->BindAction(IA_RightPunch, ETriggerEvent::Started, this, &ABaseCharacter::RightPunch);
        EnhancedInput->BindAction(IA_Inventory, ETriggerEvent::Started,this, &ABaseCharacter::ToggleInventory
        );
    }

    if (bIsDead) return; // 죽으면 입력 등록 안함
}

void ABaseCharacter::LeftPunch(const FInputActionValue& Value)
{
    if (!bCanUseLeftPunch)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Left Punch"));

    if (LeftPunchMontage)
    {
        PlayAnimMontage(LeftPunchMontage);
    }

    bCanUseLeftPunch = false;

    GetWorldTimerManager().SetTimer(
        LeftPunchCooldownTimer,
        this,
        &ABaseCharacter::ResetLeftPunchCooldown,
        LeftPunchCooldown,
        false
    );

    // 주변 몬스터 찾기
    FVector Start = GetActorLocation();
    float Radius = 150.f;

    TArray<FOverlapResult> Overlaps;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(100.f),
        Params
    );

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            AMonster* Monster = Cast<AMonster>(Result.GetActor());

            if (Monster)
            {
                Monster->TakeMonsterDamage(10.f);
            }
        }
    }
}

void ABaseCharacter::RightPunch(const FInputActionValue& Value)
{
    if (!bCanUseRightPunch)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Right Punch"));

    if (RightPunchMontage)
    {
        PlayAnimMontage(RightPunchMontage);
    }

    bCanUseRightPunch = false;

    GetWorldTimerManager().SetTimer(
        RightPunchCooldownTimer,
        this,
        &ABaseCharacter::ResetRightPunchCooldown,
        RightPunchCooldown,
        false
    );

    FVector Start = GetActorLocation();
    float Radius = 150.f;

    TArray<FOverlapResult> Overlaps;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

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
            AMonster* Monster = Cast<AMonster>(Result.GetActor());

            if (Monster)
            {
                Monster->TakeMonsterDamage(20.f); // 오른손 데미지
            }
        }
    }
}

void ABaseCharacter::ResetLeftPunchCooldown()
{
    bCanUseLeftPunch = true;
}

void ABaseCharacter::ResetRightPunchCooldown()
{
    bCanUseRightPunch = true;
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

void ABaseCharacter::TakePlayerDamage(float Damage)
{
    CurrentHP -= Damage;

    if (CurrentHP <= 0)
    {
        PlayAnimMontage(DeathMontage);
    }
    else
    {
        PlayAnimMontage(HitMontage);
    }
}