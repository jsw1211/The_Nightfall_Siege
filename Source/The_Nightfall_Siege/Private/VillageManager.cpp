// Fill out your copyright notice in the Description page of Project Settings.


#include "VillageManager.h"
#include "Portal.h"
#include "TheNightfallSiegeInstance.h"
#include "QuestGiver.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AVillageManager::AVillageManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    QuestGiverClass = AQuestGiver::StaticClass();

}

// Called when the game starts or when spawned
void AVillageManager::BeginPlay()
{
	Super::BeginPlay();
	
    UE_LOG(LogTemp, Warning, TEXT("VillageManager BeginPlay"));

    // This must run on every village load, not only after the boss portal is
    // spawned.  Returning from an ordinary dungeon creates a fresh world.
    EnsureQuestGiver();

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
        if (UTheNightfallSiegeInstance* GI = Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
        {
            GI->bBossPortalSpawned = true;
        }
    }

}

void AVillageManager::EnsureQuestGiver()
{
    if (!HasAuthority())
    {
        return;
    }

    TArray<AActor*> ExistingQuestGivers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQuestGiver::StaticClass(), ExistingQuestGivers);
    if (ExistingQuestGivers.Num() == 0)
    {
        TSubclassOf<AQuestGiver> ClassToSpawn = QuestGiverClass;
        if (!ClassToSpawn)
        {
            ClassToSpawn = AQuestGiver::StaticClass();
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AQuestGiver* SpawnedGiver = GetWorld()->SpawnActor<AQuestGiver>(
            ClassToSpawn,
            GetActorLocation() + QuestGiverOffset + FVector(0.f, 0.f, 110.f),
            GetActorRotation(),
            SpawnParams);

        UE_LOG(LogTemp, Warning, TEXT("Quest giver spawned: %s"), *GetNameSafe(SpawnedGiver));
    }
}

