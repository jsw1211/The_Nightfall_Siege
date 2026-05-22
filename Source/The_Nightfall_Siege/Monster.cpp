// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCharacter.h"
#include "DungeonManager.h"
#include "Altar.h"

// Sets default values
AMonster::AMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    MaxHP = 100.f;
    CurrentHP = 100.f;

    bIsAttacking = false;
    bCanAttack = true;
    AttackCooldown = 2.0f;

    bIsDead = false;
}

// Called when the game starts or when spawned
void AMonster::BeginPlay()
{
	Super::BeginPlay();
	
    DungeonManager =
        Cast<ADungeonManager>(
            UGameplayStatics::GetActorOfClass(
                GetWorld(),
                ADungeonManager::StaticClass()));
}

// Called every frame
void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (!Player) return;

    float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

    if (Distance <= 150.f)
    {
        GetCharacterMovement()->StopMovementImmediately();

        if (bCanAttack)
        {
            bIsAttacking = true;
            bCanAttack = false;

            ABaseCharacter* PlayerChar = Cast<ABaseCharacter>(Player);
            if (PlayerChar)
            {
                PlayerChar->TakePlayerDamage(10.f); // 몬스터 공격력
            }

            GetWorldTimerManager().SetTimer(
                AttackTimerHandle,
                this,
                &AMonster::ResetAttack,
                AttackCooldown,
                false
            );
        }
    }
    else
    {
        UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), Player);

        bIsAttacking = false;
    }
}

// Called to bind functionality to input
void AMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMonster::ResetAttack()
{
    bIsAttacking = false;
    bCanAttack = true;
}

void AMonster::TakeMonsterDamage(float Damage)
{
    if (!OwnerAltar)
    {
        return;
    }

    // 제단 비활성 상태면 무적
    if (!OwnerAltar->bActivated)
    {
        UE_LOG(LogTemp, Warning, TEXT("Monster Invincible"));

        return;
    }

    // 이미 죽었으면 무시
    if (bIsDead)
    {
        return;
    }

    CurrentHP -= Damage;

    UE_LOG(LogTemp, Warning, TEXT("Monster HP: %f"), CurrentHP);

    if (CurrentHP <= 0)
    {
        bIsDead = true;

        UE_LOG(LogTemp, Warning, TEXT("Monster Dead"));

        // DungeonManager에 몬스터 사망 알림
        if (DungeonManager)
        {
            DungeonManager->OnMonsterDead();
        }

        Destroy();
    }
}