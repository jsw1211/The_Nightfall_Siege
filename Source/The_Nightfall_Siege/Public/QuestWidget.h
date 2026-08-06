#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UQuestWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ShowObjective(const FText& Objective);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    void BuildQuestLayout();

    UPROPERTY(Transient)
    UTextBlock* QuestObjectiveText;

    UPROPERTY(Transient)
    UTextBlock* QuestProgressText;
};
