#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemSlotWidget.generated.h"

class ABaseCharacter;
class UBorder;
class UImage;
class UTextBlock;

// A runtime slot used inside WBP_Inventory's existing GridPanel.  It owns its
// drag/drop behavior so the visual Blueprint does not need graph wiring.
UCLASS()
class THE_NIGHTFALL_SIEGE_API UInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(ABaseCharacter* InOwner, int32 InIndex, const FText& InName, UTexture2D* InIcon);
	int32 GetItemIndex() const { return ItemIndex; }
	ABaseCharacter* GetOwnerCharacter() const { return OwnerCharacter; }

protected:
	virtual bool Initialize() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UImage> ItemImage;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemNameText;

	int32 ItemIndex = INDEX_NONE;
};
