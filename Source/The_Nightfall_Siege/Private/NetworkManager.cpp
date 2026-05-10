// Fill out your copyright notice in the Description page of Project Settings.

#include "NetworkManager.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

// Sets default values
ANetworkManager::ANetworkManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    ClientSocket = INVALID_SOCKET;
    WorkerInstance = nullptr;
    Thread = nullptr;
}

// Called when the game starts or when spawned
void ANetworkManager::BeginPlay()
{
	Super::BeginPlay();
    if (ConnectToServer(TEXT("127.0.0.1"), 3500)) // 서버 접속 성공 시
    {
        // 스레드 생성 및 실행
        WorkerInstance = new FNetworkWorker(ClientSocket, &AddPlayerQueue);
        Thread = FRunnableThread::Create(WorkerInstance, TEXT("NetworkReceiverThread"));
    }
	
}

bool ANetworkManager::ConnectToServer(FString IPAddress, int32 Port)
{
#include "Windows/AllowWindowsPlatformTypes.h"
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Port);
    inet_pton(AF_INET, TCHAR_TO_ANSI(*IPAddress), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;

    ClientSocket = (uintptr_t)sock;
#include "Windows/HideWindowsPlatformTypes.h"
    return true;
}

void ANetworkManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    S2C_AddPlayer Info;
    while (AddPlayerQueue.Dequeue(Info))
    {
        if (RemotePlayers.Contains(Info.playerId)) continue;

        if (GetWorld() && RemotePlayerClass)
        {
            FActorSpawnParameters Params;
            // 서버 좌표가 (10, 10)이면 언리얼에서는 (1000, 1000) 정도로 보정하는 경우가 많습니다.
            FVector Loc(Info.x * 100.f, Info.y * 100.f, 100.f);

            AActor* NewActor = GetWorld()->SpawnActor<AActor>(RemotePlayerClass, Loc, FRotator::ZeroRotator, Params);
            if (NewActor)
            {
                RemotePlayers.Add(Info.playerId, NewActor);
            }
        }
    }
}

void ANetworkManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Thread) { WorkerInstance->Stop(); Thread->WaitForCompletion(); delete Thread; }
    delete WorkerInstance;

#include "Windows/AllowWindowsPlatformTypes.h"
    if (ClientSocket != INVALID_SOCKET)
        closesocket((SOCKET)ClientSocket);
    WSACleanup();
#include "Windows/HideWindowsPlatformTypes.h"

    Super::EndPlay(EndPlayReason);
}



FNetworkWorker::FNetworkWorker(uintptr_t InSocket, TQueue<S2C_AddPlayer, EQueueMode::Spsc>* InQueue)
    : ClientSocket(InSocket), AddPlayerQueue(InQueue)
{
    bRunThread = true;
}

FNetworkWorker::~FNetworkWorker()
{
    Stop();
}

bool FNetworkWorker::Init() { return true; }

uint32 FNetworkWorker::Run()
{
    while (bRunThread)
    {
        unsigned char RecvBuffer[512];
        int32 BytesReceived = recv((SOCKET)ClientSocket, (char*)RecvBuffer, sizeof(RecvBuffer), 0);

        if (BytesReceived > 0)
        {
            // 패킷 가공 로직 (가장 간단한 형태)
            int Processed = 0;
            while (Processed < BytesReceived)
            {
                uint8 PacketSize = RecvBuffer[Processed];
                uint8 PacketType = RecvBuffer[Processed + 1];

                if (PacketType == S2C_ADD_PLAYER) // protocol.h에 정의된 타입
                {
                    S2C_AddPlayer* AddPacket = (S2C_AddPlayer*)&RecvBuffer[Processed];
                    // 큐에 복사해서 담기
                    AddPlayerQueue->Enqueue(*AddPacket);
                }
                Processed += PacketSize;
            }
        }
        else if (BytesReceived == 0 || BytesReceived == SOCKET_ERROR)
        {
            bRunThread = false; // 접속 종료 시 스레드 중단
        }
    }
    return 0;
}

void FNetworkWorker::Stop() { bRunThread = false; }
