// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "BaseCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    PortalMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));

    RootComponent = PortalMesh;

    Collision =
        CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

    Collision->SetupAttachment(RootComponent);

    Collision->SetSphereRadius(150.f);

    Collision->OnComponentBeginOverlap.AddDynamic(
        this,
        &APortal::OnOverlap);

    Collision->OnComponentEndOverlap.AddDynamic(
        this,
        &APortal::OnEndOverlap);

}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
    switch (PortalType)
    {
    case EPortalType::ReturnVillage:

        if (ReturnPortalMaterial)
        {
            PortalMesh->SetMaterial(
                0,
                ReturnPortalMaterial);
        }

        break;

    case EPortalType::Boss:

        if (BossPortalMaterial)
        {
            PortalMesh->SetMaterial(
                0,
                BossPortalMaterial);
        }

        break;
    }

}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    APawn* Player =
        UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (!Player)
    {
        return;
    }

    bPlayerInside =
        Collision->IsOverlappingActor(Player);

    if (bPlayerInside)
    {
        NearbyPlayer = Cast<ABaseCharacter>(Player);
    }
    else
    {
        NearbyPlayer = nullptr;
    }

}

void APortal::OnOverlap(
    UPrimitiveComponent* OverlappedComponent,
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

    Player->SetNearbyPortal(this);

    UE_LOG(LogTemp, Warning, TEXT("Portal Nearby"));
}

void APortal::OnEndOverlap(
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

    Player->SetNearbyPortal(nullptr);
}

void APortal::Interact(ABaseCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    switch (PortalType)
    {
    case EPortalType::ReturnVillage:

        UGameplayStatics::OpenLevel(
            this,
            FName("Lvl_TopDown"));

        break;

    case EPortalType::Boss:

        UGameplayStatics::OpenLevel(
            this,
            FName("DragonLevelSample"));

        break;
    }
}
