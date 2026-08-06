#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestWidget.generated.h"

class UTextBlock;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UQuestWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ShowObjective(const FText& Objective);

protected:
    UPROPERTY(Transient)
    UTextBlock* QuestObjectiveText;
};
