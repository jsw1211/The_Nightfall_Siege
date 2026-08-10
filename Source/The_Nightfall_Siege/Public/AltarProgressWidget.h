#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AltarProgressWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UAltarProgressWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void StartProgress(float InDuration);
    void StopProgress();

private:
    void BuildLayout();

    UPROPERTY()
    UProgressBar* ProgressBar = nullptr;

    UPROPERTY()
    UTextBlock* Label = nullptr;

    float Duration = 3.f;
    float Elapsed = 0.f;
    bool bRunning = false;
};
