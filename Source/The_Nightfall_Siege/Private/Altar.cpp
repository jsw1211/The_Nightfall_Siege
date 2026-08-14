// Fill out your copyright notice in the Description page of Project Settings.


#include "Altar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Net/UnrealNetwork.h"
#include "BasePlayerState.h"

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

	AltarLight->SetIntensity(5000.f);

	AltarLight->SetAttenuationRadius(1200.f);

	AltarLight->SetVisibility(false);

	UE_LOG(LogTemp, Warning, TEXT("Bind BeginOverlap"));
}

// Called when the game starts or when spawned
void AAltar::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("Altar BeginPlay"));

	InteractionBox->OnComponentBeginOverlap.AddDynamic(
		this,
		&AAltar::OnOverlapBegin);

	InteractionBox->OnComponentEndOverlap.AddDynamic(
		this,
		&AAltar::OnOverlapEnd);
}

// Called every frame
void AAltar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	Player->Slot1Icon = Player->EmptySlotIcon;

	if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
	{
		PS->bHasLantern = false;
		PS->bLanternEquipped = false;
	}

	Player->RefreshLanternState();

	LanternMesh->SetVisibility(true);

	AltarLight->SetVisibility(true);

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

	Player->Slot1Icon = Player->LanternIcon;

	if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
	{
		PS->bHasLantern = true;
		PS->bLanternEquipped = true;
	}

	Player->RefreshLanternState();
	Player->ResumeLanternGuidanceAfterAltar();

	LanternMesh->SetVisibility(false);

	AltarLight->SetVisibility(false);

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
	AltarLight->SetVisibility(bActivated);
}

void AAltar::OnRep_LanternPlaced()
{
	UE_LOG(LogTemp, Warning,
		TEXT("OnRep_LanternPlaced : %d"),

		bLanternPlaced);
	LanternMesh->SetVisibility(bLanternPlaced);

	AltarLight->SetVisibility(bLanternPlaced);
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
}

