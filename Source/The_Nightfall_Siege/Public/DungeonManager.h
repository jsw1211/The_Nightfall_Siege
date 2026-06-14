// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonManager.generated.h"

class ADungeonPrism;
class AAltar;
class AMonster;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ADungeonManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	// 현재 살아있는 몬스터 수
	UPROPERTY(BlueprintReadOnly)
	int32 AliveMonsterCount;

	// 프리즘 액터
	UPROPERTY(EditAnywhere)
	ADungeonPrism* DungeonPrism;

	// 몬스터 등록
	void RegisterMonster();

	// 몬스터 사망
	bool OnMonsterDead();

	// 몬스터 클래스
	UPROPERTY(EditAnywhere, Category = "Monster")
	TSubclassOf<AMonster> MonsterClass;

	// 제단당 몬스터 수
	UPROPERTY(EditAnywhere)
	int32 MonstersPerAltar;

	// 몬스터 생성 함수
	void SpawnMonsters();
};
