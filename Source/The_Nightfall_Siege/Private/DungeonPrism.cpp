// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonPrism.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TheNightfallSiegeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Portal.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FVector FindReturnPortalGroundLocation(
		const UWorld* World,
		const AActor* ActorToIgnore,
		const FVector& DesiredLocation)
	{
		if (!World)
		{
			return DesiredLocation;
		}

		FHitResult GroundHit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PortalGroundTrace), false);
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
ADungeonPrism::ADungeonPrism()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	PrismMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrismMesh"));

	RootComponent = PrismMesh;

	SphereCollision =
		CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));

	SphereCollision->SetupAttachment(RootComponent);

	SphereCollision->SetSphereRadius(150.f);

	SphereCollision->OnComponentBeginOverlap.AddDynamic(
		this,
		&ADungeonPrism::OnOverlapBegin);

	SphereCollision->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);

	SphereCollision->SetCollisionResponseToAllChannels(
		ECR_Overlap);

	PrismDropEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PrismDropEffect"));
	PrismDropEffect->SetupAttachment(RootComponent);
	PrismDropEffect->SetAutoActivate(true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PrismDropEffectAsset(
		TEXT("/Game/Effects/dropped_item/NS_Prism.NS_Prism"));
	if (PrismDropEffectAsset.Succeeded())
	{
		PrismDropEffect->SetAsset(PrismDropEffectAsset.Object);
	}

	bActivated = false;
}

// Called when the game starts or when spawned
void ADungeonPrism::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADungeonPrism::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADungeonPrism::ActivatePrism()
{
	bActivated = true;

	SetActorHiddenInGame(false);

	SetActorEnableCollision(true);

	GEngine->AddOnScreenDebugMessage(
		-1,
		3.f,
		FColor::Cyan,
		TEXT("Prism Activated"));
}

void ADungeonPrism::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ABaseCharacter* Player =
		Cast<ABaseCharacter>(OtherActor);

	if (!Player)
	{
		return;
	}

	Player->SetNearbyPrism(this);

	UE_LOG(LogTemp, Warning,
		TEXT("Prism Nearby"));
}

void ADungeonPrism::RemoveDarknessDebuff()
{
	TArray<AActor*> Players;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ABaseCharacter::StaticClass(),
		Players);

	for (AActor* Actor : Players)
	{
		ABaseCharacter* Player =
			Cast<ABaseCharacter>(Actor);

		if (Player)
		{
			Player->bDarknessDebuff = false;
		}
	}

	ActivatedPlayers.Empty();

	UE_LOG(LogTemp, Warning,
		TEXT("Darkness Removed"));
}

void ADungeonPrism::SpawnReturnPortal()
{
	if (!ReturnPortalClass)
	{
		return;
	}

	APortal* Portal =
		GetWorld()->SpawnActor<APortal>(
			ReturnPortalClass,
			FindReturnPortalGroundLocation(GetWorld(), this, GetActorLocation()),
			FRotator::ZeroRotator);

	if (Portal)
	{
		Portal->PortalType = EPortalType::ReturnVillage;
	}
}

