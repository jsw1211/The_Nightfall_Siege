// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "BaseCharacter.h"
#include "Altar.h"
#include "BasePlayerState.h"
#include "TheNightfallSiegeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

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

    PortalMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f));
    SetActorRotation(FRotator(0.f, 90.f, 0.f));

}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
	OnRep_PortalType();
}

void APortal::OnRep_PortalType()
{
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

void APortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APortal, PortalType);
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

    if (PortalType == EPortalType::ReturnVillage && HasAuthority())
    {
        TArray<AActor*> Altars;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAltar::StaticClass(), Altars);
        const bool bLanternWasPlacedAtAltar = Altars.ContainsByPredicate(
            [](const AActor* Actor)
            {
                const AAltar* Altar = Cast<AAltar>(Actor);
                return Altar && Altar->bLanternPlaced;
            });

        // The quest lantern belongs to the listen-server host.  A placed
        // altar is destroyed by travel, so restore it before leaving.
        if (bLanternWasPlacedAtAltar)
        {
            if (APlayerController* HostController = GetWorld()->GetFirstPlayerController())
            {
                if (ABaseCharacter* HostPlayer = Cast<ABaseCharacter>(HostController->GetPawn()))
                {
                    HostPlayer->bHasLantern = true;
                    HostPlayer->bLanternEquipped = false;
                    HostPlayer->bLanternPoseActive = false;
                    if (ABasePlayerState* HostPlayerState = HostPlayer->GetPlayerState<ABasePlayerState>())
                    {
                        HostPlayerState->bHasLantern = true;
                        HostPlayerState->bLanternEquipped = false;
                        HostPlayerState->ForceNetUpdate();
                    }
                    if (UTheNightfallSiegeInstance* GI =
                        Cast<UTheNightfallSiegeInstance>(GetWorld()->GetGameInstance()))
                    {
                        GI->bHasLantern = true;
                        GI->bLanternEquipped = false;
                    }
                    HostPlayer->OnRep_HasLantern();
                    HostPlayer->OnRep_LanternEquipped();
                    HostPlayer->ForceNetUpdate();
                }
            }
        }
    }

    // The next-map pawn must always start with the lantern put away.
    Player->PrepareForPortalTravel();

    switch (PortalType)
    {
    case EPortalType::ReturnVillage:
    {
        if (HasAuthority())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ServerTravel -> Village"));

            GetWorld()->ServerTravel(
                TEXT("/Game/Map/Village+Forest/Village_Forest?listen"));
        }

        break;
    }

    case EPortalType::Boss:
    {
        if (HasAuthority())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ServerTravel -> Boss"));

            GetWorld()->ServerTravel(
                TEXT("/Game/Map/Boss_Arena/Boss_Arena?listen"));
        }

        break;
    }
    }
}
