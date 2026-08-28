// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonManager.generated.h"

class ADungeonPrism;
class AAltar;
class AMonster;
class ABaseCharacter;

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

	// Number of monsters successfully spawned for this dungeon instance.
	UPROPERTY(BlueprintReadOnly)
	int32 TotalMonsterCount;

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

	// Minimum number of monsters selected independently for each altar.
	UPROPERTY(EditAnywhere, Category = "Monster|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 MinMonstersPerAltar;

	// Maximum number of monsters selected independently for each altar.
	UPROPERTY(EditAnywhere, Category = "Monster|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxMonstersPerAltar;

	// Kept only so existing maps and Blueprints can deserialize the old fixed
	// count. Runtime spawning uses the min/max range above.
	UPROPERTY()
	int32 MonstersPerAltar_DEPRECATED;

	// 몬스터 생성 함수
	void SpawnMonsters();

	// Development shortcut: defeats every living dungeon monster while keeping
	// the normal dungeon-clear and quest progression flow intact.
	int32 DebugClearDungeon(ABaseCharacter* RewardPlayer);

	bool IsDebugClearInProgress() const { return bDebugClearInProgress; }

private:
	void UpdatePlayerMonsterProgress(bool bMonsterWasKilled);

	bool bDebugClearInProgress = false;
};
