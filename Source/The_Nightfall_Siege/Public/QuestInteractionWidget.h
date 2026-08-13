#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestInteractionWidget.generated.h"

// World-space interaction label. UMG supports the imported Korean FontFace,
// unlike TextRenderComponent's legacy offline-font path.
UCLASS()
class THE_NIGHTFALL_SIEGE_API UQuestInteractionWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    void BuildLayout();
};
