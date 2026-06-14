// Fill out your copyright notice in the Description page of Project Settings.


#include "VillageManager.h"
#include "Portal.h"
#include "TheNightfallSiegeInstance.h"

// Sets default values
AVillageManager::AVillageManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVillageManager::BeginPlay()
{
	Super::BeginPlay();
	
    UE_LOG(LogTemp, Warning, TEXT("VillageManager BeginPlay"));

    UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance());

    if (!GI)
    {
        return;
    }

    if (GI->RemainingDungeons.Num() <= 0 &&
        !GI->bBossPortalSpawned)
    {
        SpawnBossPortal();
    }

}

// Called every frame
void AVillageManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVillageManager::SpawnBossPortal()
{
    if (!BossPortalClass)
    {
        return;
    }

    APortal* Portal =
        GetWorld()->SpawnActor<APortal>(
            BossPortalClass,
            BossPortalLocation,
            FRotator::ZeroRotator);

    if (Portal)
    {
        Portal->PortalType = EPortalType::Boss;
    }
}

