// Fill out your copyright notice in the Description page of Project Settings.


#include "Altar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Monster.h"

// Sets default values
AAltar::AAltar()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 메쉬 생성
	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));

	RootComponent = AltarMesh;

	// 충돌 박스 생성
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));

	InteractionBox->SetupAttachment(RootComponent);

	// 박스 크기
	InteractionBox->SetBoxExtent(FVector(300.f));

	bActivated = false;
	bPlayerInside = false;
}

// Called when the game starts or when spawned
void AAltar::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAltar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Player =
		UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (!Player) return;

	bPlayerInside =
		InteractionBox->IsOverlappingActor(Player);

	// F키 상호작용
	if (bPlayerInside)
	{
		APlayerController* PC =
			UGameplayStatics::GetPlayerController(GetWorld(), 0);

		if (PC && PC->WasInputKeyJustPressed(EKeys::F))
		{
			ActivateAltar();
		}
	}
}

void AAltar::ActivateAltar()
{
	if (bActivated) return;

	bActivated = true;

	UE_LOG(LogTemp, Warning, TEXT("Altar Activated"));
}

void AAltar::RegisterMonster(AMonster* Monster)
{
	if (Monster)
	{
		OwnedMonsters.Add(Monster);
	}
}
