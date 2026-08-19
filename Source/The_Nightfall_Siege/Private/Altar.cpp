// Fill out your copyright notice in the Description page of Project Settings.


#include "Altar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/WidgetComponent.h"
#include "AltarInteractionWidget.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "BasePlayerState.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AAltar::AAltar()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 메쉬 생성
	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));

	RootComponent = AltarMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));

	InteractionBox->SetupAttachment(RootComponent);

	InteractionBox->SetBoxExtent(FVector(300.f));

	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);

	MonsterExclusionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("MonsterExclusionCapsule"));
	MonsterExclusionCapsule->SetupAttachment(RootComponent);
	// Extend from the altar base well above the platform, so pushed monsters
	// cannot settle on its top surface.
	MonsterExclusionCapsule->SetRelativeLocation(FVector(0.f, 0.f, 600.f));
	MonsterExclusionCapsule->SetCapsuleSize(210.f, 700.f);
	MonsterExclusionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MonsterExclusionCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	MonsterExclusionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	MonsterExclusionCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MonsterExclusionCapsule->SetGenerateOverlapEvents(true);

	bActivated = false;
	bPlayerInside = false;

	LightRange = CreateDefaultSubobject<USphereComponent>(TEXT("LightRange"));

	LightRange->SetupAttachment(RootComponent);

	LightRange->SetSphereRadius(1200.f);

	LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternMesh"));

	LanternMesh->SetupAttachment(RootComponent);

	LanternMesh->SetVisibility(false);

	AltarLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AltarLight"));

	AltarLight->SetupAttachment(LanternMesh);
	AltarLight->SetMobility(EComponentMobility::Movable);

	AltarLight->SetIntensity(18000.f);

	// Monsters can only be damaged inside LightRange. Match the visual light
	// to that same radius so the illuminated area communicates the rule.
	AltarLight->SetAttenuationRadius(1200.f);

	AltarLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.0f));

	AltarLight->SetVisibility(false);

	AltarSafeZoneDecal = CreateDefaultSubobject<UDecalComponent>(
		TEXT("AltarSafeZoneDecal"));
	AltarSafeZoneDecal->SetupAttachment(RootComponent);
	AltarSafeZoneDecal->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
	AltarSafeZoneDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	AltarSafeZoneDecal->DecalSize = FVector(200.f, 1200.f, 1200.f);
	AltarSafeZoneDecal->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SanctuaryMaterial(
		TEXT("/Game/Effects/Lantern/M_Lantern_Sanctuary.M_Lantern_Sanctuary"));
	if (SanctuaryMaterial.Succeeded())
	{
		AltarSafeZoneDecal->SetDecalMaterial(SanctuaryMaterial.Object);
	}

	InteractionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	InteractionPrompt->SetupAttachment(RootComponent);
	InteractionPrompt->SetRelativeLocation(FVector(0.f, 0.f, 210.f));
	InteractionPrompt->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPrompt->SetDrawSize(FVector2D(280.f, 42.f));
	InteractionPrompt->SetPivot(FVector2D(0.5f, 0.5f));
	InteractionPrompt->SetWidgetClass(UAltarInteractionWidget::StaticClass());
	InteractionPrompt->SetVisibility(false);

	UE_LOG(LogTemp, Warning, TEXT("Bind BeginOverlap"));
}

// Called when the game starts or when spawned
void AAltar::BeginPlay()
{
	Super::BeginPlay();

	// Keep this visual boundary identical to the range used by Monster.cpp
	// when deciding whether an altar-owned monster can take damage.
	AltarLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.0f));
	AltarLight->SetAttenuationRadius(LightRange->GetScaledSphereRadius());
	const float AltarSafeRadius = LightRange->GetScaledSphereRadius();
	AltarSafeZoneDecal->DecalSize = FVector(
		200.f, AltarSafeRadius, AltarSafeRadius);
	
	UE_LOG(LogTemp, Warning, TEXT("Altar BeginPlay"));

	InteractionBox->OnComponentBeginOverlap.AddDynamic(
		this,
		&AAltar::OnOverlapBegin);

	InteractionBox->OnComponentEndOverlap.AddDynamic(
		this,
		&AAltar::OnOverlapEnd);

	// Screen-space widgets are rendered as billboards and always face the
	// local player's camera, regardless of the altar actor's rotation.
	InteractionPrompt->SetWidgetSpace(EWidgetSpace::Screen);
	// InitWidget has completed by BeginPlay. Calling SetWindowVisibility in the
	// constructor would run before the UUserWidget exists and fails during cook.
	InteractionPrompt->SetWindowVisibility(EWindowVisibility::SelfHitTestInvisible);

	// Child Blueprints may have serialized the component as visible.
	InteractionPrompt->SetVisibility(false, true);
}

// Called every frame
void AAltar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		PushMonstersOffAltar();
	}
}

void AAltar::PushMonstersOffAltar()
{
	if (!MonsterExclusionCapsule)
	{
		return;
	}

	TArray<AActor*> OverlappingMonsters;
	MonsterExclusionCapsule->GetOverlappingActors(
		OverlappingMonsters,
		AMonster::StaticClass());

	const FVector AltarLocation = GetActorLocation();
	const float ExclusionRadius = MonsterExclusionCapsule->GetScaledCapsuleRadius();

	for (AActor* OverlappingActor : OverlappingMonsters)
	{
		AMonster* Monster = Cast<AMonster>(OverlappingActor);
		if (!Monster || Monster->IsActorBeingDestroyed())
		{
			continue;
		}

		FVector AwayFromAltar = Monster->GetActorLocation() - AltarLocation;
		AwayFromAltar.Z = 0.f;
		if (AwayFromAltar.IsNearlyZero())
		{
			AwayFromAltar = FVector::ForwardVector;
		}
		else
		{
			AwayFromAltar.Normalize();
		}

		const float EjectDistance = ExclusionRadius +
			Monster->GetCapsuleComponent()->GetScaledCapsuleRadius() + 50.f;
		FVector SafeLocation = AltarLocation + AwayFromAltar * EjectDistance;
		SafeLocation.Z = Monster->GetActorLocation().Z;
		Monster->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AAltar::ActivateAltar()
{
	bActivated = true;

	UE_LOG(LogTemp, Warning, TEXT("Altar Activated"));
}

void AAltar::RegisterMonster(AMonster* Monster)
{
	if (Monster)
	{
		OwnedMonsters.Add(Monster);
		++RemainingMonsterCount;
	}
}

bool AAltar::IsInsideActiveLightZone(const FVector& WorldLocation) const
{
	// The green ground decal is visible while the lantern is placed. Use that
	// exact state for all safe-zone checks, including player darkness damage.
	if (!bLanternPlaced || !LightRange)
	{
		return false;
	}

	FVector ToLocation = WorldLocation - GetActorLocation();
	ToLocation.Z = 0.f;
	const float Range = LightRange->GetScaledSphereRadius();
	return ToLocation.SizeSquared() <= FMath::Square(Range);
}

bool AAltar::NotifyOwnedMonsterDefeated()
{
	if (!HasAuthority() || bCleared)
	{
		return bCleared;
	}

	RemainingMonsterCount = FMath::Max(0, RemainingMonsterCount - 1);
	if (RemainingMonsterCount == 0)
	{
		bCleared = true;
		ForceNetUpdate();
		UE_LOG(LogTemp, Warning, TEXT("Altar cleared: %s"), *GetName());
	}

	return bCleared;
}

void AAltar::PlaceLantern(ABaseCharacter* Player)
{
	bLanternPlaced = true;
	bActivated = true;

	Player->bHasLantern = false;
	Player->bLanternEquipped = false;
	Player->bLanternPoseActive = false;

	if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
	{
		PS->bHasLantern = false;
		PS->bLanternEquipped = false;
		PS->ForceNetUpdate();
	}

	Player->OnRep_HasLantern();
	Player->RefreshLanternState();
	Player->ForceNetUpdate();

	LanternMesh->SetVisibility(true);

	AltarLight->SetVisibility(true, true);
	AltarSafeZoneDecal->SetVisibility(true, true);
	ForceNetUpdate();

	UE_LOG(LogTemp, Warning, TEXT("Lantern Placed"));
}

void AAltar::RemoveLantern(ABaseCharacter* Player)
{
	if (!bCleared)
	{
		return;
	}

	bLanternPlaced = false;
	bActivated = false;

	Player->bHasLantern = true;
	Player->bLanternEquipped = true;
	Player->bLanternPoseActive = true;

	if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
	{
		PS->bHasLantern = true;
		PS->bLanternEquipped = true;
		PS->ForceNetUpdate();
	}

	Player->OnRep_HasLantern();
	Player->RefreshLanternState();
	Player->ResumeLanternGuidanceAfterAltar();
	Player->ForceNetUpdate();

	LanternMesh->SetVisibility(false);

	AltarLight->SetVisibility(false, true);
	AltarSafeZoneDecal->SetVisibility(false, true);
	ForceNetUpdate();

	UE_LOG(LogTemp, Warning, TEXT("Lantern Removed"));
}

void AAltar::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAltar, bActivated);
	DOREPLIFETIME(AAltar, bLanternPlaced);
	DOREPLIFETIME(AAltar, bCleared);
	DOREPLIFETIME(AAltar, RemainingMonsterCount);
}

void AAltar::OnRep_Activated()
{
	AltarLight->SetVisibility(bLanternPlaced, true);
	AltarSafeZoneDecal->SetVisibility(bLanternPlaced, true);
}

void AAltar::OnRep_LanternPlaced()
{
	UE_LOG(LogTemp, Warning,
		TEXT("OnRep_LanternPlaced : %d"),

		bLanternPlaced);
	LanternMesh->SetVisibility(bLanternPlaced);

	AltarLight->SetVisibility(bLanternPlaced, true);
	AltarSafeZoneDecal->SetVisibility(bLanternPlaced, true);
}

void AAltar::OnRep_Cleared()
{
	UE_LOG(LogTemp, Warning, TEXT("Altar clear replicated: %d"), bCleared);
}

void AAltar::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Altar BeginOverlap"));

	ABaseCharacter* Player =
		Cast<ABaseCharacter>(OtherActor);

	if (!Player)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Player Found"));
	Player->SetNearbyAltar(this);

	if (Player->IsLocallyControlled())
	{
		InteractionPrompt->SetVisibility(true, true);
	}
}

void AAltar::OnOverlapEnd(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ABaseCharacter* Player =
		Cast<ABaseCharacter>(OtherActor);

	if (!Player)
	{
		return;
	}

	Player->SetNearbyAltar(nullptr);

	if (Player->IsLocallyControlled())
	{
		InteractionPrompt->SetVisibility(false, true);
	}
}

