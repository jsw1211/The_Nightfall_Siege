#include "AltarProgressWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UAltarProgressWidget::RebuildWidget()
{
    BuildLayout();
    return Super::RebuildWidget();
}

void UAltarProgressWidget::BuildLayout()
{
    if (!WidgetTree || ProgressBar)
    {
        return;
    }

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AltarProgressRoot"));
    WidgetTree->RootWidget = Root;
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AltarProgressPanel"));
    Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.78f));
    if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PanelSlot->SetSize(FVector2D(360.f, 70.f));
    }
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AltarProgressContent"));
    Panel->SetContent(Content);
    Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AltarProgressLabel"));
    Label->SetText(FText::FromString(TEXT("랜턴 설치 중...")));
    Label->SetJustification(ETextJustify::Center);
    Content->AddChildToVerticalBox(Label)->SetPadding(FMargin(12.f, 8.f, 12.f, 5.f));
    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("AltarProgressBar"));
    ProgressBar->SetPercent(0.f);
    Content->AddChildToVerticalBox(ProgressBar)->SetPadding(FMargin(12.f, 0.f, 12.f, 10.f));
}

void UAltarProgressWidget::StartProgress(float InDuration)
{
    Duration = FMath::Max(InDuration, KINDA_SMALL_NUMBER);
    Elapsed = 0.f;
    bRunning = true;
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (ProgressBar) ProgressBar->SetPercent(0.f);
}

void UAltarProgressWidget::StopProgress()
{
    bRunning = false;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UAltarProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bRunning) return;
    Elapsed = FMath::Min(Elapsed + InDeltaTime, Duration);
    if (ProgressBar) ProgressBar->SetPercent(Elapsed / Duration);
}
