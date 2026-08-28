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
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "AIController.h"
#include "Coin.h"
#include "DungeonPrism.h"
#include "BasePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"


namespace
{
	bool IsInsideAnyActiveAltarLightZone(
		const UWorld* World,
		const AMonster* Monster)
	{
		if (!World || !Monster)
		{
			return false;
		}

		const FVector WorldLocation = Monster->GetActorLocation();
		const UCapsuleComponent* Capsule = Monster->GetCapsuleComponent();
		const float FootprintRadius = Capsule
			? Capsule->GetScaledCapsuleRadius()
			: 0.f;

		// Vulnerability belongs to the illuminated ground area, not to the
		// monster's spawn/owner altar. A monster from another altar must also lose
		// invulnerability while standing anywhere above this XY circle.
		for (TActorIterator<AAltar> It(World); It; ++It)
		{
			if (It->IsInsideActiveLightZone(WorldLocation, FootprintRadius))
			{
				return true;
			}
		}

		return false;
	}

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

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HitFlashMaterialAsset(
        TEXT("/Game/BP_Monster/Shared/M_MonsterHitFlash.M_MonsterHitFlash"));
    if (HitFlashMaterialAsset.Succeeded())
    {
        HitFlashOverlayMaterial = HitFlashMaterialAsset.Object;
    }

    BarrierFXComponent = CreateDefaultSubobject<UNiagaraComponent>(
        TEXT("BarrierFXComponent"));

    BarrierFXComponent->SetupAttachment(GetMesh());
    BarrierFXComponent->SetAutoActivate(false);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BarrierAsset(
        TEXT("/Game/Effects/7_WereWolf/Barrier/NS_WereWolf_Barrier.NS_WereWolf_Barrier"));

    if (BarrierAsset.Succeeded())
    {
        BarrierFX = BarrierAsset.Object;
        BarrierFXComponent->SetAsset(BarrierFX);
    }
}

// Called when the game starts or when spawned
void AMonster::BeginPlay()
{
	Super::BeginPlay();

	// Path following should never let a monster step off the navigable floor,
	// even if avoidance or a short movement correction points over an edge.
	GetCharacterMovement()->bCanWalkOffLedges = false;
	
    DungeonManager = Cast<ADungeonManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ADungeonManager::StaticClass()));

    // Monsters and players can be spawned in either order (including on
    // replicated clients). Ignore only their movement sweeps in both
    // directions; the Pawn collision profile remains untouched for damage,
    // monster-to-monster blocking, and floor collision.
    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ABaseCharacter::StaticClass(),
        Players);

    for (AActor* Actor : Players)
    {
        if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
        {
            MoveIgnoreActorAdd(Player);
            Player->MoveIgnoreActorAdd(this);
        }
    }
    
    if (UUserWidget* Widget =
        HPWidget->GetUserWidgetObject())
    {
    }

    if (HasAuthority())
    {
        if (!IsInsideAnyActiveAltarLightZone(GetWorld(), this))
        {
            bBarrierActive = true;
        }
    }

    // 서버와 클라이언트 모두 자기 베리어를 생성
    if (bBarrierActive)
    {
        EnsureBarrierFX();
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

    const bool bInsideLanternZone =
        IsInsideAnyActiveAltarLightZone(GetWorld(), this);

    if (!bInsideLanternZone && !bBarrierActive)
    {
        bBarrierActive = true;
        OnRep_BarrierActive();
    }
    else if (bInsideLanternZone && bBarrierActive)
    {
        bBarrierActive = false;
        OnRep_BarrierActive();
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
    PlayHitFlash();
}

void AMonster::PlayHitFlash()
{
    if (!GetMesh() || !HitFlashOverlayMaterial)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
    GetMesh()->SetOverlayMaterial(HitFlashOverlayMaterial);

    if (HitFlashDuration <= 0.f)
    {
        ClearHitFlash();
        return;
    }

    GetWorldTimerManager().SetTimer(
        HitFlashTimerHandle,
        this,
        &AMonster::ClearHitFlash,
        HitFlashDuration,
        false);
}

void AMonster::ClearHitFlash()
{
    if (GetMesh())
    {
        GetMesh()->SetOverlayMaterial(nullptr);
    }
}

void AMonster::MulticastPlayDeath_Implementation()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        // A dead monster must no longer block pawns or receive gameplay
        // overlaps, but removing the capsule entirely lets root motion and
        // network corrections carry it through the dungeon floor.
        Capsule->SetGenerateOverlapEvents(false);
        Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
        Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (USkeletalMeshComponent* MonsterMesh = GetMesh())
    {
        MonsterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
    }
}

void AMonster::TakeMonsterDamage(float Damage)
{
    if (!OwnerAltar)
    {
        return;
    }

    // A placed lantern creates an infinite-height cylinder over its visible
    // XY ground circle. Altar ownership and Z height do not affect immunity.
	if (!IsInsideAnyActiveAltarLightZone(GetWorld(), this))
    {
        UE_LOG(LogTemp, Warning, TEXT("Monster outside active altar light circle"));

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

            // Debug dungeon clears grant this reward explicitly after every
            // monster is processed, so it cannot be missed or awarded twice.
            if (!DungeonManager || !DungeonManager->IsDebugClearInProgress())
            {
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

                    if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
                    {
						// PlayerState is the per-player durable balance. Preserve any
						// unspent points and add exactly two for this clear.
						PS->SkillPoints = FMath::Max(0, PS->SkillPoints) + 2;
						Player->SkillPoints = PS->SkillPoints;
                        PS->NotifyDungeonCleared();
						PS->ForceNetUpdate();
                        Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
                    }

					Player->OnRep_SkillPoints();
					Player->ForceNetUpdate();
                }
            }
        }

        // 이동 정지
        GetCharacterMovement()->DisableMovement();

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
    DOREPLIFETIME(AMonster, bBarrierActive);
    DOREPLIFETIME(AMonster, bIsAttacking);
    DOREPLIFETIME(AMonster, bIsChasing);
}


void AMonster::OnRep_BarrierActive()
{
    if (bBarrierActive)
    {
        EnsureBarrierFX();
    }
    else
    {
        if (IsValid(BarrierFXComponent))
        {
            BarrierFXComponent->DeactivateImmediate();
            BarrierFXComponent->SetVisibility(false, true);
        }
    }
}


void AMonster::EnsureBarrierFX()
{
    if (!BarrierFX || !GetMesh())
    {
        return;
    }

    if (IsValid(BarrierFXComponent))
    {
        BarrierFXComponent->SetAsset(BarrierFX);
        BarrierFXComponent->SetVisibility(true, true);
        BarrierFXComponent->SetHiddenInGame(false, true);
        BarrierFXComponent->Activate(true);
        return;
    }

    BarrierFXComponent =
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            BarrierFX,
            GetMesh(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            false);

    if (BarrierFXComponent)
    {
        BarrierFXComponent->SetVisibility(true, true);
        BarrierFXComponent->SetHiddenInGame(false, true);
        BarrierFXComponent->Activate(true);
    }
}
