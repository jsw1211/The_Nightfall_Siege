// Fill out your copyright notice in the Description page of Project Settings.


#include "Lantern.h"
#include "BaseCharacter.h"
#include "TheNightfallSiegeInstance.h"

// Sets default values
ALantern::ALantern()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternMesh"));

	RootComponent = LanternMesh;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));

	InteractionSphere->SetupAttachment(RootComponent);

	InteractionSphere->SetSphereRadius(200.f);
}

// Called when the game starts or when spawned
void ALantern::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALantern::OnOverlapBegin);

	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ALantern::OnOverlapEnd);

    UTheNightfallSiegeInstance* GI =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance());

    if (GI && GI->bHasLantern)
    {
        Destroy();

        return;
    }
}

// Called every frame
void ALantern::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALantern::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);

    if (Player)
    {
        Player->SetNearbyLantern(this);

        UE_LOG(LogTemp, Warning,
            TEXT("Press F To Pick Lantern"));
    }
}

void ALantern::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);

    if (Player)
    {
        Player->SetNearbyLantern(nullptr);
    }
}