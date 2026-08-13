#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AltarInteractionWidget.generated.h"

// Screen-space labels stay upright relative to the player's camera.
UCLASS()
class THE_NIGHTFALL_SIEGE_API UAltarInteractionWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
