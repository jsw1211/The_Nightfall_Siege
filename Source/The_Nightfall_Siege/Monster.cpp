// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCharacter.h"
#include "DungeonManager.h"
#include "Altar.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "Coin.h"
#include "DungeonPrism.h"
#include "BasePlayerState.h"
#include "TheNightfallSiegeInstance.h"
#include "Net/UnrealNetwork.h"

namespace
{
    FVector FindMonsterDropGroundLocation(
        const UWorld* World,
        const AActor* ActorToIgnore,
        const FVector& DesiredLocation)
    {
        if (!World)
        {
            return DesiredLocation;
        }

        FHitResult GroundHit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DropGroundTrace), false);
        QueryParams.AddIgnoredActor(ActorToIgnore);

        const FVector TraceStart = DesiredLocation + FVector(0.f, 0.f, 500.f);
        const FVector TraceEnd = DesiredLocation - FVector(0.f, 0.f, 5000.f);
        if (World->LineTraceSingleByChannel(
                GroundHit,
                TraceStart,
                TraceEnd,
                ECC_Visibility,
                QueryParams))
        {
            return GroundHit.ImpactPoint;
        }

        return DesiredLocation;
    }
}

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

    // AI targeting, attack cooldowns, and HP changes must run only on the
    // server.  Replicated monsters also tick on clients; without this guard a
    // local client independently dealt the same 10 damage a second time.
    if (!HasAuthority())
    {
        return;
    }

    // Keep the navigation completion radius inside attack range. Using the
    // exact same value can leave an AI on the acceptance boundary, outside
    // the damage check due to path-following tolerance.
    constexpr float AttackRange = 100.f;
    constexpr float ChaseStopRange = 75.f;

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

    // Compare the gap between collision capsules, not their center points.
    // A large werewolf capsule may be unable to get its center within the
    // attack range before colliding with the player's capsule.
    const float CenterDistance = FVector::Dist2D(
        GetActorLocation(),
        Player->GetActorLocation());
    const float MonsterCollisionRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float PlayerCollisionRadius = Player->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Distance = FMath::Max(
        0.f,
        CenterDistance - MonsterCollisionRadius - PlayerCollisionRadius);

    if (Distance <= AttackRange)
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
                ChaseStopRange,
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

void AMonster::MulticastShowDamage_Implementation(float Damage)
{
    ShowDamage(Damage);
}

void AMonster::MulticastPlayDeath_Implementation()
{
    if (DeathMontage)
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PlayAnimMontage(DeathMontage);
    }
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

    if (!OwnerAltar->IsInsideActiveLightZone(GetActorLocation()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Outside Altar Light Semicircle"));

        return;
    }

    // 이미 죽었으면 무시
    if (bIsDead)
    {
        return;
    }

    CurrentHP -= Damage;

    MulticastShowDamage(Damage);

    UE_LOG(LogTemp, Warning, TEXT("Monster HP: %f"), CurrentHP);

    if (CurrentHP <= 0)
    {
        bIsDead = true;

        CurrentHP = 0.f;

        UE_LOG(LogTemp, Warning, TEXT("Monster Dead"));

        bool bLastMonster = false;
		const bool bAltarCleared = OwnerAltar->NotifyOwnedMonsterDefeated();
		if (bAltarCleared)
		{
			UE_LOG(LogTemp, Warning, TEXT("Altar wave complete: %s"), *OwnerAltar->GetName());
		}

        if (DungeonManager)
        {
            bLastMonster = DungeonManager->OnMonsterDead();
        }

        if (!DungeonManager || !DungeonManager->IsDebugClearInProgress())
        {
            SpawnCoin();
        }

        if (bLastMonster)
        {
            if (!DungeonManager || !DungeonManager->IsDebugClearInProgress())
            {
                SpawnPrism();
            }

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

                    if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
                    {
                        PS->SkillPoints = Player->SkillPoints;
                        PS->NotifyDungeonCleared();
                        Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
                    }
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
        MulticastPlayDeath();
    }
}

bool AMonster::CanSeePlayer(ABaseCharacter* Player)
{
    if (!Player)
    {
        return false;
    }

    // Detect players in a circle centered on the monster, independent of
    // facing direction. DistSquared2D keeps the detection range horizontal.
    const float DistanceSquared = FVector::DistSquared2D(
        GetActorLocation(),
        Player->GetActorLocation());

    if (DistanceSquared > FMath::Square(SightRange))
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

    // A character's actor origin is at the center of its capsule, not its
    // feet. Trace to the dungeon floor so dropped coins do not remain in air.
    const FVector SpawnLocation = FindMonsterDropGroundLocation(
        GetWorld(),
        this,
        GetActorLocation()) + FVector(0.f, 0.f, 50.f);

    ACoin* SpawnedCoin = GetWorld()->SpawnActor<ACoin>(
        CoinClass,
        SpawnLocation,
        FRotator(0.f, 90.f, 0.f));

    if (SpawnedCoin && SpawnedCoin->Mesh)
    {
        // Match the visual size and pickup radius.
        SpawnedCoin->Mesh->SetRelativeScale3D(
            SpawnedCoin->Mesh->GetRelativeScale3D() * 2.f);
    }

    if (SpawnedCoin && SpawnedCoin->Sphere)
    {
        SpawnedCoin->Sphere->SetSphereRadius(
            SpawnedCoin->Sphere->GetUnscaledSphereRadius() * 2.f);
    }
}

void AMonster::SpawnPrism()
{
    if (!PrismClass)
    {
        return;
    }

    const FVector SpawnLocation = FindMonsterDropGroundLocation(
        GetWorld(),
        this,
        GetActorLocation() + FVector(50.f, 50.f, 0.f));

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

