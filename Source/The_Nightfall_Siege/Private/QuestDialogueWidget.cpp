#include "QuestDialogueWidget.h"

#include "BaseCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

TSharedRef<SWidget> UQuestDialogueWidget::RebuildWidget()
{
    BuildFallbackLayout();
    return Super::RebuildWidget();
}

void UQuestDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    if (AcceptButton) AcceptButton->OnClicked.AddDynamic(this, &UQuestDialogueWidget::HandleAcceptClicked);
    if (DeclineButton) DeclineButton->OnClicked.AddDynamic(this, &UQuestDialogueWidget::HandleDeclineClicked);
    RefreshDialogueView();
}

void UQuestDialogueWidget::ConfigureDialogue(AQuestGiver* InQuestGiver, const TArray<FText>& InLines,
    const FText& InSpeakerName, bool bInCanControlDialogue,
    bool bInRequiresQuestDecision, int32 InDialogueSessionId)
{
    QuestGiver = InQuestGiver;
    DialogueLines = InLines;
    SpeakerName = InSpeakerName;
    CurrentPage = 0;
    DialogueSessionId = InDialogueSessionId;
    bShowingChoice = false;
    bCanControlDialogue = bInCanControlDialogue;
    bRequiresQuestDecision = bInRequiresQuestDecision;
    RefreshDialogueView();
    if (bCanControlDialogue)
    {
        SetKeyboardFocus();
    }
}

void UQuestDialogueWidget::SetDialogueState(int32 InDialoguePage, bool bInShowingQuestChoice)
{
    const bool bWasShowingChoice = bShowingChoice;
    CurrentPage = DialogueLines.Num() > 0
        ? FMath::Clamp(InDialoguePage, 0, DialogueLines.Num() - 1)
        : 0;
    bShowingChoice = bInShowingQuestChoice;
    RefreshDialogueView();

    if (!bWasShowingChoice && bShowingChoice && bCanControlDialogue && bRequiresQuestDecision)
    {
        OnQuestChoiceShown();
    }
}

bool UQuestDialogueWidget::MatchesDialogue(const AQuestGiver* InQuestGiver, int32 InDialogueSessionId) const
{
    return QuestGiver == InQuestGiver && DialogueSessionId == InDialogueSessionId;
}

FReply UQuestDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::F)
    {
        if (bCanControlDialogue)
        {
            if (!bShowingChoice) AdvanceDialogue();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UQuestDialogueWidget::AdvanceDialogue()
{
    if (!bCanControlDialogue || bShowingChoice) return;
    if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwningPlayerPawn()))
    {
        Character->ServerAdvanceQuestDialogue(QuestGiver, DialogueSessionId);
    }
}

void UQuestDialogueWidget::AcceptQuest() { SubmitDecision(true); }
void UQuestDialogueWidget::DeclineQuest() { SubmitDecision(false); }

FText UQuestDialogueWidget::GetCurrentDialogueLine() const
{
    return DialogueLines.IsValidIndex(CurrentPage) ? DialogueLines[CurrentPage] : FText::GetEmpty();
}

void UQuestDialogueWidget::RefreshDialogueView()
{
    const int32 PageCount = DialogueLines.Num();
    const FText Line = GetCurrentDialogueLine();
    if (SpeakerText) SpeakerText->SetText(SpeakerName);
    if (DialogueText) DialogueText->SetText(Line);
    if (PageText)
    {
        if (bShowingChoice)
        {
            PageText->SetText(bCanControlDialogue
                ? FText::FromString(TEXT("퀘스트를 수락하시겠습니까?"))
                : FText::FromString(TEXT("대화를 시작한 플레이어가 결정 중입니다.")));
        }
        else
        {
            PageText->SetText(bCanControlDialogue
                ? FText::FromString(FString::Printf(TEXT("[F] 다음  %d / %d"), CurrentPage + 1, PageCount))
                : FText::FromString(FString::Printf(TEXT("대화 진행 중  %d / %d"), CurrentPage + 1, PageCount)));
        }
    }

    const bool bShouldShowChoice = bShowingChoice && bCanControlDialogue && bRequiresQuestDecision;
    if (AcceptButton) AcceptButton->SetVisibility(bShouldShowChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (DeclineButton) DeclineButton->SetVisibility(bShouldShowChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    OnDialoguePageChanged(SpeakerName, Line, CurrentPage + 1, PageCount);
}

void UQuestDialogueWidget::SubmitDecision(bool bAccepted)
{
    if (!bShowingChoice || !bCanControlDialogue || !bRequiresQuestDecision) return;
    if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwningPlayerPawn()))
    {
        Character->ServerSubmitQuestDecision(QuestGiver, DialogueSessionId, bAccepted);
    }
}

void UQuestDialogueWidget::HandleAcceptClicked() { AcceptQuest(); }
void UQuestDialogueWidget::HandleDeclineClicked() { DeclineQuest(); }

void UQuestDialogueWidget::BuildFallbackLayout()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuestDialogueRoot"));
    WidgetTree->RootWidget = Root;
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueContent"));
    UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Content);
    CanvasSlot->SetAnchors(FAnchors(0.5f, 0.72f));
    CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    CanvasSlot->SetSize(FVector2D(720.f, 240.f));
    SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerText"));
    DialogueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueText"));
    PageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PageText"));
    AcceptButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AcceptButton"));
    DeclineButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeclineButton"));
    UTextBlock* AcceptLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AcceptLabel"));
    UTextBlock* DeclineLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeclineLabel"));
    AcceptLabel->SetText(FText::FromString(TEXT("수락")));
    DeclineLabel->SetText(FText::FromString(TEXT("거절")));
    AcceptButton->AddChild(AcceptLabel);
    DeclineButton->AddChild(DeclineLabel);
    DialogueText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(SpeakerText);
    Content->AddChildToVerticalBox(DialogueText);
    Content->AddChildToVerticalBox(PageText);
    Content->AddChildToVerticalBox(AcceptButton);
    Content->AddChildToVerticalBox(DeclineButton);
}
