// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"       
#include "Containers/Queue.h"   
#include "protocol.h"
#include "NetworkManager.generated.h"

class FNetworkWorker : public FRunnable
{
public:
	FNetworkWorker(uintptr_t InSocket, TQueue<S2C_AddPlayer, EQueueMode::Spsc>* InQueue);
	virtual ~FNetworkWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	uintptr_t ClientSocket;
	bool bRunThread = false;
	TQueue<S2C_AddPlayer, EQueueMode::Spsc>* AddPlayerQueue;
};

UCLASS()
class THE_NIGHTFALL_SIEGE_API ANetworkManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANetworkManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool ConnectToServer(FString IPAddress, int32 Port);
	UPROPERTY(EditAnywhere, Category = "Network")
	TSubclassOf<AActor> RemotePlayerClass;

private:
	uintptr_t ClientSocket;
	FNetworkWorker* WorkerInstance;
	FRunnableThread* Thread;
	// 패킷 전달용 큐 및 다른 플레이어 관리 맵
	TQueue<S2C_AddPlayer, EQueueMode::Spsc> AddPlayerQueue;
	TMap<int, AActor*> RemotePlayers; // playerId -> Actor
};

