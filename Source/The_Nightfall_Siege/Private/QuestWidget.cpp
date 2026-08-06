#include "QuestWidget.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UQuestWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (QuestObjectiveText || !WidgetTree)
    {
        return;
    }

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuestRoot"));
    WidgetTree->RootWidget = Root;

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestPanel"));
    Panel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.06f, 0.88f));
    UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.66f, 0.055f, 0.96f, 0.17f));
    PanelSlot->SetOffsets(FMargin(0.f));

    QuestObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestObjectiveText"));
    QuestObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.82f, 0.34f, 1.f)));
    QuestObjectiveText->SetAutoWrapText(true);
    QuestObjectiveText->SetJustification(ETextJustify::Center);
    FSlateFontInfo Font = QuestObjectiveText->GetFont();
    Font.Size = 20;
    QuestObjectiveText->SetFont(Font);
    Panel->SetContent(QuestObjectiveText);
}

void UQuestWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
    if (ABasePlayerState* PlayerState = Player ? Player->GetPlayerState<ABasePlayerState>() : nullptr)
    {
        ShowObjective(PlayerState->GetQuestObjectiveText());
    }
}

void UQuestWidget::ShowObjective(const FText& Objective)
{
    if (QuestObjectiveText) QuestObjectiveText->SetText(Objective);
}
