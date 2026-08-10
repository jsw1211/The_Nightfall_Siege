#include "QuestGiver.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "GameFramework/PlayerController.h"
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

    SpeakerName = FText::FromString(TEXT("수호자"));
    DialogueLines = {
        FText::FromString(TEXT("여행자여, 숲의 어둠이 점점 짙어지고 있습니다.")),
        FText::FromString(TEXT("던전을 찾아 정화해 주시겠습니까?")),
        FText::FromString(TEXT("수락한다면 파티의 리더에게 길을 안내할 랜턴을 드리겠습니다."))
    };
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
    if (!HasAuthority() || !CanInteractWith(Player)) return;
    if (ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>())
    {
        if (PS->QuestStage == EQuestStage::NotAccepted)
        {
            Player->ClientOpenQuestDialogue(this, DialogueLines, SpeakerName);
        }
        else
        {
            Player->ClientShowQuestMessage(PS->GetQuestObjectiveText().ToString());
        }
    }
}

bool AQuestGiver::CanInteractWith(const ABaseCharacter* Player) const
{
    return Player
        && Player->GetWorld() == GetWorld()
        && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation())
            <= FMath::Square(InteractionRange ? InteractionRange->GetScaledSphereRadius() + 100.f : 280.f);
}

void AQuestGiver::ResolveQuestDecision(ABaseCharacter* Player, bool bAccepted)
{
    if (!HasAuthority() || !CanInteractWith(Player)) return;

    ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>();
    if (!PS || PS->QuestStage != EQuestStage::NotAccepted) return;

    if (!bAccepted)
    {
        Player->ClientFinishQuestDialogue(false, FText::FromString(TEXT("언제든 준비가 되면 다시 말을 걸어 주세요.")));
        return;
    }

    PS->AcceptMainQuest();

    // In a listen-server party, the server's first controller owns the host pawn.
    ABaseCharacter* PartyLeader = nullptr;
    if (APlayerController* HostController = GetWorld()->GetFirstPlayerController())
    {
        PartyLeader = Cast<ABaseCharacter>(HostController->GetPawn());
    }
    if (!PartyLeader) PartyLeader = Player;
    PartyLeader->GrantQuestLantern();

    Player->ClientFinishQuestDialogue(true, PS->GetQuestObjectiveText());
}
