#include "InventoryItemSlotWidget.h"

#include "BaseCharacter.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"

bool UInventoryItemSlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// WBP_Inventory에서 이 위젯을 직접 배치하므로
	// C++에서는 별도의 이미지/텍스트 UI를 생성하지 않는다.
	// RootWidget이 이미 WBP에서 만들어져 있으면 그대로 사용한다.
	if (!WidgetTree->RootWidget)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("SlotInputArea")
		);

		// 보이지 않는 입력 영역
		Border->SetBrushColor(FLinearColor::Transparent);

		WidgetTree->RootWidget = Border;
	}

	return true;
}

void UInventoryItemSlotWidget::Configure(
	ABaseCharacter* InOwner,
	int32 InIndex,
	UTexture2D* InIcon)
{
	OwnerCharacter = InOwner;
	ItemIndex = InIndex;
	bContainsItem = true;
	DragIcon = InIcon;
}

void UInventoryItemSlotWidget::ConfigureEmpty()
{
	OwnerCharacter = nullptr;
	ItemIndex = INDEX_NONE;
	bContainsItem = false;
	DragIcon = nullptr;
}

FReply UInventoryItemSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!bContainsItem)
	{
		return FReply::Unhandled();
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(
		InMouseEvent,
		this,
		EKeys::LeftMouseButton
	).NativeReply;
}

FReply UInventoryItemSlotWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bContainsItem &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		OwnerCharacter)
	{
		OwnerCharacter->UsePurchasedItemAtIndex(ItemIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(
		InGeometry,
		InMouseEvent
	);
}

void UInventoryItemSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!bContainsItem || !DragIcon)
	{
		return;
	}

	UDragDropOperation* Operation =
		NewObject<UDragDropOperation>(this);

	Operation->Payload = this;

	// 드래그 아이콘의 중앙이 마우스 커서에 오도록 설정
	Operation->Pivot = EDragPivot::CenterCenter;

	// 드래그 중 표시할 아이콘
	UImage* DragImage = NewObject<UImage>(this);
	DragImage->SetBrushFromTexture(DragIcon);

	// 현재 슬롯의 실제 크기를 그대로 사용
	const FVector2D SlotSize = InGeometry.GetLocalSize();
	DragImage->SetDesiredSizeOverride(SlotSize);

	Operation->DefaultDragVisual = DragImage;

	OutOperation = Operation;
}

bool UInventoryItemSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UInventoryItemSlotWidget* SourceSlot =
		InOperation
		? Cast<UInventoryItemSlotWidget>(InOperation->Payload)
		: nullptr;

	if (!bContainsItem ||
		!SourceSlot ||
		!SourceSlot->bContainsItem ||
		SourceSlot == this ||
		SourceSlot->OwnerCharacter != OwnerCharacter)
	{
		return false;
	}

	OwnerCharacter->MovePurchasedItem(
		SourceSlot->ItemIndex,
		ItemIndex
	);

	return true;
}