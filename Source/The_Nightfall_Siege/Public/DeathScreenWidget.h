#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathScreenWidget.generated.h"

class UButton;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UDeathScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    void SetRetryAvailable(bool bAvailable);

private:
    void BuildDeathScreen();

    UFUNCTION()
    void HandleRetryClicked();

    UPROPERTY()
    UButton* RetryButton = nullptr;
};
