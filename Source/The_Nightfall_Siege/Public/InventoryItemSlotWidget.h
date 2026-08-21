#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemSlotWidget.generated.h"

class ABaseCharacter;
class UBorder;

// WBP_Inventory에 미리 배치된 슬롯의 입력/드래그/드롭을 담당하는 위젯.
// 실제 아이템 이미지와 이름은 WBP에서 직접 배치하고,
// 이 위젯은 슬롯의 입력 영역 역할만 담당한다.
UCLASS()
class THE_NIGHTFALL_SIEGE_API UInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(
		ABaseCharacter* InOwner,
		int32 InIndex,
		UTexture2D* InIcon);

	// 비어 있는 슬롯으로 설정
	void ConfigureEmpty();

	int32 GetItemIndex() const { return ItemIndex; }
	ABaseCharacter* GetOwnerCharacter() const { return OwnerCharacter; }

protected:
	virtual bool Initialize() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;

	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> OwnerCharacter;

	int32 ItemIndex = INDEX_NONE;

	bool bContainsItem = false;

	UPROPERTY()
	TObjectPtr<UTexture2D> DragIcon;

};