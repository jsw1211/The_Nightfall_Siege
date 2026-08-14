// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Altar.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UDecalComponent;
class AMonster;
class ALantern;
class ABaseCharacter;
class USphereComponent;
class UPointLightComponent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API AAltar : public AActor
{
	GENERATED_BODY()

public:
	AAltar();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 제단 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* AltarMesh;

	// 상호작용 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* InteractionBox;

	// 활성화 여부
	UPROPERTY(ReplicatedUsing = OnRep_Activated, BlueprintReadOnly)
	bool bActivated;

	// 플레이어 범위 안 여부
	bool bPlayerInside;

	// 이 제단 몬스터 목록
	UPROPERTY()
	TArray<AMonster*> OwnedMonsters;

	// 몬스터 등록
	void RegisterMonster(AMonster* Monster);

	// 제단 활성화
	void ActivateAltar();

	UPROPERTY(ReplicatedUsing = OnRep_LanternPlaced, BlueprintReadOnly)
	bool bLanternPlaced = false;

	UPROPERTY(ReplicatedUsing = OnRep_Cleared, BlueprintReadOnly, Category = "Altar")
	bool bCleared = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Altar")
	int32 RemainingMonsterCount = 0;

	void PlaceLantern(ABaseCharacter* Player);
	void RemoveLantern(ABaseCharacter* Player);
	bool NotifyOwnedMonsterDefeated();
	// The altar's circular damageable zone, shared with its visible light.
	bool IsInsideActiveLightZone(const FVector& WorldLocation) const;
	bool IsAvailableNavigationTarget() const { return !bCleared && !bLanternPlaced; }

	UFUNCTION()
	void OnRep_Activated();

	UFUNCTION()
	void OnRep_LanternPlaced();

	UFUNCTION()
	void OnRep_Cleared();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* LightRange;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* LanternMesh;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* AltarLight;

	UPROPERTY(VisibleAnywhere, Category = "Altar")
	UDecalComponent* AltarSafeZoneDecal;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
