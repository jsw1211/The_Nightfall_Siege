#include "ShopWidget.h"

#include "BaseCharacter.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	UTextBlock* AddLabel(UWidgetTree* WidgetTree, UVerticalBox* Panel, const FText& Text, int32 FontSize)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetFont(FSlateFontInfo(Label->GetFont().FontObject, FontSize));
		Label->SetJustification(ETextJustify::Center);

		UVerticalBoxSlot* Slot = Panel->AddChildToVerticalBox(Label);
		Slot->SetPadding(FMargin(12.f, 5.f));
		Slot->SetHorizontalAlignment(HAlign_Center);
		return Label;
	}
}

bool UShopWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (WidgetTree->RootWidget)
	{
		return true;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ShopRoot"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopPanel"));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	AddLabel(WidgetTree, Panel, FText::FromString(TEXT("상점")), 32);
	GoldText = AddLabel(WidgetTree, Panel, FText::GetEmpty(), 20);
	PotionText = AddLabel(WidgetTree, Panel, FText::FromString(TEXT("회복 포션: 5 골드")), 20);

	BuyPotionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BuyPotionButton"));
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonText->SetText(FText::FromString(TEXT("포션 구매")));
	ButtonText->SetJustification(ETextJustify::Center);
	ButtonText->SetFont(FSlateFontInfo(ButtonText->GetFont().FontObject, 22));
	BuyPotionButton->SetContent(ButtonText);
	BuyPotionButton->OnClicked.AddDynamic(this, &UShopWidget::BuyPotion);

	UVerticalBoxSlot* BuySlot = Panel->AddChildToVerticalBox(BuyPotionButton);
	BuySlot->SetPadding(FMargin(12.f, 12.f));
	BuySlot->SetHorizontalAlignment(HAlign_Fill);

	StatusText = AddLabel(WidgetTree, Panel, FText::FromString(TEXT("P 또는 Esc: 닫기")), 16);
	RefreshDetails();
	return true;
}

void UShopWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshDetails();
}

FReply UShopWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::P || InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseShop();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UShopWidget::BuyPotion()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		return;
	}

	if (Player->Coin < Player->PotionPrice)
	{
		StatusText->SetText(FText::FromString(TEXT("골드가 부족합니다.")));
		return;
	}

	Player->RequestBuyPotion();
	StatusText->SetText(FText::FromString(TEXT("포션을 구매했습니다.")));
}

void UShopWidget::CloseShop()
{
	if (ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn()))
	{
		Player->ToggleShop();
	}
}

void UShopWidget::RefreshDetails()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (!Player || !GoldText || !PotionText)
	{
		return;
	}

	GoldText->SetText(FText::FromString(FString::Printf(TEXT("보유 골드: %d"), Player->Coin)));
	PotionText->SetText(FText::FromString(FString::Printf(TEXT("회복 포션: %d개 (가격: %d 골드)"), Player->PotionCount, Player->PotionPrice)));
}
