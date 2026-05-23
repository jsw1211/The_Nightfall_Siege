// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Altar.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class AMonster;

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
	UPROPERTY(BlueprintReadOnly)
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
};
