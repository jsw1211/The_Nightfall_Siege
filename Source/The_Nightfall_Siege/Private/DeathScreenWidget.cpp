#include "DeathScreenWidget.h"

#include "BaseController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Widgets/SWidget.h"

TSharedRef<SWidget> UDeathScreenWidget::RebuildWidget()
{
    BuildDeathScreen();
    return Super::RebuildWidget();
}

void UDeathScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (RetryButton)
    {
        RetryButton->OnClicked.RemoveDynamic(this, &UDeathScreenWidget::HandleRetryClicked);
        RetryButton->OnClicked.AddDynamic(this, &UDeathScreenWidget::HandleRetryClicked);
    }
}

void UDeathScreenWidget::BuildDeathScreen()
{
    if (!WidgetTree || WidgetTree->RootWidget || RetryButton) return;

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DeathScreenRoot"));
    WidgetTree->RootWidget = Root;

    UTextBlock* DeathText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("YouDiedText"));
    DeathText->SetText(FText::FromString(TEXT("YOU DIED")));
    FSlateFontInfo DeathFont = DeathText->GetFont();
    DeathFont.Size = 64;
    DeathText->SetFont(DeathFont);
    DeathText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.05f, 0.05f)));
    DeathText->SetJustification(ETextJustify::Center);
    UCanvasPanelSlot* DeathSlot = Root->AddChildToCanvas(DeathText);
    DeathSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    DeathSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    DeathSlot->SetPosition(FVector2D(0.f, -55.f));
    DeathSlot->SetSize(FVector2D(700.f, 100.f));

    RetryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RetryButton"));
    UTextBlock* RetryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RetryText"));
    RetryText->SetText(FText::FromString(TEXT("RETRY")));
    FSlateFontInfo RetryFont = RetryText->GetFont();
    RetryFont.Size = 30;
    RetryText->SetFont(RetryFont);
    RetryText->SetJustification(ETextJustify::Center);
    RetryButton->AddChild(RetryText);
    UCanvasPanelSlot* RetrySlot = Root->AddChildToCanvas(RetryButton);
    RetrySlot->SetAnchors(FAnchors(0.5f, 0.5f));
    RetrySlot->SetAlignment(FVector2D(0.5f, 0.5f));
    RetrySlot->SetPosition(FVector2D(0.f, 45.f));
    RetrySlot->SetSize(FVector2D(220.f, 58.f));
    SetRetryAvailable(false);
}

void UDeathScreenWidget::SetRetryAvailable(bool bAvailable)
{
    if (RetryButton)
    {
        RetryButton->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        RetryButton->SetIsEnabled(bAvailable);
    }
}

void UDeathScreenWidget::HandleRetryClicked()
{
    if (ABaseController* Controller = Cast<ABaseController>(GetOwningPlayer()))
    {
        Controller->RequestRetry();
    }
}
