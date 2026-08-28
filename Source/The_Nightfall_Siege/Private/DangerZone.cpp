// Fill out your copyright notice in the Description page of Project Settings.


#include "DangerZone.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ADangerZone::ADangerZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LineMaterialAsset(
		TEXT("/Game/BP_Monster/Dragon/M_DangerZone_Line.M_DangerZone_Line"));
	if (LineMaterialAsset.Succeeded())
	{
		LineMaterial = LineMaterialAsset.Object;
	}

	RootComponent = Decal;

}

void ADangerZone::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// BP_DangerZone has a legacy class-default override that disables movement
	// replication even though the native constructor enables it.  Central breath
	// and close breath move their warnings after spawning, so enforce the flag
	// after Blueprint defaults have been applied on both server and clients.
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void ADangerZone::BeginPlay()
{
	Super::BeginPlay();
	
	SetLifeSpan(LifeTime);

}

// Called every frame
void ADangerZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADangerZone::SetLineShape()
{
	if (LineMaterial)
	{
		Decal->SetDecalMaterial(LineMaterial);
	}

	Decal->DecalSize =
		FVector(
			300.f,
			LineWidth,
			LineLength * 0.5f
		);
}

void ADangerZone::SetBiteShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			350.f,
			350.f);
}

void ADangerZone::SetCloseBreathShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			LineWidth,
			LineLength);
}

void ADangerZone::SetFullMapShape()
{
	Decal->DecalSize =
		FVector(
			300.f,
			10000.f,
			10000.f);

	if (FullMapMaterial)
	{
		Decal->SetDecalMaterial(FullMapMaterial);
	}
}

void ADangerZone::OnRep_ZoneType()
{
	switch (ZoneType)
	{
	case EDangerZoneType::Circle:
		SetBiteShape();
		break;

	case EDangerZoneType::Cone:
		SetCloseBreathShape();
		break;

	case EDangerZoneType::Line:
		SetLineShape();
		break;

	case EDangerZoneType::FullMap:
		SetFullMapShape();
		break;
	}
}

void ADangerZone::OnRep_LineLength()
{
	if (ZoneType == EDangerZoneType::Line)
	{
		SetLineShape();
	}
	else if (ZoneType == EDangerZoneType::Cone)
	{
		SetCloseBreathShape();
	}
}

void ADangerZone::OnRep_LineWidth()
{
	if (ZoneType == EDangerZoneType::Line)
	{
		SetLineShape();
	}
	else if (ZoneType == EDangerZoneType::Cone)
	{
		SetCloseBreathShape();
	}
}

void ADangerZone::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADangerZone, ZoneType);
	DOREPLIFETIME(ADangerZone, LineLength);
	DOREPLIFETIME(ADangerZone, LineWidth);
}

