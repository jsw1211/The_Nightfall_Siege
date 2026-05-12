// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonPortal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RaidGameInstance.h"

// Sets default values
ADungeonPortal::ADungeonPortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PortalMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));

	RootComponent = PortalMesh;

	CollisionBox =
		CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));

	CollisionBox->SetupAttachment(RootComponent);

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

		if (PC && PC->WasInputKeyJustPressed(EKeys::E))
		{
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

	GEngine->AddOnScreenDebugMessage(
		-1,
		2.f,
		FColor::Green,
		TEXT("Press E to Enter Dungeon"));
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
	URaidGameInstance* GI =
		Cast<URaidGameInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	UGameplayStatics::OpenLevel(
		this,
		GI->CurrentDungeon);
}

