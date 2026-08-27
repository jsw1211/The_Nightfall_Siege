#include "QuestGiver.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "QuestInteractionWidget.h"
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
    InteractionText->SetText(FText::FromString(TEXT("마을 촌장\n[F] 대화")));
    // TextRender's default font has no Korean glyphs. Keep the component for
    // backwards-compatible Blueprint layouts, but render the actual label via
    // UMG using the imported Noto Sans KR FontFace.
    InteractionText->SetVisibility(false);

    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(SceneRoot);
    InteractionWidget->SetRelativeLocation(FVector(0.f, 0.f, 205.f));
    InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
    InteractionWidget->SetDrawSize(FVector2D(420.f, 110.f));
    InteractionWidget->SetPivot(FVector2D(0.5f, 0.5f));
    InteractionWidget->SetWidgetClass(UQuestInteractionWidget::StaticClass());

    SpeakerName = FText::FromString(TEXT("마을 촌장"));
    DialogueLines = {
        FText::FromString(TEXT("여행자여, 사악한 용에 의해 세상이 어둠에 잠겼습니다.")),
        FText::FromString(TEXT("부디 용을 처치하고 세상에 빛을 되찾아주세요.")),
        FText::FromString(TEXT("수락하신다면 세상의 마지막 빛이 담긴 랜턴을 드리겠습니다.")),
        FText::FromString(TEXT("이 랜턴으로 용의 수하들을 처치해 용과 대적할 아이템을 얻고 용에게 도전하세요."))
    };
}

void AQuestGiver::BeginPlay()
{
    Super::BeginPlay();

    // The widget is created during component registration, which has completed
    // by BeginPlay. Calling this from the constructor fails during cooking.
    InteractionWidget->SetWindowVisibility(EWindowVisibility::SelfHitTestInvisible);

    // Child Blueprints may have serialized the old component as visible.
    // Hide it again at runtime so only the Korean-capable UMG label renders.
    if (InteractionText)
    {
        InteractionText->SetVisibility(false, true);
    }
}

void AQuestGiver::OnRangeBegin(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor)) Player->SetNearbyQuestGiver(this);
}

void AQuestGiver::OnRangeEnd(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
    if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
    {
        Player->SetNearbyQuestGiver(nullptr);
        if (HasAuthority() && Player == ActiveDialogueOwner)
        {
            CancelActiveDialogue();
        }
    }
}

void AQuestGiver::OnDialogueOwnerDestroyed(AActor* DestroyedActor)
{
    if (HasAuthority() && DestroyedActor == ActiveDialogueOwner)
    {
        CancelActiveDialogue();
    }
}

void AQuestGiver::Interact(ABaseCharacter* Player)
{
    if (!HasAuthority() || !CanInteractWith(Player)) return;

    // The first valid interaction owns the public conversation. Further F
    // presses must never replace that owner while the dialogue is active.
    if (ActiveDialogueOwner)
    {
        if (IsValid(ActiveDialogueOwner) && CanInteractWith(ActiveDialogueOwner)) return;
        CancelActiveDialogue();
    }

    ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>();
    if (!PS) return;

    ActiveDialogueOwner = Player;
    ActiveDialogueOwner->OnDestroyed.AddUniqueDynamic(this, &AQuestGiver::OnDialogueOwnerDestroyed);
    ActiveDialoguePage = 0;
    bShowingQuestChoice = false;
    bActiveDialogueRequiresDecision = PS->QuestStage == EQuestStage::NotAccepted;
    ActiveDialogueLines = bActiveDialogueRequiresDecision
        ? DialogueLines
        : TArray<FText>{ FText::FromString(TEXT("부디 저희를 도와주세요.")) };

    if (DialogueSessionCounter == MAX_int32)
    {
        DialogueSessionCounter = 1;
    }
    else
    {
        ++DialogueSessionCounter;
    }
    ActiveDialogueSessionId = DialogueSessionCounter;

    BroadcastDialogueOpened();
}

bool AQuestGiver::CanInteractWith(const ABaseCharacter* Player) const
{
    return Player
        && Player->GetWorld() == GetWorld()
        && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation())
            <= FMath::Square(InteractionRange ? InteractionRange->GetScaledSphereRadius() + 100.f : 280.f);
}

void AQuestGiver::AdvanceDialogue(ABaseCharacter* Player, int32 DialogueSessionId)
{
    if (!HasAuthority()
        || Player != ActiveDialogueOwner
        || DialogueSessionId != ActiveDialogueSessionId
        || bShowingQuestChoice)
    {
        return;
    }

    if (!CanInteractWith(Player))
    {
        CancelActiveDialogue();
        return;
    }

    if (ActiveDialoguePage + 1 < ActiveDialogueLines.Num())
    {
        ++ActiveDialoguePage;
        BroadcastDialogueState();
        return;
    }

    if (bActiveDialogueRequiresDecision)
    {
        bShowingQuestChoice = true;
        BroadcastDialogueState();
        return;
    }

    FinishActiveDialogue(false, FText::GetEmpty());
}

void AQuestGiver::ResolveQuestDecision(ABaseCharacter* Player, int32 DialogueSessionId, bool bAccepted)
{
    if (!HasAuthority()
        || Player != ActiveDialogueOwner
        || DialogueSessionId != ActiveDialogueSessionId
        || !bActiveDialogueRequiresDecision
        || !bShowingQuestChoice)
    {
        return;
    }

    if (!CanInteractWith(Player))
    {
        CancelActiveDialogue();
        return;
    }

    ABasePlayerState* PS = Player->GetPlayerState<ABasePlayerState>();
    if (!PS || PS->QuestStage != EQuestStage::NotAccepted)
    {
        CancelActiveDialogue();
        return;
    }

    if (!bAccepted)
    {
        FinishActiveDialogue(false, FText::FromString(TEXT("언제든 준비가 되면 다시 말을 걸어 주세요.")));
        return;
    }

    PS->AcceptMainQuest();

    // The lantern is held by the listen-server host, regardless of which
    // party member accepts the shared quest.
    ABaseCharacter* PartyLeader = nullptr;
    if (APlayerController* HostController = GetWorld()->GetFirstPlayerController())
    {
        PartyLeader = Cast<ABaseCharacter>(HostController->GetPawn());
    }
    if (PartyLeader)
    {
        PartyLeader->GrantQuestLantern();
    }

    FinishActiveDialogue(true, PS->GetQuestObjectiveText());
}

void AQuestGiver::BroadcastDialogueOpened()
{
    if (!HasAuthority() || !ActiveDialogueOwner || ActiveDialogueSessionId == 0) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        ABaseCharacter* Participant = PC ? Cast<ABaseCharacter>(PC->GetPawn()) : nullptr;
        if (!Participant) continue;

        Participant->ClientOpenQuestDialogue(
            this,
            ActiveDialogueLines,
            SpeakerName,
            Participant == ActiveDialogueOwner,
            bActiveDialogueRequiresDecision,
            ActiveDialogueSessionId);
    }
}

void AQuestGiver::BroadcastDialogueState()
{
    if (!HasAuthority() || ActiveDialogueSessionId == 0) return;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (ABaseCharacter* Participant = PC ? Cast<ABaseCharacter>(PC->GetPawn()) : nullptr)
        {
            Participant->ClientUpdateQuestDialogue(
                this,
                ActiveDialogueSessionId,
                ActiveDialoguePage,
                bShowingQuestChoice);
        }
    }
}

void AQuestGiver::FinishActiveDialogue(bool bAccepted, const FText& ResultMessage)
{
    if (!HasAuthority() || ActiveDialogueSessionId == 0) return;

    const int32 FinishedSessionId = ActiveDialogueSessionId;

    // Clear server ownership before notifying the listen-server client so a
    // new interaction can never inherit the completed session.
    if (ActiveDialogueOwner)
    {
        ActiveDialogueOwner->OnDestroyed.RemoveDynamic(this, &AQuestGiver::OnDialogueOwnerDestroyed);
    }
    ActiveDialogueOwner = nullptr;
    ActiveDialogueLines.Reset();
    ActiveDialogueSessionId = 0;
    ActiveDialoguePage = 0;
    bActiveDialogueRequiresDecision = false;
    bShowingQuestChoice = false;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (ABaseCharacter* Participant = PC ? Cast<ABaseCharacter>(PC->GetPawn()) : nullptr)
        {
            Participant->ClientFinishQuestDialogue(this, FinishedSessionId, bAccepted, ResultMessage);
        }
    }
}

void AQuestGiver::CancelActiveDialogue()
{
    FinishActiveDialogue(false, FText::GetEmpty());
}
