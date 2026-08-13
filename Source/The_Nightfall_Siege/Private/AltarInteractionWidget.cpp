#include "AltarInteractionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UAltarInteractionWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        UTextBlock* Prompt = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("AltarInteractionPrompt"));
        Prompt->SetText(FText::FromString(TEXT("[Press F to interact]")));
        Prompt->SetJustification(ETextJustify::Center);
        Prompt->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
        Prompt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Prompt->SetShadowOffset(FVector2D(1.f, 1.f));
        Prompt->SetShadowColorAndOpacity(FLinearColor::Black);
        WidgetTree->RootWidget = Prompt;
    }

    return Super::RebuildWidget();
}
