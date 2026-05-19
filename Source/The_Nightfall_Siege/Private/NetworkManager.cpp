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
        WorkerInstance = new FNetworkWorker(ClientSocket, &AddPlayerQueue, &MovePlayerQueue);
        Thread = FRunnableThread::Create(WorkerInstance, TEXT("NetworkReceiverThread"));

        C2S_Login LoginPacket;
        LoginPacket.size = sizeof(LoginPacket);
        LoginPacket.type = C2S_LOGIN;
        strncpy_s(LoginPacket.m_username, "UE_Client", MAX_NAME_LEN);
#include "Windows/AllowWindowsPlatformTypes.h"
        send((SOCKET)ClientSocket, (char*)&LoginPacket, LoginPacket.size, 0);
#include "Windows/HideWindowsPlatformTypes.h"
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

void ANetworkManager::SendMovePacket(short X, short Y, int32 MoveTime)
{
#include "Windows/AllowWindowsPlatformTypes.h"
    C2S_Move Packet;
    Packet.size = sizeof(C2S_Move);
    Packet.type = C2S_MOVE;
    Packet.x = X;                 
    Packet.y = Y;                 
    Packet.move_time = MoveTime;  

    if (ClientSocket != INVALID_SOCKET)
    {
        int32 SentBytes = send((SOCKET)ClientSocket, reinterpret_cast<const char*>(&Packet), Packet.size, 0);
        if (SentBytes == SOCKET_ERROR)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to send move packet."));
        }
    }
#include "Windows/HideWindowsPlatformTypes.h"
}

void ANetworkManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    S2C_AddPlayer AddInfo;
    while (AddPlayerQueue.Dequeue(AddInfo)) {
        if (RemotePlayers.Contains(AddInfo.playerId)) continue;

        if (GetWorld() && RemotePlayerClass) {
            FActorSpawnParameters Params;
            FVector Loc(AddInfo.x * 100.f, AddInfo.y * 100.f, 100.f);
            AActor* NewActor = GetWorld()->SpawnActor<AActor>(RemotePlayerClass, Loc, FRotator::ZeroRotator, Params);
            if (NewActor) RemotePlayers.Add(AddInfo.playerId, NewActor);
        }
    }

	S2C_MovePlayer MoveInfo;
    while (MovePlayerQueue.Dequeue(MoveInfo)) {
        if (RemotePlayers.Contains(MoveInfo.playerId)) {
            AActor* TargetActor = RemotePlayers[MoveInfo.playerId];
            if (TargetActor) {
                FVector NewLoc(MoveInfo.x * 100.f, MoveInfo.y * 100.f, TargetActor->GetActorLocation().Z);
                TargetActor->SetActorLocation(NewLoc);
            }
        }
	}
}

void ANetworkManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 1. 소켓을 먼저 파괴하여 recv 블로킹을 강제로 깨부숩니다.
#include "Windows/AllowWindowsPlatformTypes.h"
    if (ClientSocket != INVALID_SOCKET)
    {
        closesocket((SOCKET)ClientSocket);
        ClientSocket = INVALID_SOCKET; // 중복 해제 방지
    }
    //WSACleanup();
#include "Windows/HideWindowsPlatformTypes.h"

    // 2. 이제 recv에서 빠져나온 스레드를 안전하게 정리합니다.
    if (Thread)
    {
        WorkerInstance->Stop();
        Thread->WaitForCompletion(); // 이제 수신 스레드가 풀려났으므로 대기 없이 즉시 통과합니다!
        delete Thread;
        Thread = nullptr;
    }

    if (WorkerInstance)
    {
        delete WorkerInstance;
        WorkerInstance = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}



FNetworkWorker::FNetworkWorker(uintptr_t InSocket,
    TQueue<S2C_AddPlayer, EQueueMode::Spsc>* InAddQueue,
    TQueue<S2C_MovePlayer, EQueueMode::Spsc>* InMoveQueue)
    : ClientSocket(InSocket), AddPlayerQueue(InAddQueue), MovePlayerQueue(InMoveQueue)
{
    bRunThread = true;
}

FNetworkWorker::~FNetworkWorker()
{
    Stop();
}

bool FNetworkWorker::Init()
{
    return true; 
}

uint32 FNetworkWorker::Run()
{
    unsigned char RecvBuffer[1024];
    int32 TotalBytesBuffer = 0;

    while (bRunThread)
    {
#include "Windows/AllowWindowsPlatformTypes.h"
        int32 BytesReceived = recv((SOCKET)ClientSocket, (char*)(RecvBuffer + TotalBytesBuffer), sizeof(RecvBuffer) - TotalBytesBuffer, 0);
#include "Windows/HideWindowsPlatformTypes.h"

        if (BytesReceived > 0)
        {
            TotalBytesBuffer += BytesReceived;
            int Processed = 0;

            // 최소 헤더 크기(2바이트: size, type) 이상이 남았을 때만 파싱
            while (Processed + 2 <= TotalBytesBuffer)
            {
                uint8 PacketSize = RecvBuffer[Processed];

                // 패킷이 완전히 다 도착하지 않았다면 루프 탈출 후 다음 recv를 대기
                if (Processed + PacketSize > TotalBytesBuffer)
                {
                    break;
                }

                uint8 PacketType = RecvBuffer[Processed + 1];

                // 1. 플레이어 추가 패킷 처리
                if (PacketType == S2C_ADD_PLAYER)
                {
                    S2C_AddPlayer* AddPacket = (S2C_AddPlayer*)&RecvBuffer[Processed];
                    AddPlayerQueue->Enqueue(*AddPacket);
                }
                // 2. 플레이어 이동 패킷 처리
                else if (PacketType == S2C_MOVE_PLAYER)
                {
                    S2C_MovePlayer* MovePacket = (S2C_MovePlayer*)&RecvBuffer[Processed];
                    MovePlayerQueue->Enqueue(*MovePacket);
                }

                Processed += PacketSize;
            }

            // 처리하고 남은 자투리 데이터가 있다면 버퍼의 맨 앞으로 당겨줌 (TCP 패킷 밀림/잘림 방지)
            if (Processed > 0)
            {
                int32 RemainingBytes = TotalBytesBuffer - Processed;
                if (RemainingBytes > 0)
                {
                    FMemory::Memmove(RecvBuffer, &RecvBuffer[Processed], RemainingBytes);
                }
                TotalBytesBuffer = RemainingBytes;
            }
        }
        else if (BytesReceived == 0 || BytesReceived == SOCKET_ERROR)
        {
            bRunThread = false;
        }
    }
    return 0;
}

void FNetworkWorker::Stop() { bRunThread = false; }
