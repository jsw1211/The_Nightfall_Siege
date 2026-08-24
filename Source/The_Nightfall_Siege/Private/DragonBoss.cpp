// Fill out your copyright notice in the Description page of Project Settings.


#include "DragonBoss.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DungeonPrism.h"
#include "BasePlayerState.h"
#include "DragonBreathProjectile.h"
#include "DangerZone.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "The_Nightfall_SiegeGameMode.h"

// Sets default values
ADragonBoss::ADragonBoss()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// ACharacter must retain its root capsule for CharacterMovement.  Ignore
	// gameplay projectiles on that broad capsule so only the bone-following
	// hitboxes below can receive weapon/arrow damage.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	// Reflectors use a dedicated object channel. Block their actual mesh only,
	// allowing the boss to approach without introducing an exclusion radius.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);

	auto ConfigureHitbox = [this](TObjectPtr<UCapsuleComponent>& Hitbox, const TCHAR* Name,
		const FName BoneName, float Radius, float HalfHeight)
	{
		Hitbox = CreateDefaultSubobject<UCapsuleComponent>(Name);
		Hitbox->SetupAttachment(GetMesh(), BoneName);
		// Capsule dimensions below are calculated from world-space bone positions.
		// Do not inherit the mesh scale as well, otherwise a scaled dragon makes
		// the hitbox grow twice (once in the bone distance, once as a child).
		Hitbox->SetAbsolute(false, false, true);
		Hitbox->SetCapsuleSize(Radius, HalfHeight);
		// Capsule components extend along local Z.  Rotate them onto the dragon's
		// length axis; individual placement can still be refined in BP_DragonBoss.
		Hitbox->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Hitbox->SetCollisionObjectType(ECC_Pawn);
		Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
		Hitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		// Area attacks query the Pawn channel, while arrows/weapons use
		// WorldDynamic.  Both must see the segmented hitboxes.
		Hitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Hitbox->SetGenerateOverlapEvents(true);
		Hitbox->SetCanEverAffectNavigation(false);
	};

	ConfigureHitbox(HeadHitbox, TEXT("HeadHitbox"), TEXT("Head2"), 65.f, 200.f);

	BodyHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("BodyHitbox"));
	BodyHitbox->SetupAttachment(GetMesh(), TEXT("Body1"));
	BodyHitbox->SetAbsolute(false, false, true);
	BodyHitbox->SetBoxExtent(FVector(270.f, 375.f, 500.f));
	BodyHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyHitbox->SetCollisionObjectType(ECC_Pawn);
	BodyHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyHitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BodyHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BodyHitbox->SetGenerateOverlapEvents(true);
	BodyHitbox->SetCanEverAffectNavigation(false);

	ConfigureHitbox(TailHitbox, TEXT("TailHitbox"), TEXT("Tail3"), 80.f, 420.f);

	BarrierFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BarrierFXComponent"));
	BarrierFXComponent->SetupAttachment(GetMesh());
	// The dragon mesh is intentionally scaled up in BP; keep the barrier at
	// the Niagara system's authored scale instead of inheriting that scale.
	BarrierFXComponent->SetAbsolute(false, false, true);
	BarrierFXComponent->SetWorldScale3D(FVector::OneVector);
	BarrierFXComponent->SetAutoActivate(false);

	// Keep these gameplay effects wired even when the BP defaults have not
	// explicitly overridden the corresponding C++ properties.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BlackoutChargingAsset(
		TEXT("/Game/Effects/6_Dragon/Blackout_Debuff/NS_Dragon_Blackout_Charging.NS_Dragon_Blackout_Charging"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BlackoutReleaseAsset(
		TEXT("/Game/Effects/6_Dragon/Blackout_Debuff/NS_Dragon_Blackout_Release.NS_Dragon_Blackout_Release"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PhaseTwoAsset(
		TEXT("/Game/Effects/6_Dragon/Second_Phase/NS_Dragon_FX.NS_Dragon_FX"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PhaseTwoMaterialAsset(
		TEXT("/Game/Effects/6_Dragon/Second_Phase/M_Dragon_FX.M_Dragon_FX"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BarrierAsset(
		TEXT("/Game/Effects/6_Dragon/Barrier/NS_Dragon_Barrier.NS_Dragon_Barrier"));
	static ConstructorHelpers::FClassFinder<ADragonBreathProjectile> BreathProjectileAsset(
		TEXT("/Game/BP_Monster/Dragon/BP_DragonBreathProjectile"));

	if (BlackoutChargingAsset.Succeeded())
	{
		BlackoutChargingFX = BlackoutChargingAsset.Object;
	}

	if (BlackoutReleaseAsset.Succeeded())
	{
		BlackoutReleaseFX = BlackoutReleaseAsset.Object;
	}

	if (PhaseTwoAsset.Succeeded())
	{
		PhaseTwoFX = PhaseTwoAsset.Object;
	}

	if (PhaseTwoMaterialAsset.Succeeded())
	{
		PhaseTwoOverlayMaterial = PhaseTwoMaterialAsset.Object;
	}

	if (BarrierAsset.Succeeded())
	{
		BarrierFX = BarrierAsset.Object;
		BarrierFXComponent->SetAsset(BarrierFX);
	}

	if (BreathProjectileAsset.Succeeded())
	{
		BreathProjectileClass = BreathProjectileAsset.Class;
	}

}

void ADragonBoss::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// This also runs while editing BP_DragonBoss, so the collision preview is
	// fitted immediately when the actor or skeletal mesh scale is changed.
	UpdateDamageHitboxes();
}

// Called when the game starts or when spawned
void ADragonBoss::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_GameTraceChannel2,
		ECR_Block);

	// 서버에서만 게임 상태 초기화
	if (HasAuthority())
	{
		ArenaCenter = FVector(0.f, 0.f, 0.f);

		bShielded = true;
		bCanTakeDamage = false;

		CurrentState = EDragonState::Idle;

		UpdatePlayerList();
		bFirstBreathDone = false;

		ForceNetUpdate();
	}

	// 서버/클라이언트 모두 자기 로컬 베리어 생성
	if (bShielded)
	{
		EnsureBarrierFX();
	}
}

// Called every frame
void ADragonBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateDamageHitboxes();

	if (!HasAuthority())
	{
		return;
	}

	const FVector ClampedCurrentLocation = ClampToMovementBounds(GetActorLocation());
	if (!ClampedCurrentLocation.Equals(GetActorLocation(), KINDA_SMALL_NUMBER))
	{
		GetCharacterMovement()->StopMovementImmediately();
		SetActorLocation(ClampedCurrentLocation);
	}

	if (!bEncounterStarted)
	{
		UpdatePlayerList();
		const float StartRangeSquared = FMath::Square(EncounterStartRange);

		for (ABaseCharacter* Player : AlivePlayers)
		{
			if (!Player ||
				FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > StartRangeSquared)
			{
				continue;
			}

			bEncounterStarted = true;
			TargetPlayer = Player;
			bFirstBreathDone = true;
			UE_LOG(LogTemp, Warning,
				TEXT("Dragon encounter started by %s at %.0f units"),
				*Player->GetName(),
				FMath::Sqrt(FVector::DistSquared(GetActorLocation(), Player->GetActorLocation())));
			return;
		}

		return;
	}

	if (bCenterTracking && TargetPlayer)
	{
		FVector Direction = TargetPlayer->GetActorLocation() - GetActorLocation();

		Direction.Z = 0.f;

		FRotator TargetRotation = Direction.Rotation();

		SetActorRotation(TargetRotation);

		if (CurrentBreathZone)
		{
			// The warning is a ground rectangle from the dragon itself to the
			// selected target, rather than a socket-relative decal that can drift
			// sideways as the head animation moves.
			FVector LineStart = GetActorLocation();
			FVector LineEnd = TargetPlayer->GetActorLocation();
			LineEnd.Z = LineStart.Z;
			FVector LineDirection = LineEnd - LineStart;

			const float TargetDistance = LineDirection.Size2D();
			const float TelegraphLength = FMath::Clamp(
				TargetDistance + 500.f, 1000.f, BreathTelegraphRange);
			LineDirection = LineDirection.GetSafeNormal2D();
			FVector SpawnLocation = LineStart + LineDirection * (TelegraphLength * 0.5f);
			FRotator LineRotation = LineDirection.Rotation();
			LineRotation.Pitch = -90.f;

			CurrentBreathZone->SetActorLocation(SpawnLocation);

			CurrentBreathZone->SetActorRotation(LineRotation);
			CurrentBreathZone->LineLength = TelegraphLength;
			CurrentBreathZone->OnRep_LineLength();
			CurrentBreathZone->ForceNetUpdate();
		}
	}

	if (bCenterMechanicActive)
	{
		if (!bCenterBreathStarted)
		{
			const FVector PreviousLocation = GetActorLocation();
			const FVector NewLocation = FMath::VInterpConstantTo(
				PreviousLocation, ArenaCenter, DeltaTime, 1200.f);

			MoveWithSweepAndSlide(NewLocation);

			if (DeltaTime > UE_SMALL_NUMBER)
			{
				GetCharacterMovement()->Velocity =
					(GetActorLocation() - PreviousLocation) / DeltaTime;
			}

			FVector CenterDirection = ArenaCenter - GetActorLocation();
			CenterDirection.Z = 0.f;
			if (!CenterDirection.IsNearlyZero())
			{
				SetActorRotation(CenterDirection.Rotation());
			}
		}

		float Distance =
			FVector::Dist(
				GetActorLocation(),
				ArenaCenter);

		if (Distance <= 200.f && !bCenterBreathStarted)
		{
			bCenterBreathStarted = true;

			UE_LOG(LogTemp, Warning,
				TEXT("Center Arrived"));

			bCenterTracking = true;

			StartAttackTelegraph(
				EDragonAttackType::Breath);

			return;
		}

		return;
	}

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	if (!bFirstBreathDone)
	{
		return;
	}

	if (CurrentState == EDragonState::Leap)
	{
		return;
	}

	if (CurrentState == EDragonState::Flying)
	{
		FlyToTarget();
		return;
	}

	if (CurrentState == EDragonState::Landing)
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
	}

	if (bCenterMechanicActive)
	{
		return;
	}

	if (TargetPlayer && !bIsAttacking && !bStunned && !bIsTelegraphing)
	{
		float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		const float EffectiveEngageRange = FMath::Max(AttackEngageRange, AttackStartRange + 150.f);

		if (bIsFlying)
		{
			FlyToTarget();
		}
		else
		{
			if (Distance > FlyStartRange)
			{
				FlyToTarget();
			}
			else if (Distance > EffectiveEngageRange)
			{
				WalkToTarget();
			}
			else if (Distance <= EffectiveEngageRange)
			{
				if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
				{
					StartAttackCycle();
				}
			}
		}
	}
}

void ADragonBoss::UpdateDamageHitboxes()
{
	USkeletalMeshComponent* DragonMesh = GetMesh();
	if (!DragonMesh)
	{
		return;
	}

	// Each capsule is constructed from two live bone locations rather than
	// fixed component offsets.  This keeps it fitted while the neck/tail bend
	// during attack, flight, and death animations.
	auto FitCapsuleToBones = [DragonMesh](UCapsuleComponent* Hitbox,
		const FName StartBone, const FName EndBone, float RadiusRatio)
	{
		if (!Hitbox ||
			DragonMesh->GetBoneIndex(StartBone) == INDEX_NONE ||
			DragonMesh->GetBoneIndex(EndBone) == INDEX_NONE)
		{
			return;
		}

		const FVector Start = DragonMesh->GetBoneLocation(StartBone);
		const FVector End = DragonMesh->GetBoneLocation(EndBone);
		const FVector Segment = End - Start;
		const float SegmentLength = Segment.Size();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		// Derive every dimension from the live world-space bones.  Deliberately
		// avoid absolute size clamps so changing the dragon's Actor/Mesh scale
		// scales its hitboxes by exactly the same factor.
		const float Radius = FMath::Max(SegmentLength * RadiusRatio, 1.f);
		const float HalfHeight = (SegmentLength * 0.5f) + Radius;
		const FQuat Rotation = FQuat::FindBetweenNormals(
			FVector::UpVector, Segment / SegmentLength);

		Hitbox->SetCapsuleSize(Radius, HalfHeight, false);
		Hitbox->SetWorldLocationAndRotation(
			(Start + End) * 0.5f, Rotation, false, nullptr,
			ETeleportType::TeleportPhysics);
	};

	// The body is a box so its horizontal and vertical thickness can be
	// independently widened beyond the capsule-shaped head and tail hitboxes.
	auto FitBodyBoxToBones = [DragonMesh](UBoxComponent* Hitbox,
		const FName StartBone, const FName EndBone, float RadiusRatio)
	{
		if (!Hitbox ||
			DragonMesh->GetBoneIndex(StartBone) == INDEX_NONE ||
			DragonMesh->GetBoneIndex(EndBone) == INDEX_NONE)
		{
			return;
		}

		const FVector Start = DragonMesh->GetBoneLocation(StartBone);
		const FVector End = DragonMesh->GetBoneLocation(EndBone);
		const FVector Segment = End - Start;
		const float SegmentLength = Segment.Size();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float Radius = FMath::Max(SegmentLength * RadiusRatio, 1.f);
		const FQuat Rotation = FQuat::FindBetweenNormals(
			FVector::ForwardVector, Segment / SegmentLength);

		// SetBoxExtent uses half-extents: Y is horizontal width, Z is height.
		Hitbox->SetBoxExtent(FVector(
			SegmentLength * 0.5f,
			Radius * 1.5f,
			Radius * 2.0f), false);
		Hitbox->SetWorldLocationAndRotation(
			(Start + End) * 0.5f, Rotation, false, nullptr,
			ETeleportType::TeleportPhysics);
	};

	// The ratios create a narrow head, broad body, and tapered tail silhouette.
	FitCapsuleToBones(HeadHitbox, TEXT("Neck1"), TEXT("Head3"), 0.40f);
	FitBodyBoxToBones(BodyHitbox, TEXT("Neck1"), TEXT("Tail1"), 0.38f);
	FitCapsuleToBones(TailHitbox, TEXT("Tail1"), TEXT("Tail4"), 0.12f);
}

FVector ADragonBoss::ClampToMovementBounds(const FVector& DesiredLocation) const
{
	FVector ClampedLocation = DesiredLocation;
	const FVector2D HorizontalLocation(DesiredLocation.X, DesiredLocation.Y);
	const float RadiusSquared = FMath::Square(MovementBoundaryRadius);

	if (HorizontalLocation.SizeSquared() > RadiusSquared)
	{
		const FVector2D BoundaryLocation =
			HorizontalLocation.GetSafeNormal() * MovementBoundaryRadius;
		ClampedLocation.X = BoundaryLocation.X;
		ClampedLocation.Y = BoundaryLocation.Y;
	}

	return ClampedLocation;
}

// Called to bind functionality to input
void ADragonBoss::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void ADragonBoss::StartAttackCycle()
{
	if (!HasAuthority() || IsActorBeingDestroyed() || CurrentState == EDragonState::Dead)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(
		AttackTimerHandle);

	if (bStunned)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("StartAttackCycle"));

	UpdatePlayerList();

	if (AlivePlayers.Num() == 0)
	{
		return;
	}
	//ChooseRandomTarget();

	float RandomDelay = FMath::RandRange(1.f, 2.f); // 디버그용으로 1~2초

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ADragonBoss::ExecutePattern,
		RandomDelay,
		false
	);
}

EDragonAttackType ADragonBoss::ChooseRandomAttack()
{
	int32 Rand = FMath::RandRange(1, 100);

	if (Rand <= 50)
	{
		return EDragonAttackType::Bite;
	}

	if (Rand <= 90)
	{
		return EDragonAttackType::CloseBreath;
	}

	return EDragonAttackType::Debuff;
}

void ADragonBoss::ExecuteRandomAttack()
{
	if (!HasAuthority())
	{
		return;
	}

	EDragonAttackType AttackType =
		ChooseRandomAttack();

	StartAttackTelegraph(
		AttackType);
}

void ADragonBoss::BiteAttack()
{
	///////////////////////////////////// Debug Log
	BiteCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] Bite (%d / Total:%d)"),
		BiteCount,
		TotalPatternCount);
	/////////////////////////////////////
	SetActorRotation(TelegraphRotation);

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Bite"));

	MulticastPlayAttack(EDragonAttackType::Bite);

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		this,
		&ADragonBoss::OnAttackFinished,
		2.0f,
		false
	);
}

void ADragonBoss::CloseBreathAttack()
{
	////////////////////////////////
	CloseBreathCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] CloseBreath (%d / Total:%d)"),
		CloseBreathCount,
		TotalPatternCount);
	/////////////////////////////////
	SetActorRotation(TelegraphRotation);

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Close Breath"));

	MulticastPlayAttack(EDragonAttackType::CloseBreath);

	// The damage notify happens before this animation is over. Keep the attack
	// state (and therefore movement/pattern selection) locked until the montage
	// actually finishes instead of releasing it at a fixed time.
	if (CloseBreathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ADragonBoss::OnCloseBreathMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, CloseBreathMontage);
			return;
		}
	}

	// Fallback only for an unassigned montage or unavailable anim instance.
	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		this,
		&ADragonBoss::OnAttackFinished,
		3.0f,
		false
	);
}

void ADragonBoss::OnCloseBreathMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (HasAuthority() && Montage == CloseBreathMontage)
	{
		OnAttackFinished();
	}
}

void ADragonBoss::BreathAttack()
{
	/////////////////////////////////////////
	TargetChangeBreathCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[TargetChange] Breath (%d)"),
		TargetChangeBreathCount);
	//////////////////////////////////////////
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	if (bCenterMechanicActive)
	{
		bMovementLockedForBreath = true;
		GetCharacterMovement()->DisableMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning, TEXT("Dragon Used Breath"));

	// AM_dragon_breath_attack's BreathFire notify calls BreathFire exactly
	// when the dragon's mouth opens. Center breath uses this same path.
	MulticastPlayAttack(EDragonAttackType::Breath);

	if (BreathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ADragonBoss::OnBreathMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, BreathMontage);
			return;
		}
	}

	// Only used if the montage is not assigned or an anim instance is missing.
	GetWorldTimerManager().SetTimer(AttackEndHandle, this,
		&ADragonBoss::OnAttackFinished, 4.0f, false);
}

void ADragonBoss::OnBreathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (HasAuthority() && Montage == BreathMontage)
	{
		OnAttackFinished();
	}
}

void ADragonBoss::StopBreathTracking()
{
	bCenterTracking = false;
	CurrentBreathZone = nullptr;
}

void ADragonBoss::StartBlackoutChargingFX()
{
	if (HasAuthority() && !IsActorBeingDestroyed() && CurrentState != EDragonState::Dead)
	{
		MulticastSpawnBlackoutChargingFX();
	}
}

void ADragonBoss::DebuffAttack()
{
	/////////////////////////////////
	DebuffCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] Debuff (%d / Total:%d)"),
		DebuffCount,
		TotalPatternCount);
	//////////////////////////////////
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = true;

	AAIController* AIController = Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	CurrentState = EDragonState::Attacking;

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Used Debuff"));

	ResetPrismCleanseParticipants();

	MulticastStopBlackoutChargingFX();

	// The telegraph has completed: release the blackout before applying it.
	MulticastSpawnBlackoutReleaseFX(GetActorLocation());

	TArray<AActor*> Prisms;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ADungeonPrism::StaticClass(),
		Prisms);

	for (AActor* Actor : Prisms)
	{
		ADungeonPrism* Prism =
			Cast<ADungeonPrism>(Actor);

		if (Prism)
		{
			Prism->ActivatePrism();
		}
	}

	UpdatePlayerList();

	for (ABaseCharacter* Player : AlivePlayers)
	{
		if (Player)
		{
			Player->bDarknessDebuff = true;

			UE_LOG(LogTemp, Warning,
				TEXT("%s Darkness Debuff"),
				*Player->GetName());
		}
	}

	GetWorldTimerManager().SetTimer(
		AttackEndHandle,
		this,
		&ADragonBoss::OnAttackFinished,
		2.5f,
		false
	);
}

void ADragonBoss::ResetPrismCleanseParticipants()
{
	PrismCleanseParticipants.Empty();
}

bool ADragonBoss::ArePrismHoldersGathered(const TArray<AActor*>& PlayerActors) const
{
	const float MaxDistanceSquared = FMath::Square(PrismCleanseGatherRadius);
	for (AActor* FirstActor : PlayerActors)
	{
		ABaseCharacter* First = Cast<ABaseCharacter>(FirstActor);
		if (!First || First->IsDead() || !First->bHasPrism) continue;

		for (AActor* SecondActor : PlayerActors)
		{
			ABaseCharacter* Second = Cast<ABaseCharacter>(SecondActor);
			if (!Second || Second == First || Second->IsDead() || !Second->bHasPrism) continue;

			if (FVector::DistSquared(First->GetActorLocation(), Second->GetActorLocation()) > MaxDistanceSquared)
			{
				return false;
			}
		}
	}

	return true;
}

void ADragonBoss::RegisterPrismCleanseParticipant(ABaseCharacter* Player)
{
	if (!HasAuthority() || !Player || Player->IsDead() || !Player->bHasPrism || !Player->bPrismEquipped || !Player->bDarknessDebuff)
	{
		return;
	}

	PrismCleanseParticipants.Add(Player);

	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), PlayerActors);
	int32 RequiredParticipants = 0;
	for (AActor* Actor : PlayerActors)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(Actor);
		if (Character && !Character->IsDead() && Character->bHasPrism)
		{
			++RequiredParticipants;
			if (!PrismCleanseParticipants.Contains(Character))
			{
				UE_LOG(LogTemp, Warning, TEXT("Prism cleanse waiting: %d/%d"), PrismCleanseParticipants.Num(), RequiredParticipants);
				return;
			}
		}
	}

	if (RequiredParticipants == 0)
	{
		return;
	}

	if (!ArePrismHoldersGathered(PlayerActors))
	{
		PrismCleanseParticipants.Empty();
		UE_LOG(LogTemp, Warning, TEXT("Prism cleanse failed: holders must gather within %.0f units"), PrismCleanseGatherRadius);
		return;
	}

	for (AActor* Actor : PlayerActors)
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(Actor))
		{
			Character->bDarknessDebuff = false;

			// RepNotifies are not invoked on the authoritative Listen Server.
			// Run the local presentation callback explicitly for its host pawn;
			// remote clients still receive the replicated update and run it there.
			if (Character->IsLocallyControlled())
			{
				Character->OnRep_DarknessDebuff();
			}

			Character->ForceNetUpdate();
		}
	}

	PrismCleanseParticipants.Empty();
	UE_LOG(LogTemp, Warning, TEXT("Group prism cleanse completed by %d players"), RequiredParticipants);
}

void ADragonBoss::WalkToTarget()
{
	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController && TargetPlayer)
	{
		const float EffectiveChaseStopRange = FMath::Clamp(ChaseStopRange, 100.f, AttackStartRange * 0.8f);
		AIController->MoveToLocation(
			ClampToMovementBounds(TargetPlayer->GetActorLocation()),
			EffectiveChaseStopRange);
	}
}

void ADragonBoss::FlyToTarget()
{
	if (!bIsFlying)
	{
		if (bIsLeaping)
		{
			return;
		}

		//////////////////////////////////////
		TargetChangeFlyCount++;

		UE_LOG(LogTemp, Warning,
			TEXT("[TargetChange] Fly (%d)"),
			TargetChangeFlyCount);
		//////////////////////////////////////

		bIsLeaping = true;

		CurrentState = EDragonState::Leap;

		MulticastPlayMovementTransition(false);
		// Fallback for missing/failed animation notifies: never remain in Leap.
		GetWorldTimerManager().SetTimer(LeapRecoveryHandle, this, &ADragonBoss::OnLeapFinished, 2.f, false);

		return;
	}

	float Distance =
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation());

	if (Distance <= FlyLandingRange)
	{
		bIsFlying = false;

		CurrentState =
			EDragonState::Landing;

		// Do not freeze in MOVE_Flying during the landing montage. Falling
		// immediately runs the normal walkable-floor/base checks, so a player
		// capsule or reflector cannot become its movement base. Keep a deliberate
		// lateral escape velocity as well: a purely vertical fall onto the flat
		// top of an unwalkable capsule would otherwise have no slide direction.
		FVector EscapeDirection = GetActorLocation() - TargetPlayer->GetActorLocation();
		EscapeDirection.Z = 0.f;
		if (!EscapeDirection.Normalize())
		{
			EscapeDirection = GetActorRightVector().GetSafeNormal2D();
		}

		UCharacterMovementComponent* LandingMovement = GetCharacterMovement();
		LandingMovement->SetMovementMode(MOVE_Falling);
		LandingMovement->Velocity = EscapeDirection * FMath::Max(ChaseStopRange, 500.f);

		MulticastPlayMovementTransition(true);
		GetWorldTimerManager().SetTimer(LandingRecoveryHandle, this, &ADragonBoss::OnLandFinished, 2.f, false);

		return;
	}

	FVector TargetLoc =
		TargetPlayer->GetActorLocation();

	TargetLoc.Z += 300.f;
	TargetLoc = ClampToMovementBounds(TargetLoc);

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FVector PreviousLocation = GetActorLocation();

	FVector NewLocation =
		FMath::VInterpConstantTo(
			PreviousLocation,
			TargetLoc,
			DeltaSeconds,
			1200.f);

	// Sweep the movement capsule so manual flight cannot teleport onto a player
	// or through a reflector. Slide on contact so the dragon keeps routing along
	// the real mesh instead of becoming stuck at an invisible exclusion radius.
	MoveWithSweepAndSlide(ClampToMovementBounds(NewLocation));

	// SetActorLocation does not provide a stable CharacterMovement velocity.
	// Keep it updated so animation blueprints never see a zero-speed frame
	// while the dragon is visibly flying. Use the actual swept result so a
	// blocked flight does not report movement that never occurred.
	if (DeltaSeconds > UE_SMALL_NUMBER)
	{
		GetCharacterMovement()->Velocity =
			(GetActorLocation() - PreviousLocation) / DeltaSeconds;
	}

	SetActorRotation(
		(TargetLoc - GetActorLocation())
		.Rotation());
}

void ADragonBoss::MoveWithSweepAndSlide(const FVector& DesiredLocation)
{
	const FVector MoveDelta = DesiredLocation - GetActorLocation();
	if (MoveDelta.IsNearlyZero())
	{
		return;
	}

	FHitResult MoveHit;
	SetActorLocation(DesiredLocation, true, &MoveHit);

	if (MoveHit.IsValidBlockingHit())
	{
		// CharacterMovement narrows this override to protected in UE 5.7, but
		// the public MovementComponent interface still dispatches to it.
		UMovementComponent* Movement = GetCharacterMovement();
		const FVector BeforeSlide = GetActorLocation();
		Movement->SlideAlongSurface(
			MoveDelta,
			1.f - MoveHit.Time,
			MoveHit.Normal,
			MoveHit,
			true);

		// A perfectly head-on hit projects to a zero slide vector. Take one
		// deterministic tangential step so repeated straight-line flight can
		// route around a player/reflector instead of retrying the same point.
		if (FVector::DistSquared(BeforeSlide, GetActorLocation()) <= 1.f)
		{
			FVector SideStepDirection = FVector::CrossProduct(
				FVector::UpVector,
				FVector(MoveHit.Normal.X, MoveHit.Normal.Y, 0.f));

			if (!SideStepDirection.Normalize())
			{
				SideStepDirection = GetActorRightVector().GetSafeNormal2D();
			}
			else if (FVector::DotProduct(SideStepDirection, GetActorRightVector()) < 0.f)
			{
				SideStepDirection *= -1.f;
			}

			const float RemainingFraction = FMath::Clamp(1.f - MoveHit.Time, 0.f, 1.f);
			const float SideStepDistance = MoveDelta.Size() * RemainingFraction;
			const FVector BeforeSideStep = GetActorLocation();
			FHitResult SideStepHit;
			AddActorWorldOffset(
				SideStepDirection * SideStepDistance,
				true,
				&SideStepHit);

			if (FVector::DistSquared(BeforeSideStep, GetActorLocation()) <= 1.f)
			{
				AddActorWorldOffset(
					SideStepDirection * -SideStepDistance,
					true);
			}
		}
	}
}

void ADragonBoss::FlyToCenter()
{
	bIsFlying = true;

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	CurrentState = EDragonState::Flying;

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Flying To Center"));

	UE_LOG(LogTemp, Error,
		TEXT("Moving To Center : %s"),
		*ArenaCenter.ToString());
}

void ADragonBoss::TakeBossDamage(float Damage)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bCanTakeDamage)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Dragon Is Invincible"));
		return;
	}

	CurrentHP = FMath::Max(
		0.f,
		CurrentHP - Damage);

	CheckPhaseTwo();

	MulticastShowDamage(Damage);

	ForceNetUpdate();

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon HP : %f"),
		CurrentHP);

	if (CurrentHP <= 0.f)
	{
		Die();
	}
}

bool ADragonBoss::IsWithinDamageRadius(const FVector& Location, float Radius) const
{
	const float DamageRadius = FMath::Max(0.f, Radius);
	const TObjectPtr<UPrimitiveComponent> Hitboxes[] =
	{
		HeadHitbox,
		BodyHitbox,
		TailHitbox
	};

	for (const UPrimitiveComponent* Hitbox : Hitboxes)
	{
		if (!IsValid(Hitbox) ||
			Hitbox->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}

		const FBoxSphereBounds Bounds = Hitbox->Bounds;
		const FVector2D HorizontalOffset(
			Location.X - Bounds.Origin.X,
			Location.Y - Bounds.Origin.Y);
		const float HitboxRadius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
		if (HorizontalOffset.SizeSquared() <= FMath::Square(DamageRadius + HitboxRadius))
		{
			return true;
		}
	}

	return false;
}

void ADragonBoss::OnBreathReflected()
{
	if (!HasAuthority())
	{
		return;
	}

	// A reflected projectile reaching the dragon always breaks an active
	// shield, regardless of which breath in the encounter produced it.
	if (bShielded)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shield Broken"));

		bShielded = false;
		bCanTakeDamage = true;
		OnRep_Shielded();

		bStunned = true;

		CurrentState = EDragonState::Attacking;

		GetCharacterMovement()->DisableMovement();

		GetWorldTimerManager().SetTimer(
			StunTimerHandle,
			this,
			&ADragonBoss::EndStun,
			5.f,
			false
		);
		return;
	}

	if (bCenterMechanicActive)
	{
		OnCenterMechanicSuccess();
		return;
	}

	{
		const float Damage = MaxHP * 0.1f;

		CurrentHP = FMath::Max(
			0.f,
			CurrentHP - Damage);

		CheckPhaseTwo();

		UE_LOG(LogTemp, Warning,
			TEXT("Reflect Damage : %f"),
			Damage);

		UE_LOG(LogTemp, Warning,
			TEXT("Boss HP : %f"),
			CurrentHP);

		if (CurrentHP <= 0.f)
		{
			Die();
		}
	}

}

void ADragonBoss::EndStun()
{
	UE_LOG(LogTemp, Error,
		TEXT("EndStun Called"));

	bStunned = false;
	bIsAttacking = false;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Walking
	);

	UE_LOG(LogTemp, Warning,
		TEXT("Stun End"));

	StartAttackCycle();
}

void ADragonBoss::UpdatePlayerList()
{
	AlivePlayers.Empty();

	TArray<AActor*> FoundPlayers;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ABaseCharacter::StaticClass(),
		FoundPlayers
	);

	for (AActor* Actor : FoundPlayers)
	{
		ABaseCharacter* Player =
			Cast<ABaseCharacter>(Actor);

		if (Player && !Player->IsDead())
		{
			AlivePlayers.Add(Player);
		}
	}
}

void ADragonBoss::ChooseRandomTarget()
{
	if (!HasAuthority())
	{
		return;
	}

	UpdatePlayerList();

	if (AlivePlayers.Num() == 0)
	{
		TargetPlayer = nullptr;
		return;
	}

	int32 RandomIndex =
		FMath::RandRange(
			0,
			AlivePlayers.Num() - 1
		);

	TargetPlayer = AlivePlayers[RandomIndex];

	UE_LOG(LogTemp, Warning,
		TEXT("New Target : %s"),
		*TargetPlayer->GetName());

	bIsLeaping = false;
	bIsFlying = false;
	bIsAttacking = false;

	CurrentState = EDragonState::Walking;
}

EDragonPatternType ADragonBoss::ChoosePattern()
{
	int32 Rand = FMath::RandRange(1, 100);

	if (Rand <= 60)
	{
		return EDragonPatternType::NormalAttack;
	}

	if (Rand <= 80)
	{
		return EDragonPatternType::TargetChange;
	}

	return EDragonPatternType::CenterMechanic;
}

void ADragonBoss::ExecutePattern()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();

		if (!TargetPlayer)
		{
			return;
		}
	}

	if (CurrentState == EDragonState::Flying ||
		CurrentState == EDragonState::Leap ||
		CurrentState == EDragonState::Landing)
	{
		return;
	}

	float Distance =
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation());
	const float EffectiveEngageRange = FMath::Max(AttackEngageRange, AttackStartRange + 150.f);

	UE_LOG(LogTemp, Warning,
		TEXT("ExecutePattern Distance = %f"),
		Distance);

	if (Distance > EffectiveEngageRange)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Target Out Of Range : %f"), Distance);

		if (Distance > FlyStartRange)
		{
			FlyToTarget();
		}
		else
		{
			WalkToTarget();
		}

		// Tick keeps the boss chasing and will start a new cycle only after it
		// enters the attack envelope.  Rescheduling here caused boundary spam.
		return;
	}

	if (bStunned)
	{
		return;
	}

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecutePattern"));

	EDragonPatternType Pattern = ChoosePattern();

	switch (Pattern)
	{
	case EDragonPatternType::NormalAttack:
		ExecuteRandomAttack();
		break;

	case EDragonPatternType::TargetChange:
		TargetChangePattern();
		break;

	case EDragonPatternType::CenterMechanic:
		CenterMechanicPattern();
		break;
	}
	///////////////////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("===== Dragon Pattern Statistics ====="));

	UE_LOG(LogTemp, Warning,
		TEXT("Bite             : %d"),
		BiteCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Close Breath     : %d"),
		CloseBreathCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Debuff           : %d"),
		DebuffCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Change    : %d"),
		TargetChangeCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Fly       : %d"),
		TargetChangeFlyCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Target Breath    : %d"),
		TargetChangeBreathCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic  : %d"),
		CenterMechanicCount);

	UE_LOG(LogTemp, Warning,
		TEXT("Total            : %d"),
		TotalPatternCount);

	///////////////////////////////////////////
}

void ADragonBoss::TargetChangePattern()
{
	/////////////////////////////////
	TargetChangeCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] TargetChange (%d / Total:%d)"),
		TargetChangeCount,
		TotalPatternCount);
	//////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("Target Change Pattern"));

	ChooseRandomTarget();

	UE_LOG(LogTemp, Warning,
		TEXT("TargetChange Distance=%f"),
		FVector::Dist(
			GetActorLocation(),
			TargetPlayer->GetActorLocation()));

	bIsAttacking = false;

	CurrentState = EDragonState::Walking;

	StartAttackCycle();
}

void ADragonBoss::CenterMechanicPattern()
{
	//////////////////////////////////////////
	CenterMechanicCount++;
	TotalPatternCount++;

	UE_LOG(LogTemp, Warning,
		TEXT("[Pattern] CenterMechanic (%d / Total:%d)"),
		CenterMechanicCount,
		TotalPatternCount);
	//////////////////////////////////////////
	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic"));

	bCenterMechanicActive = true;

	FlyToCenter();

	ChooseRandomTarget();
	if (!TargetPlayer)
	{
		FailCenterMechanic();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Center Target : %s"),
		*TargetPlayer->GetName());

	GetWorldTimerManager().SetTimer(
		CenterFailHandle,
		this,
		&ADragonBoss::FailCenterMechanic,
		8.f,
		false
	);
}

void ADragonBoss::FailCenterMechanic()
{
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	// Once the central Breath montage begins, its end callback owns the
	// transition back into the normal pattern loop.
	if (bMovementLockedForBreath)
	{
		return;
	}

	// The former timeout scheduled an attack but left bCenterMechanicActive
	// true, so Tick returned forever and the boss stopped attacking.
	bCenterMechanicActive = false;
	bCenterBreathStarted = false;
	bCenterTracking = false;
	bIsAttacking = false;
	bIsTelegraphing = false;
	bIsLeaping = false;
	bIsFlying = false;
	CurrentState = EDragonState::Walking;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetWorldTimerManager().ClearTimer(CenterFailHandle);
	ChooseRandomTarget();
	StartAttackCycle();
}

void ADragonBoss::OnCenterMechanicSuccess()
{
	if (!bCenterMechanicActive)
	{
		return;
	}

	float Damage = MaxHP * 0.1f;

	CurrentHP = FMath::Max(
		0.f,
		CurrentHP - Damage);

	CheckPhaseTwo();

	if (CurrentHP <= 0.f)
	{
		Die();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Center Mechanic Success"));

	UE_LOG(LogTemp, Warning,
		TEXT("Damage : %f"),
		Damage);

	UE_LOG(LogTemp, Warning,
		TEXT("Boss HP : %f"),
		CurrentHP);

	const bool bBreathMontageIsPlaying =
		bIsAttacking && CurrentState == EDragonState::Attacking;

	bCenterMechanicActive = false;
	bCenterTracking = false;
	bCenterBreathStarted = false;
	GetWorldTimerManager().ClearTimer(CenterFailHandle);

	// A reflected breath can resolve the mechanic before the animation ends.
	// Let its montage callback start the next pattern in that case.
	if (bBreathMontageIsPlaying)
	{
		return;
	}

	bIsAttacking = false;
	CurrentState = EDragonState::Walking;
	StartAttackCycle();
}

void ADragonBoss::OnAttackFinished()
{
	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	UpdatePlayerList();

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();
	}

	bIsAttacking = false;
	bIsTelegraphing = false;
	if (bCenterMechanicActive || bMovementLockedForBreath)
	{
		// Keep the central mechanic active until the breath montage ends so the
		// next pattern cannot interrupt its animation or projectile.
		bCenterMechanicActive = false;
		bCenterTracking = false;
		bCenterBreathStarted = false;
		bIsFlying = false;
		bMovementLockedForBreath = false;
		CurrentState = EDragonState::Walking;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetWorldTimerManager().ClearTimer(CenterFailHandle);
	}
	else if (!bIsFlying)
	{
		CurrentState = EDragonState::Walking;
	}

	if (!bFirstBreathDone)
	{
		bFirstBreathDone = true;
	}

	StartAttackCycle();
}

void ADragonBoss::Die()
{

	if (CurrentState == EDragonState::Dead)
	{
		return;
	}

	CurrentHP = 0.f;

	CurrentState = EDragonState::Dead;

	bIsFlying = false;
	bIsLeaping = false;
	bIsAttacking = false;

	bCenterTracking = false;
	bCenterMechanicActive = false;

	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	HeadHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MulticastPlayDeath();

	GetWorldTimerManager().ClearTimer(
		AttackTimerHandle
	);
	GetWorldTimerManager().ClearTimer(AttackEndHandle);
	GetWorldTimerManager().ClearTimer(ChargingEffectHandle);

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon Dead"));

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
	for (AActor* Actor : Players)
	{
		if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
		{
			if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
			{
				PS->NotifyBossDefeated();
				Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
			}
		}
	}

	if (AThe_Nightfall_SiegeGameMode* GameMode = GetWorld()->GetAuthGameMode<AThe_Nightfall_SiegeGameMode>())
	{
		GameMode->HandleBossDefeated();
	}

	GetWorldTimerManager().ClearTimer(
		TelegraphHandle);

	GetWorldTimerManager().ClearTimer(
		StunTimerHandle);

	GetWorldTimerManager().ClearTimer(
		CenterFailHandle);

	GetWorldTimerManager().ClearTimer(
		CenterTrackingHandle);

	// All remaining actor-bound callbacks (including recovery timers) must not
	// re-enter the attack cycle after death or while the level is unloading.
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

void ADragonBoss::StartAttackTelegraph(
	EDragonAttackType AttackType)
{
	if (!HasAuthority() || CurrentState == EDragonState::Dead || bStunned)
	{
		return;
	}

	if (!TargetPlayer || TargetPlayer->IsDead())
	{
		ChooseRandomTarget();

		if (!TargetPlayer)
		{
			bIsTelegraphing = false;
			bIsAttacking = false;
			CurrentState = EDragonState::Idle;
			return;
		}
	}

	AAIController* AIController =
		Cast<AAIController>(GetController());

	if (AIController)
	{
		AIController->StopMovement();
	}

	GetCharacterMovement()
		->StopMovementImmediately();

	bIsTelegraphing = true;
	bIsAttacking = true;

	if (TargetPlayer)
	{
		FVector Direction =
			TargetPlayer->GetActorLocation()
			- GetActorLocation();

		Direction.Z = 0.f;

		TelegraphRotation =
			Direction.Rotation();

		SetActorRotation(
			TelegraphRotation);
	}

	// ============================================================
	// Danger Zone 생성
	// ============================================================

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
	{
		FVector MouthLocation =
			GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

		FVector Forward =
			GetActorForwardVector();

		FVector SpawnLocation =
			MouthLocation + Forward * (BiteHitRange * 0.5f - BiteBackwardOffset);

		FRotator ZoneRotation =
			Forward.Rotation();

		ZoneRotation.Pitch = -90.f;

		ADangerZone* Zone =
			GetWorld()->SpawnActor<ADangerZone>(
				DangerZoneClass,
				SpawnLocation,
				ZoneRotation);

		if (Zone)
		{
			// Match BiteHit exactly: forward from the mouth and +/- BiteHalfWidth.
			Zone->LineLength = BiteHitRange;
			Zone->LineWidth = BiteHalfWidth;
			Zone->ZoneType = EDangerZoneType::Line;

			Zone->OnRep_ZoneType();

			// Bite는 공격 애니메이션을 즉시 시작하므로
			// 실제 BiteHit Notify 시점에 맞춰 조절
			constexpr float BiteWarningTime = 0.6f;

			Zone->LifeTime = BiteWarningTime;
			Zone->SetLifeSpan(BiteWarningTime);
		}

		break;
	}

	case EDragonAttackType::CloseBreath:
	{
		FVector SpawnLocation = GetCloseBreathCenter();

		FRotator ZoneRotation =
			TelegraphRotation;

		ZoneRotation.Pitch = -90.f;

		ADangerZone* Zone =
			GetWorld()->SpawnActor<ADangerZone>(
				DangerZoneClass,
				SpawnLocation,
				ZoneRotation);

		if (Zone)
		{
			// SetCloseBreathShape uses these exact radii for the warning decal.
			Zone->LineLength = CloseBreathHitRadius;
			Zone->LineWidth = CloseBreathHitRadius;
			Zone->ZoneType =
				EDangerZoneType::Cone;

			Zone->OnRep_ZoneType();

			// CloseBreath는 공격 애니메이션을 즉시 시작하므로
			// 실제 CloseBreathFire Notify 시점에 맞춰 조절
			constexpr float CloseBreathWarningTime = 0.8f;

			Zone->LifeTime = CloseBreathWarningTime;
			Zone->SetLifeSpan(CloseBreathWarningTime);
		}

		break;
	}

	case EDragonAttackType::Breath:
	{
	// Build the rectangle directly from the dragon to its chosen target.
	// Its centre is the midpoint and its long local axis is the target line.
	FVector LineStart = GetActorLocation();
	FVector LineEnd = TargetPlayer->GetActorLocation();
	LineEnd.Z = LineStart.Z;
	FVector LineDirection = LineEnd - LineStart;
	const float TargetDistance = LineDirection.Size2D();
	const float TelegraphLength = FMath::Clamp(
		TargetDistance + 500.f, 1000.f, BreathTelegraphRange);
	LineDirection = LineDirection.GetSafeNormal2D();
	FVector SpawnLocation = LineStart + LineDirection * (TelegraphLength * 0.5f);

	FRotator Rot = LineDirection.Rotation();
	Rot.Pitch = -90.f;

		ADangerZone* Zone =
			GetWorld()->SpawnActor<ADangerZone>(
				DangerZoneClass,
				SpawnLocation,
				Rot);

		if (Zone)
		{
			Zone->LineLength = TelegraphLength;
			Zone->ZoneType = EDangerZoneType::Line;
			Zone->OnRep_ZoneType();

			// 원거리 Breath는 기존처럼
			// 범위 표시 후 3초 뒤 공격 시작
			constexpr float BreathWarningTime = 3.0f;

			Zone->LifeTime = BreathWarningTime;
			Zone->SetLifeSpan(BreathWarningTime);

			// 중앙 기믹의 Breath일 경우
			// Tick에서 범위를 보스와 타겟 방향에 맞춰 움직이도록 저장
			if (bCenterMechanicActive)
			{
				CurrentBreathZone = Zone;
			}
		}

		break;
	}

	case EDragonAttackType::Debuff:
	{
		FVector SpawnLocation =
			ArenaCenter;

		SpawnLocation.Z += 5.f;

		FRotator Rotation(
			-90.f,
			0.f,
			0.f);

		ADangerZone* Zone =
			GetWorld()->SpawnActor<ADangerZone>(
				DangerZoneClass,
				SpawnLocation,
				Rotation);

		if (Zone)
		{
			Zone->ZoneType =
				EDangerZoneType::FullMap;

			Zone->OnRep_ZoneType();

			// Debuff는 기존처럼
			// 범위 표시 후 3초 뒤 실행
			constexpr float DebuffWarningTime = 2.0f;

			Zone->LifeTime = DebuffWarningTime;
			Zone->SetLifeSpan(DebuffWarningTime);
		}

		// 이펙트는 기존대로 텔레그래프 시작 시 재생
		MulticastPlayAttack(EDragonAttackType::Debuff);

		// 차징 이펙트는 대기 시작 후 1초 뒤 재생
		GetWorldTimerManager().SetTimer(
			ChargingEffectHandle,
			this,
			&ADragonBoss::StartBlackoutChargingFX,
			1.0f,
			false);

		break;
	}
	}

	// ============================================================
	// 공격 실행
	// Bite / CloseBreath : 즉시 실행
	// Breath / Debuff    : 3초 후 실행
	// ============================================================

	if (AttackType == EDragonAttackType::Bite ||
		AttackType == EDragonAttackType::CloseBreath)
	{
		// 기존에 남아 있을 수 있는 타이머 제거
		GetWorldTimerManager().ClearTimer(
			TelegraphHandle);

		// DangerZone 생성 직후 바로 공격 애니메이션 시작
		ExecuteTelegraphedAttack(
			AttackType);

		return;
	}

	// Breath / Debuff는 기존처럼 3초 대기
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &ADragonBoss::ExecuteTelegraphedAttack, AttackType);

	GetWorldTimerManager().SetTimer(
		TelegraphHandle,
		Delegate,
		2.f,
		false);
}

void ADragonBoss::ExecuteTelegraphedAttack(
	EDragonAttackType AttackType)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsTelegraphing = false;

	switch (AttackType)
	{
	case EDragonAttackType::Bite:
		BiteAttack();
		break;

	case EDragonAttackType::CloseBreath:
		CloseBreathAttack();
		break;

	case EDragonAttackType::Breath:
		// Lock aim as the montage begins; the remaining decal is only the
		// tail end of its visual lifetime and must no longer track the target.
		GetWorldTimerManager().ClearTimer(CenterTrackingHandle);
		StopBreathTracking();
		BreathAttack();
		break;

	case EDragonAttackType::Debuff:
		DebuffAttack();
		break;
	}
}

void ADragonBoss::OnLeapFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Error,
		TEXT("===== LEAP FINISHED ====="));

	bIsLeaping = false;
	GetWorldTimerManager().ClearTimer(LeapRecoveryHandle);
	bIsFlying = true;

	UE_LOG(LogTemp, Warning, TEXT("bIsFlying = %d"), bIsFlying);

	CurrentState = EDragonState::Flying;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Flying);

	UE_LOG(LogTemp, Error,
		TEXT("Target = %s"),
		TargetPlayer ? *TargetPlayer->GetName() : TEXT("None"));

	if (TargetPlayer)
	{
		FlyToTarget();
	}
}

void ADragonBoss::OnLandFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsFlying = false;
	GetWorldTimerManager().ClearTimer(LandingRecoveryHandle);

	CurrentState = EDragonState::Walking;

	GetCharacterMovement()->SetMovementMode(
		MOVE_Walking);

	UE_LOG(LogTemp, Error,
		TEXT("===== LAND FINISHED ====="));

	StartAttackCycle();
}

void ADragonBoss::BiteHit()
{
	if (!HasAuthority())
	{
		return;
	}

	float Damage =
		AttackPower * 1.0f * GetCurrentDamageMultiplier();

	FVector MouthLocation =
		GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

	FVector Forward =
		GetActorForwardVector();

	FVector Right =
		GetActorRightVector();

	MulticastSpawnBiteFX(MouthLocation);

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

		FVector ToPlayer =
			Player->GetActorLocation() - MouthLocation;

		// 보스 정면/옆 방향으로 거리 계산
		float ForwardDistance =
			FVector::DotProduct(ToPlayer, Forward);

		float RightDistance =
			FVector::DotProduct(ToPlayer, Right);

		// 직사각형 범위
		// 길이 600 → MouthSocket 기준 앞쪽 -150 ~ +450
		// 폭 300 → 좌우 -150 ~ +150
		if (ForwardDistance >= -BiteBackwardOffset &&
			ForwardDistance <= BiteHitRange - BiteBackwardOffset &&
			FMath::Abs(RightDistance) <= BiteHalfWidth)
		{
			Player->TakePlayerDamage(Damage);

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Bite Hit : %s"),
				*Player->GetName());
		}
	}
}

FVector ADragonBoss::GetCloseBreathCenter() const
{
	FVector Center = GetMesh()->GetSocketLocation(TEXT("MouthSocket"))
		+ GetActorForwardVector() * CloseBreathForwardOffset;
	Center.Z = GetActorLocation().Z;
	return Center;
}

void ADragonBoss::CloseBreathFire()
{
	if (!HasAuthority())
	{
		return;
	}

	float Damage = AttackPower * 2.0f * GetCurrentDamageMultiplier();

	const FVector BreathCenter = GetCloseBreathCenter();

	MulticastSpawnCloseBreathFX(BreathCenter);

	TArray<AActor*> Players;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);

	for (AActor* Actor : Players)
	{
		ABaseCharacter* Player = Cast<ABaseCharacter>(Actor);

		if (!Player)
		{
			continue;
		}

		float Distance = FVector::Dist(Player->GetActorLocation(), BreathCenter);

		if (Distance <= CloseBreathHitRadius)
		{
			Player->TakePlayerDamage(Damage);

			UE_LOG(LogTemp, Warning, TEXT("Close Breath Hit : %s"), *Player->GetName());
		}
	}
}

void ADragonBoss::BreathFire()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!BreathProjectileClass)
	{
		return;
	}

	FVector MouthLocation = GetMesh()->GetSocketLocation(TEXT("MouthSocket"));

	FRotator SpawnRotation = GetActorRotation();

	ADragonBreathProjectile* Projectile =
		GetWorld()->SpawnActor<ADragonBreathProjectile>(BreathProjectileClass, MouthLocation, SpawnRotation);

	if (Projectile)
	{
		Projectile->DamageMultiplier = GetCurrentDamageMultiplier();

		Projectile->LaunchInDirection(GetActorForwardVector());
	}
}

void ADragonBoss::MulticastShowDamage_Implementation(float Damage)
{
	ShowDamage(Damage);
}

void ADragonBoss::MulticastPlayAttack_Implementation(EDragonAttackType AttackType)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance)
	{
		return;
	}

	switch (AttackType)
	{
	case EDragonAttackType::Bite:

		if (BiteMontage)
		{
			AnimInstance->Montage_Play(BiteMontage);
		}

		break;

	case EDragonAttackType::CloseBreath:

		if (CloseBreathMontage)
		{
			AnimInstance->Montage_Play(CloseBreathMontage);
		}

		break;

	case EDragonAttackType::Breath:

		if (BreathMontage)
		{
			AnimInstance->Montage_Play(BreathMontage);
		}

		break;

	case EDragonAttackType::Debuff:

		if (DebuffMontage)
		{
			AnimInstance->Montage_Play(DebuffMontage);
		}

		break;

	default:
		break;
	}
}

void ADragonBoss::MulticastPlayMovementTransition_Implementation(bool bLanding)
{
	UAnimMontage* Montage = bLanding ? LandMontage : LeapMontage;

	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

void ADragonBoss::MulticastSpawnBiteFX_Implementation(
	FVector Location)
{
	if (!BiteFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		BiteFX,
		Location,
		FRotator::ZeroRotator,
		FVector(3.f));
}

void ADragonBoss::MulticastSpawnCloseBreathFX_Implementation(
	FVector Location)
{
	if (!CloseBreathFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		CloseBreathFX,
		Location);
}

void ADragonBoss::MulticastSpawnBlackoutChargingFX_Implementation()
{
	if (!BlackoutChargingFX || !GetMesh())
	{
		return;
	}

	BlackoutChargingFXComponent =
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			BlackoutChargingFX,
			GetMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
}

void ADragonBoss::MulticastSpawnBlackoutReleaseFX_Implementation(FVector Location)
{
	if (!BlackoutReleaseFX)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		BlackoutReleaseFX,
		Location);
}

void ADragonBoss::MulticastStartPhaseTwoFX_Implementation()
{
	EnsurePhaseTwoFX();
}

void ADragonBoss::MulticastStartPhaseTwoTransition_Implementation()
{
	EnsurePhaseTwoFX();

	float MontageDuration = 0.f;
	if (DebuffMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			MontageDuration = AnimInstance->Montage_Play(DebuffMontage);
		}
	}

	// If the montage is unavailable, apply the material immediately rather
	// than leaving this client permanently in the phase-one appearance.
	const float MaterialDelay = FMath::Max(0.f, MontageDuration - 0.5f);
	if (MaterialDelay <= 0.f)
	{
		ApplyPhaseTwoMaterial();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PhaseTwoMaterialHandle,
		this,
		&ADragonBoss::ApplyPhaseTwoMaterial,
		MaterialDelay,
		false);
}


bool ADragonBoss::TakeArcherQVolleyDamage(ABaseCharacter* Attacker, int32 VolleyId, float Damage)
{
	if (!Attacker || VolleyId == INDEX_NONE || !HasAuthority())
	{
		return false;
	}

	int32& LastVolleyId = LastArcherQVolleyByAttacker.FindOrAdd(Attacker, INDEX_NONE);
	if (LastVolleyId == VolleyId)
	{
		return false;
	}

	LastVolleyId = VolleyId;
	TakeBossDamage(Damage);
	return true;
}

void ADragonBoss::EnsurePhaseTwoFX()
{
	if (!PhaseTwoFX || !GetMesh() || IsValid(PhaseTwoFXComponent))
	{
		return;
	}

	// The phase effect remains attached to the mesh for the rest of the fight.
	PhaseTwoFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		PhaseTwoFX,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false);
}

void ADragonBoss::OnRep_Shielded()
{
	if (bShielded)
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

void ADragonBoss::EnsureBarrierFX()
{
	if (!BarrierFX)
	{
		BarrierFX = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Game/Effects/6_Dragon/Barrier/NS_Dragon_Barrier.NS_Dragon_Barrier"));
	}

	if (!BarrierFX || !GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("Dragon barrier Niagara asset could not be loaded"));
		return;
	}

	if (IsValid(BarrierFXComponent))
	{
		BarrierFXComponent->SetAsset(BarrierFX);
		BarrierFXComponent->SetAbsolute(false, false, true);
		BarrierFXComponent->SetWorldScale3D(FVector::OneVector);
		BarrierFXComponent->SetVisibility(true, true);
		BarrierFXComponent->SetHiddenInGame(false, true);
		BarrierFXComponent->Activate(true);
		return;
	}

	// The Niagara system intentionally does not auto-kill its finished particles,
	// so this component stays attached until the shield is explicitly broken.
	BarrierFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		BarrierFX,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false);

	if (BarrierFXComponent)
	{
		BarrierFXComponent->SetAbsolute(false, false, true);
		BarrierFXComponent->SetWorldScale3D(FVector::OneVector);
		BarrierFXComponent->SetVisibility(true, true);
		BarrierFXComponent->SetHiddenInGame(false, true);
		BarrierFXComponent->Activate(true);
	}
}

void ADragonBoss::ApplyPhaseTwoMaterial()
{
	if (GetMesh() && PhaseTwoOverlayMaterial)
	{
		GetMesh()->SetOverlayMaterial(PhaseTwoOverlayMaterial);
	}
}

float ADragonBoss::GetCurrentDamageMultiplier() const
{
	return bIsPhaseTwo ? PhaseTwoDamageMultiplier : 1.f;
}

void ADragonBoss::CheckPhaseTwo()
{
	if (bIsPhaseTwo || MaxHP <= 0.f || CurrentHP > MaxHP * PhaseTwoHealthThreshold)
	{
		return;
	}

	bIsPhaseTwo = true;
	bIsAttacking = true;
	bIsTelegraphing = false;
	CurrentState = EDragonState::Attacking;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();

	// A phase transition replaces any attack already in progress.  Otherwise a
	// previous telegraph or attack-end callback can interrupt its montage.
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEndHandle);
	GetWorldTimerManager().ClearTimer(ChargingEffectHandle);
	GetWorldTimerManager().ClearTimer(TelegraphHandle);

	ForceNetUpdate();
	MulticastStartPhaseTwoTransition();

	float MontageDuration = 0.f;
	if (DebuffMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			MontageDuration = DebuffMontage->GetPlayLength() /
				FMath::Max(AnimInstance->Montage_GetPlayRate(DebuffMontage), KINDA_SMALL_NUMBER);
		}
	}

	const float MaterialDelay = FMath::Max(0.f, MontageDuration - 0.5f);
	GetWorldTimerManager().SetTimer(
		PhaseTwoMaterialReplicationHandle,
		this,
		&ADragonBoss::MarkPhaseTwoMaterialApplied,
		MaterialDelay,
		false);
	GetWorldTimerManager().SetTimer(
		PhaseTransitionEndHandle,
		this,
		&ADragonBoss::FinishPhaseTwoTransition,
		MontageDuration,
		false);

	UE_LOG(LogTemp, Warning,
		TEXT("Dragon entered phase two. Damage multiplier: %.2f"),
		PhaseTwoDamageMultiplier);
}

void ADragonBoss::OnRep_IsPhaseTwo()
{
	if (bIsPhaseTwo)
	{
		EnsurePhaseTwoFX();
	}
}

void ADragonBoss::OnRep_PhaseTwoMaterialApplied()
{
	if (bPhaseTwoMaterialApplied)
	{
		ApplyPhaseTwoMaterial();
	}
}

void ADragonBoss::MarkPhaseTwoMaterialApplied()
{
	if (!HasAuthority() || bPhaseTwoMaterialApplied)
	{
		return;
	}

	bPhaseTwoMaterialApplied = true;
	ForceNetUpdate();
}

void ADragonBoss::FinishPhaseTwoTransition()
{
	if (!HasAuthority() || CurrentState == EDragonState::Dead)
	{
		return;
	}

	bIsAttacking = false;
	CurrentState = EDragonState::Walking;
	StartAttackCycle();
}

void ADragonBoss::OnRep_CurrentState()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Dragon State Changed : %d"),
		(int32)CurrentState);
}

void ADragonBoss::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADragonBoss, CurrentState);
	DOREPLIFETIME(ADragonBoss, CurrentHP);
	DOREPLIFETIME(ADragonBoss, bIsFlying);
	DOREPLIFETIME(ADragonBoss, bIsPhaseTwo);
	DOREPLIFETIME(ADragonBoss, bPhaseTwoMaterialApplied);
	DOREPLIFETIME(ADragonBoss, bShielded);
}

void ADragonBoss::OnRep_CurrentHP()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Boss HP : %.1f"),
		CurrentHP);
}

void ADragonBoss::DebugBite()
{
	StartAttackTelegraph(EDragonAttackType::Bite);
}

void ADragonBoss::DebugCloseBreath()
{
	StartAttackTelegraph(EDragonAttackType::CloseBreath);
}

void ADragonBoss::DebugBreath()
{
	StartAttackTelegraph(EDragonAttackType::Breath);
}

void ADragonBoss::DebugDebuff()
{
	StartAttackTelegraph(EDragonAttackType::Debuff);
}

void ADragonBoss::DebugCenterMechanic()
{
	if (!bCenterMechanicActive && CurrentState != EDragonState::Dead)
	{
		CenterMechanicPattern();
	}
}

void ADragonBoss::MulticastPlayDeath_Implementation()
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

void ADragonBoss::MulticastStopBlackoutChargingFX_Implementation()
{
	if (IsValid(BlackoutChargingFXComponent))
	{
		BlackoutChargingFXComponent->Deactivate();
		BlackoutChargingFXComponent->DestroyComponent();
		BlackoutChargingFXComponent = nullptr;
	}
}
