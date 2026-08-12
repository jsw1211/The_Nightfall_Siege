#include "InventoryItemSlotWidget.h"

#include "BaseCharacter.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

bool UInventoryItemSlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (!WidgetTree->RootWidget)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBorder"));
		Border->SetPadding(FMargin(6.f));
		Border->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.9f));
		WidgetTree->RootWidget = Border;

		UVerticalBox* Contents = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotContents"));
		Border->SetContent(Contents);

		ItemImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemImage"));
		ItemImage->SetDesiredSizeOverride(FVector2D(72.f, 72.f));
		Contents->AddChildToVerticalBox(ItemImage)->SetHorizontalAlignment(HAlign_Center);

		ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemName"));
		ItemNameText->SetJustification(ETextJustify::Center);
		Contents->AddChildToVerticalBox(ItemNameText)->SetHorizontalAlignment(HAlign_Fill);
	}

	return true;
}

void UInventoryItemSlotWidget::Configure(ABaseCharacter* InOwner, int32 InIndex, const FText& InName, UTexture2D* InIcon)
{
	OwnerCharacter = InOwner;
	ItemIndex = InIndex;
	bContainsItem = true;
	ItemImage->SetBrushFromTexture(InIcon);
	ItemNameText->SetText(InName);
}

void UInventoryItemSlotWidget::ConfigureEmpty()
{
	OwnerCharacter = nullptr;
	ItemIndex = INDEX_NONE;
	bContainsItem = false;
	ItemImage->SetBrushFromTexture(nullptr);
	ItemNameText->SetText(FText::GetEmpty());
}

FReply UInventoryItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bContainsItem)
	{
		return FReply::Unhandled();
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply UInventoryItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bContainsItem && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OwnerCharacter)
	{
		OwnerCharacter->UsePurchasedItemAtIndex(ItemIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!bContainsItem)
	{
		return;
	}

	UDragDropOperation* Operation = NewObject<UDragDropOperation>(this);
	Operation->Payload = this;
	Operation->DefaultDragVisual = this;
	Operation->Pivot = EDragPivot::MouseDown;
	OutOperation = Operation;
}

bool UInventoryItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UInventoryItemSlotWidget* SourceSlot = InOperation ? Cast<UInventoryItemSlotWidget>(InOperation->Payload) : nullptr;
	if (!bContainsItem || !SourceSlot || !SourceSlot->bContainsItem || SourceSlot == this || SourceSlot->OwnerCharacter != OwnerCharacter)
	{
		return false;
	}

	OwnerCharacter->MovePurchasedItem(SourceSlot->ItemIndex, ItemIndex);
	return true;
}
