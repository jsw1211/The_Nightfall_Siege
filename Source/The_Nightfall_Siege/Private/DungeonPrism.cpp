// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonPrism.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "RaidGameInstance.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
ADungeonPrism::ADungeonPrism()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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

	bActivated = false;
}

// Called when the game starts or when spawned
void ADungeonPrism::BeginPlay()
{
	Super::BeginPlay();
	
	// Ã³À½¿£ ¼û±è
	SetActorHiddenInGame(true);

	SetActorEnableCollision(false);
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
	if (!bActivated)
	{
		return;
	}

	ABaseCharacter* Player =
		Cast<ABaseCharacter>(OtherActor);

	if (!Player)
	{
		return;
	}

	URaidGameInstance* GI =
		Cast<URaidGameInstance>(GetGameInstance());

	if (!GI)
	{
		return;
	}

	bool bAllClear =
		GI->ClearCurrentDungeon();

	if (bAllClear)
	{
		UGameplayStatics::OpenLevel(
			this,
			"DragonBossMap");
	}
	else
	{
		UGameplayStatics::OpenLevel(
			this,
			"ForestLevel");
	}

	ActivatedPlayers.AddUnique(Player);

	UE_LOG(LogTemp, Warning,
		TEXT("%s Activated Prism"),
		*Player->GetName());

	if (ActivatedPlayers.Num() >= 2)
	{
		RemoveDarknessDebuff();
	}
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

