#include "QuestInteractionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"

TSharedRef<SWidget> UQuestInteractionWidget::RebuildWidget()
{
    BuildLayout();
    return Super::RebuildWidget();
}

void UQuestInteractionWidget::BuildLayout()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuestInteractionRoot"));
    WidgetTree->RootWidget = Root;

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestInteractionContent"));
    UCanvasPanelSlot* ContentSlot = Root->AddChildToCanvas(Content);
    ContentSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    ContentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    ContentSlot->SetSize(FVector2D(420.f, 110.f));

    // RebuildWidget runs after construction, where FObjectFinder is invalid.
    // LoadObject is safe here and resolves the imported FontFace at runtime.
    UFontFace* KoreanFontFace = LoadObject<UFontFace>(
        nullptr,
        TEXT("/Game/Asset/UI/Font/NOTOSANSKR-VF.NOTOSANSKR-VF"));

    // A FontFace is raw data, while Slate text needs a composite UFont that
    // maps its typeface name to that data.  Supplying the FontFace directly
    // makes Slate fall back to the default Latin font for Hangul characters.
    UFont* KoreanFont = NewObject<UFont>(this, NAME_None, RF_Transient);
    KoreanFont->FontCacheType = EFontCacheType::Runtime;
    KoreanFont->RuntimeFontSource = ERuntimeFontSource::Asset;
    FTypefaceEntry& FontEntry = KoreanFont->GetMutableInternalCompositeFont()
        .DefaultTypeface.Fonts.AddDefaulted_GetRef();
    FontEntry.Name = FName(TEXT("Regular"));
    FontEntry.Font = FFontData(KoreanFontFace);

    const FSlateFontInfo LabelFont(KoreanFont, 20, FName(TEXT("Regular")));

    UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestGiverName"));
    NameText->SetText(FText::FromString(TEXT("마을 촌장")));
    NameText->SetFont(LabelFont);
    NameText->SetJustification(ETextJustify::Center);
    NameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.82f, 0.3f, 1.f)));
    Content->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));

    UTextBlock* QuestPromptText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("QuestGiverQuestPrompt"));

    QuestPromptText->SetText(FText::FromString(TEXT("퀘스트 [F]")));
    QuestPromptText->SetFont(FSlateFontInfo(KoreanFont, 15, FName(TEXT("Regular"))));
    QuestPromptText->SetJustification(ETextJustify::Center);
    QuestPromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Content->AddChildToVerticalBox(QuestPromptText);

    UTextBlock* ShopPromptText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("QuestGiverShopPrompt"));

    ShopPromptText->SetText(FText::FromString(TEXT("상점 [P]")));
    ShopPromptText->SetFont(FSlateFontInfo(KoreanFont, 15, FName(TEXT("Regular"))));
    ShopPromptText->SetJustification(ETextJustify::Center);
    ShopPromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Content->AddChildToVerticalBox(ShopPromptText);
}
