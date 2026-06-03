// Fill out your copyright notice in the Description page of Project Settings.


#include "Altar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Monster.h"
#include "BaseCharacter.h"
#include "Components/SphereComponent.h"

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

	LightRange = CreateDefaultSubobject<USphereComponent>(TEXT("LightRange"));

	LightRange->SetupAttachment(RootComponent);

	LightRange->SetSphereRadius(1200.f);

	LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>( TEXT("LanternMesh"));

	LanternMesh->SetupAttachment(RootComponent);

	LanternMesh->SetVisibility(false);
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
			ABaseCharacter* PlayerChar = Cast<ABaseCharacter>(Player);

			if (!PlayerChar)
				return;

			if (!bLanternPlaced)
			{
				if (PlayerChar->bHasLantern)
				{
					PlaceLantern(PlayerChar);
				}
			}
			else
			{
				RemoveLantern(PlayerChar);
			}
		}
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
	}
}

void AAltar::PlaceLantern(ABaseCharacter* Player)
{
	bLanternPlaced = true;
	bActivated = true;

	Player->bHasLantern = false;
	Player->bLanternEquipped = false;
	Player->bLanternPoseActive = false;

	Player->OnLanternUnequipped();

	LanternMesh->SetVisibility(true);

	UE_LOG(LogTemp, Warning, TEXT("Lantern Placed"));
}

void AAltar::RemoveLantern(ABaseCharacter* Player)
{
	bLanternPlaced = false;
	bActivated = false;

	Player->bHasLantern = true;
	Player->bLanternEquipped = true;
	Player->bLanternPoseActive = true;

	Player->OnLanternEquipped();

	LanternMesh->SetVisibility(false);

	UE_LOG(LogTemp, Warning, TEXT("Lantern Removed"));
}