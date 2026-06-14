// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Portal.generated.h"

class ABaseCharacter;

UENUM(BlueprintType)
enum class EPortalType : uint8
{
	ReturnVillage,

	Boss
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	bool bPlayerInside = false;

	UPROPERTY()
	ABaseCharacter* NearbyPlayer = nullptr;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	EPortalType PortalType;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PortalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	UMaterialInterface* ReturnPortalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	UMaterialInterface* BossPortalMaterial;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;

	UFUNCTION()
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable)
	void Interact(ABaseCharacter* Player);
};
