// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonPortal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TheNightfallSiegeInstance.h"

// Sets default values
ADungeonPortal::ADungeonPortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	if (bPlayerInside)
	{
		APlayerController* PC =
			GetWorld()->GetFirstPlayerController();

		if (PC && PC->WasInputKeyJustPressed(EKeys::F))
		{
			UE_LOG(LogTemp, Warning, TEXT("F Pressed"));

			EnterDungeon();
		}
	}
}

void ADungeonPortal::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	bPlayerInside = true;

}

void ADungeonPortal::OnOverlapEnd(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	bPlayerInside = false;
}

void ADungeonPortal::EnterDungeon()
{
	UTheNightfallSiegeInstance* GI =
		Cast<UTheNightfallSiegeInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Open Dungeon: %s"),
		*GI->CurrentDungeon.ToString()); // 디버그용


	UGameplayStatics::OpenLevel(
		this,
		GI->CurrentDungeon);
}

