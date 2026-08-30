#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PausedOverlayWidget.generated.h"

/** Non-interactive pause notice shown to players who did not request the pause. */
UCLASS()
class THE_NIGHTFALL_SIEGE_API UPausedOverlayWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildOverlay();
};
