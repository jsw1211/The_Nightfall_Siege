#include "QuestGiver.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AQuestGiver::AQuestGiver()
{
    bReplicates = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
    DummyMesh->SetupAttachment(SceneRoot);
    DummyMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
    DummyMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
    DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DummyMesh->SetVisibility(true, true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/Asset/npc/npc_1"));
    if (Cube.Succeeded()) DummyMesh->SetStaticMesh(Cube.Object);

    InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
    InteractionRange->SetupAttachment(SceneRoot);
    InteractionRange->SetSphereRadius(180.f);
    InteractionRange->SetCollisionProfileName(TEXT("Trigger"));
    InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &AQuestGiver::OnRangeBegin);
    InteractionRange->OnComponentEndOverlap.AddDynamic(this, &AQuestGiver::OnRangeEnd);

    InteractionText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InteractionText"));
    InteractionText->SetupAttachment(SceneRoot);
    InteractionText->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
    InteractionText->SetHorizontalAlignment(EHTA_Center);
    InteractionText->SetText(FText::FromString(TEXT("퀘스트 수호자\n[F] 대화")));
}

void AQuestGiver::BeginPlay()
{
    Super::BeginPlay();
}

void AQuestGiver::OnRangeBegin(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor)) Player->SetNearbyQuestGiver(this);
}

void AQuestGiver::OnRangeEnd(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
    if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor)) Player->SetNearbyQuestGiver(nullptr);
}

void AQuestGiver::Interact(ABaseCharacter* Player)
{
    if (!HasAuthority() || !Player) return;
    if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
    {
        PS->AcceptMainQuest();
        Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
    }
}
