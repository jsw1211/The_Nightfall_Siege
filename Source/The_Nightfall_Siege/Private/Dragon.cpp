// Fill out your copyright notice in the Description page of Project Settings.


#include "Dragon.h"

// Sets default values
ADragon::ADragon()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ADragon::BeginPlay()
{
	Super::BeginPlay();

	ScheduleNextAttack();
}

// Called every frame
void ADragon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADragon::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ADragon::ScheduleNextAttack()
{
    GetWorldTimerManager().SetTimer(
        AttackTimerHandle,
        this,
        &ADragon::StartAttack,
        AttackDelay,
        false
    );
}

void ADragon::StartAttack()
{
    if (bIsAttacking)
        return;

    bIsAttacking = true;

    UAnimMontage* SelectedMontage = nullptr;

    EDragonAttackType AttackType = SelectNextAttack();

    switch (AttackType)
    {
    case EDragonAttackType::Bite:
        SelectedMontage = BiteMontage;
        break;

    case EDragonAttackType::Breath:
        SelectedMontage = BreathMontage;
        break;

    case EDragonAttackType::CloseBreath:
        SelectedMontage = CloseBreathMontage;
        break;
    }

    if (SelectedMontage)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        if (AnimInstance)
        {
            AnimInstance->Montage_Play(SelectedMontage);

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &ADragon::OnAttackEnded);

            AnimInstance->Montage_SetEndDelegate(
                EndDelegate,
                SelectedMontage
            );
        }
    }
}

void ADragon::OnAttackEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsAttacking = false;

    ScheduleNextAttack();
}

EDragonAttackType ADragon::SelectNextAttack()
{
    EDragonAttackType Result;

    switch (CurrentAttackIndex)
    {
    case 0:
        Result = EDragonAttackType::Bite;
        break;

    case 1:
        Result = EDragonAttackType::Breath;
        break;

    default:
        Result = EDragonAttackType::CloseBreath;
        break;
    }

    CurrentAttackIndex++;

    if (CurrentAttackIndex >= 3)
    {
        CurrentAttackIndex = 0;
    }

    return Result;
}

