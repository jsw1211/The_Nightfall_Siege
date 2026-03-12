// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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
    }

}

void ABaseCharacter::LeftPunch(const FInputActionValue& Value)
{
    if (!bCanUseLeftPunch)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Left Punch Input"));

    if (LeftPunchMontage)
    {
        PlayAnimMontage(LeftPunchMontage);

        // 쿨타임 시작
        bCanUseLeftPunch = false;

        GetWorldTimerManager().SetTimer(
            LeftPunchCooldownTimer,
            this,
            &ABaseCharacter::ResetLeftPunchCooldown,
            LeftPunchCooldown,
            false
        );
    }
}

void ABaseCharacter::RightPunch(const FInputActionValue& Value)
{
    if (!bCanUseRightPunch)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Right Punch Input"));

    if (RightPunchMontage)
    {
        PlayAnimMontage(RightPunchMontage);

        // 쿨타임 시작
        bCanUseRightPunch = false;

        GetWorldTimerManager().SetTimer(
            RightPunchCooldownTimer,
            this,
            &ABaseCharacter::ResetRightPunchCooldown,
            RightPunchCooldown,
            false
        );
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

