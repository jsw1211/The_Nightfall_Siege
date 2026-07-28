#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UButton;
class UTextBlock;

// A self-contained fallback shop UI. A Blueprint can replace it later through
// BaseCharacter::ShopWidgetClass without changing the purchase logic.
UCLASS()
class THE_NIGHTFALL_SIEGE_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	UFUNCTION()
	void BuyPotion();

	void CloseShop();
	void RefreshDetails();

	UPROPERTY()
	UTextBlock* GoldText;

	UPROPERTY()
	UTextBlock* PotionText;

	UPROPERTY()
	UTextBlock* StatusText;

	UPROPERTY()
	UButton* BuyPotionButton;
};
