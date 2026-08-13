#include "QuestWidget.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UQuestWidget::RebuildWidget()
{
    BuildQuestLayout();
    return Super::RebuildWidget();
}

void UQuestWidget::BuildQuestLayout()
{
    if (QuestObjectiveText || !WidgetTree)
    {
        return;
    }

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuestRoot"));
    WidgetTree->RootWidget = Root;

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestPanel"));
    Panel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.06f, 0.88f));
    Panel->SetPadding(FMargin(18.f, 14.f));
    UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
    PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
    PanelSlot->SetPosition(FVector2D(-36.f, 42.f));
    PanelSlot->SetSize(FVector2D(390.f, 145.f));

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestContent"));
    Panel->SetContent(Content);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTitleText"));
    Title->SetText(FText::FromString(TEXT("메인 퀘스트")));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.78f, 0.2f, 1.f)));
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.Size = 22;
    Title->SetFont(TitleFont);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

    QuestObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestObjectiveText"));
    QuestObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    QuestObjectiveText->SetAutoWrapText(true);
    QuestObjectiveText->SetJustification(ETextJustify::Left);
    FSlateFontInfo Font = QuestObjectiveText->GetFont();
    Font.Size = 17;
    QuestObjectiveText->SetFont(Font);
    Content->AddChildToVerticalBox(QuestObjectiveText)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

    QuestProgressText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestProgressText"));
    QuestProgressText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.78f, 1.f, 1.f)));
    FSlateFontInfo ProgressFont = QuestProgressText->GetFont();
    ProgressFont.Size = 15;
    QuestProgressText->SetFont(ProgressFont);
    Content->AddChildToVerticalBox(QuestProgressText);
}

void UQuestWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
    if (ABasePlayerState* PlayerState = Player ? Player->GetPlayerState<ABasePlayerState>() : nullptr)
    {
        const int32 QuestStage = static_cast<int32>(PlayerState->QuestStage);
        if (QuestStage == CachedQuestStage
            && PlayerState->ClearedDungeonCount == CachedClearedDungeonCount
            && PlayerState->DungeonMonsterKillCount == CachedDungeonMonsterKillCount
            && PlayerState->DungeonMonsterTotalCount == CachedDungeonMonsterTotalCount)
        {
            return;
        }

        CachedQuestStage = QuestStage;
        CachedClearedDungeonCount = PlayerState->ClearedDungeonCount;
        CachedDungeonMonsterKillCount = PlayerState->DungeonMonsterKillCount;
        CachedDungeonMonsterTotalCount = PlayerState->DungeonMonsterTotalCount;
        ShowObjective(PlayerState->GetQuestObjectiveText());

        if (QuestProgressText)
        {
			if (PlayerState->QuestStage == EQuestStage::ClearDungeon)
			{
				QuestProgressText->SetText(FText::FromString(FString::Printf(
					TEXT("처치 몬스터 : %d/%d"),
					PlayerState->DungeonMonsterKillCount,
					PlayerState->DungeonMonsterTotalCount)));
				return;
			}

            const bool bBossQuest = PlayerState->QuestStage == EQuestStage::FindBossPortal
                || PlayerState->QuestStage == EQuestStage::DefeatBoss;
            const FString Progress = PlayerState->QuestStage == EQuestStage::Completed
                ? TEXT("진행도: 완료")
                : bBossQuest
                    ? TEXT("진행도: 보스 퀘스트")
                    : FString::Printf(TEXT("던전 클리어: %d / 3"), PlayerState->ClearedDungeonCount);
            QuestProgressText->SetText(FText::FromString(Progress));
        }
    }
}

void UQuestWidget::ShowObjective(const FText& Objective)
{
    if (QuestObjectiveText && !Objective.EqualTo(CachedObjective))
    {
        QuestObjectiveText->SetText(Objective);
        CachedObjective = Objective;
    }
}
