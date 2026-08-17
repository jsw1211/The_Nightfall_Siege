// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonPortal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TheNightfallSiegeInstance.h"
#include "BaseCharacter.h"
#include "BasePlayerState.h"

// Sets default values
ADungeonPortal::ADungeonPortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	RootComponent = SceneRoot;

	PortalMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));

	PortalMesh->SetupAttachment(SceneRoot);

	CollisionBox =
		CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));

	CollisionBox->SetupAttachment(SceneRoot);

	CollisionBox->SetBoxExtent(FVector(150.f));

	CollisionBox->OnComponentBeginOverlap.AddDynamic(
		this,
		&ADungeonPortal::OnOverlapBegin);

	CollisionBox->OnComponentEndOverlap.AddDynamic(
		this,
		&ADungeonPortal::OnOverlapEnd);

	bPlayerInside = false;

	PortalMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f));
	SetActorRotation(FRotator(0.f, 90.f, 0.f));
}

// Called when the game starts or when spawned
void ADungeonPortal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADungeonPortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADungeonPortal::OnOverlapBegin(
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

	Player->SetNearbyDungeonPortal(this);
}

void ADungeonPortal::OnOverlapEnd(
	UPrimitiveComponent* OverlappedComp,
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

	Player->SetNearbyDungeonPortal(nullptr);
}

void ADungeonPortal::ServerEnterDungeon_Implementation()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ServerEnterDungeon"));


	if (!HasAuthority())
	{
		return;
	}

	UTheNightfallSiegeInstance* GI =
		Cast<UTheNightfallSiegeInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	// A failed dungeon attempt must not keep coins collected inside it.  Store
	// a separate checkpoint per player because each player can have a
	// different balance in multiplayer.
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Players);
	for (AActor* Actor : Players)
	{
		if (ABaseCharacter* Player = Cast<ABaseCharacter>(Actor))
		{
			if (ABasePlayerState* PlayerState = Player->GetPlayerState<ABasePlayerState>())
			{
				PlayerState->Coin = Player->Coin;
				PlayerState->DungeonEntryCoin = Player->Coin;
				PlayerState->bHasDungeonCoinCheckpoint = true;
				PlayerState->ForceNetUpdate();
			}
		}
	}

	FString MapPath =
		FString::Printf(
			TEXT("/Game/Level/%s?listen"),
			*GI->CurrentDungeon.ToString());

	UE_LOG(LogTemp, Warning,
		TEXT("ServerTravel : %s"),
		*MapPath);

	GetWorld()->ServerTravel(MapPath);
}

