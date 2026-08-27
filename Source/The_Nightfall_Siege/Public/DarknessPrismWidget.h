#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DarknessPrismWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UDarknessPrismWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    virtual void NativeConstruct() override;

    void UpdatePrismStatus();

    UPROPERTY(meta = (BindWidget))
    UImage* GuideImage;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText;
};