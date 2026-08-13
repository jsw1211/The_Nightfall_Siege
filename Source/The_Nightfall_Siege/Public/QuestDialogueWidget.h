#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestDialogueWidget.generated.h"

class AQuestGiver;
class UButton;
class UTextBlock;

UCLASS(Blueprintable)
class THE_NIGHTFALL_SIEGE_API UQuestDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Quest Dialogue")
    void ConfigureDialogue(AQuestGiver* InQuestGiver, const TArray<FText>& InLines, const FText& InSpeakerName, bool bInRequiresQuestDecision);

    UFUNCTION(BlueprintCallable, Category = "Quest Dialogue")
    void AdvanceDialogue();

    UFUNCTION(BlueprintCallable, Category = "Quest Dialogue")
    void AcceptQuest();

    UFUNCTION(BlueprintCallable, Category = "Quest Dialogue")
    void DeclineQuest();

    UFUNCTION(BlueprintPure, Category = "Quest Dialogue")
    FText GetCurrentDialogueLine() const;

    UFUNCTION(BlueprintPure, Category = "Quest Dialogue")
    int32 GetCurrentDialoguePage() const { return CurrentPage + 1; }

    UFUNCTION(BlueprintPure, Category = "Quest Dialogue")
    int32 GetDialoguePageCount() const { return DialogueLines.Num(); }

    UFUNCTION(BlueprintPure, Category = "Quest Dialogue")
    bool IsShowingQuestChoice() const { return bShowingChoice; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Quest Dialogue")
    void OnDialoguePageChanged(const FText& InSpeakerName, const FText& DialogueLine, int32 Page, int32 PageCount);

    UFUNCTION(BlueprintImplementableEvent, Category = "Quest Dialogue")
    void OnQuestChoiceShown();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Quest Dialogue")
    UTextBlock* SpeakerText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Quest Dialogue")
    UTextBlock* DialogueText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Quest Dialogue")
    UTextBlock* PageText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Quest Dialogue")
    UButton* AcceptButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Quest Dialogue")
    UButton* DeclineButton = nullptr;

private:
    void RefreshDialogueView();
    void BuildFallbackLayout();
    void SubmitDecision(bool bAccepted);
    UFUNCTION() void HandleAcceptClicked();
    UFUNCTION() void HandleDeclineClicked();

    UPROPERTY(Transient) AQuestGiver* QuestGiver = nullptr;
    UPROPERTY(Transient) TArray<FText> DialogueLines;
    UPROPERTY(Transient) FText SpeakerName;
    int32 CurrentPage = 0;
    bool bShowingChoice = false;
    bool bRequiresQuestDecision = true;
};
