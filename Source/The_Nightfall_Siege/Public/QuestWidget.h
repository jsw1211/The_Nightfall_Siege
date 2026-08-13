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

    // Keep the HUD from invalidating its Slate layout when quest state is unchanged.
    FText CachedObjective;
    int32 CachedQuestStage = INDEX_NONE;
    int32 CachedClearedDungeonCount = INDEX_NONE;
    int32 CachedDungeonMonsterKillCount = INDEX_NONE;
    int32 CachedDungeonMonsterTotalCount = INDEX_NONE;
};
