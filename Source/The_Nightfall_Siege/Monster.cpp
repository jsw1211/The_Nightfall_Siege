// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCharacter.h"
#include "DungeonManager.h"
#include "Altar.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "Coin.h"
#include "DungeonPrism.h"
#include "TheNightfallSiegeInstance.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AMonster::AMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

    MaxHP = 1000.f;
    CurrentHP = 1000.f;

    HPWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidget"));

    HPWidget->SetupAttachment(GetRootComponent());

    HPWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

    HPWidget->SetWidgetSpace(EWidgetSpace::Screen);

    HPWidget->SetDrawSize(FVector2D(150.f, 20.f));

    bIsAttacking = false;
    bCanAttack = true;
    AttackCooldown = 2.0f;

    bIsDead = false;
}

// Called when the game starts or when spawned
void AMonster::BeginPlay()
{
	Super::BeginPlay();
	
    DungeonManager = Cast<ADungeonManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ADungeonManager::StaticClass()));
    
    if (UUserWidget* Widget =
        HPWidget->GetUserWidgetObject())
    {
    }
}

// Called every frame
void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (bIsDead)
    {
        return;
    }

    ABaseCharacter* Player = nullptr;

    if (bIsTaunted && TauntTarget)
    {
        Player = TauntTarget;
    }
    else
    {
        TArray<AActor*> Players;

        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ABaseCharacter::StaticClass(),
            Players);

        float ClosestDistance = FLT_MAX;

        for (AActor* Actor : Players)
        {
            ABaseCharacter* Character = Cast<ABaseCharacter>(Actor);

            if (!Character)
            {
                continue;
            }

            if (Character->bIsDead)
            {
                continue;
            }

            float Distance = FVector::Dist(
                GetActorLocation(),
                Character->GetActorLocation());

            if (!CanSeePlayer(Character))
            {
                continue;
            }

            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                Player = Character;
            }
        }
    }

    if (!Player)
    {
        if (AAIController* AI = Cast<AAIController>(GetController()))
        {
            AI->StopMovement();
        }

        GetCharacterMovement()->StopMovementImmediately();

        GetCharacterMovement()->Velocity = FVector::ZeroVector;

        bIsAttacking = false;
        bIsChasing = false;

        return;
    }

    float Distance = FVector::Dist(
        GetActorLocation(),
        Player->GetActorLocation());

    if (Distance <= 250.f)
    {
        bIsChasing = false;

        GetCharacterMovement()->StopMovementImmediately();

        if (bCanAttack)
        {
            bIsAttacking = true;
            bCanAttack = false;

            if (Player)
            {
                Player->TakePlayerDamage(10.f);
            }

            GetWorldTimerManager().SetTimer(
                AttackTimerHandle,
                this,
                &AMonster::ResetAttack,
                AttackCooldown,
                false);
        }
    }
    else
    {
        AAIController* AI = Cast<AAIController>(GetController());

        if (AI)
        {
            AI->MoveToActor(
                Player,
                100.f,
                true,
                true,
                true,
                nullptr,
                true);

            bIsChasing = true;
        }

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

void AMonster::DestroyMonster()
{
    GetWorldTimerManager().SetTimer(
        DeathTimerHandle,
        this,
        &AMonster::DestroyMonsterDelay,
        0.4f,
        false
    );
}

void AMonster::DestroyMonsterDelay()
{
    Destroy();
}

void AMonster::ApplyTaunt(ABaseCharacter* Target)
{
    if (!Target)
    {
        return;
    }

    bIsTaunted = true;
    TauntTarget = Target;

    UE_LOG(LogTemp, Warning,
        TEXT("Monster Taunted"));

    GetWorldTimerManager().ClearTimer(TauntTimerHandle);

    GetWorldTimerManager().SetTimer(
        TauntTimerHandle,
        this,
        &AMonster::ClearTaunt,
        5.f,
        false);
}

void AMonster::ClearTaunt()
{
    bIsTaunted = false;
    TauntTarget = nullptr;

    UE_LOG(LogTemp, Warning,
        TEXT("Taunt End"));
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

    float Distance = FVector::Dist(GetActorLocation(), OwnerAltar->GetActorLocation());

    if (Distance > OwnerAltar->LightRange->GetScaledSphereRadius())
    {
        UE_LOG(LogTemp, Warning, TEXT("Outside Light"));

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

        CurrentHP = 0.f;

        UE_LOG(LogTemp, Warning, TEXT("Monster Dead"));

        bool bLastMonster = false;

        if (DungeonManager)
        {
            bLastMonster = DungeonManager->OnMonsterDead();
        }

        SpawnCoin();

        if (bLastMonster)
        {
            SpawnPrism();

            UTheNightfallSiegeInstance* GI =
                Cast<UTheNightfallSiegeInstance>(GetGameInstance());

            if (GI)
            {
                GI->SkillPoints += 2;

                TArray<AActor*> Players;

                UGameplayStatics::GetAllActorsOfClass(
                    GetWorld(),
                    ABaseCharacter::StaticClass(),
                    Players);

                for (AActor* Actor : Players)
                {
                    ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);

                    if (!Player)
                    {
                        continue;
                    }

                    Player->SkillPoints = GI->SkillPoints;
                }
            }
        }

        // 이동 정지
        GetCharacterMovement()->DisableMovement();

        // 충돌 제거
        GetCapsuleComponent()->SetCollisionEnabled(
            ECollisionEnabled::NoCollision);

        // 공격 중지
        bCanAttack = false;
        bIsAttacking = false;

        // AI 정지
        if (AAIController* AI = Cast<AAIController>(GetController()))
        {
            AI->StopMovement();
        }

        // 죽는 애니메이션
        if (DeathMontage)
        {
            GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PlayAnimMontage(DeathMontage);
        }
    }
}

bool AMonster::CanSeePlayer(ABaseCharacter* Player)
{
    if (!Player)
    {
        return false;
    }

    FVector ToPlayer =
        Player->GetActorLocation() - GetActorLocation();

    float Distance = ToPlayer.Length();

    if (Distance > SightRange)
    {
        return false;
    }

    ToPlayer.Normalize();

    FVector Forward = GetActorForwardVector();

    float Dot = FVector::DotProduct(
        Forward,
        ToPlayer);

    float Angle =
        FMath::RadiansToDegrees(
            FMath::Acos(Dot));

    if (Angle > SightAngle * 0.5f)
    {
        return false;
    }

    /*FHitResult Hit;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        GetActorLocation() + FVector(0, 0, 50),
        Player->GetActorLocation() + FVector(0, 0, 50),
        ECC_Visibility,
        Params);

    if (!bHit)
    {
        return false;
    }

    return Hit.GetActor() == Player;*/

    return true;
}

void AMonster::SpawnCoin()
{
    if (!CoinClass)
    {
        return;
    }

    FVector SpawnLocation =
        GetActorLocation() + FVector(0, 0, 0);

    GetWorld()->SpawnActor<ACoin>(
        CoinClass,
        SpawnLocation,
        FRotator::ZeroRotator);
}

void AMonster::SpawnPrism()
{
    if (!PrismClass)
    {
        return;
    }

    FVector SpawnLocation =
        GetActorLocation() + FVector(50.f, 50.f, 0.f);

    GetWorld()->SpawnActor<ADungeonPrism>(
        PrismClass,
        SpawnLocation,
        FRotator::ZeroRotator);
}

void AMonster::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMonster, CurrentHP);
    DOREPLIFETIME(AMonster, MaxHP);
}   

