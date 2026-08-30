#include "PausedOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Widgets/SWidget.h"

TSharedRef<SWidget> UPausedOverlayWidget::RebuildWidget()
{
    BuildOverlay();
    return Super::RebuildWidget();
}

void UPausedOverlayWidget::BuildOverlay()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("PausedOverlayRoot"));
    Root->SetVisibility(ESlateVisibility::HitTestInvisible);
    WidgetTree->RootWidget = Root;

    UTextBlock* PausedText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("PausedText"));
    PausedText->SetText(FText::FromString(TEXT("Paused")));
    PausedText->SetJustification(ETextJustify::Center);
    PausedText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    PausedText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
    PausedText->SetShadowOffset(FVector2D(3.f, 3.f));

    FSlateFontInfo PausedFont = PausedText->GetFont();
    PausedFont.Size = 72;
    PausedText->SetFont(PausedFont);

    UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(PausedText);
    TextSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    TextSlot->SetPosition(FVector2D::ZeroVector);
    TextSlot->SetSize(FVector2D(600.f, 120.f));
}
