#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestGiver.generated.h"

class ABaseCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWidgetComponent;

// Place BP_QuestGiver (or this native actor) in the village.  Its default cube
// is intentional: artists can replace DummyMesh with any NPC mesh later.
UCLASS()
class THE_NIGHTFALL_SIEGE_API AQuestGiver : public AActor
{
    GENERATED_BODY()

public:
    AQuestGiver();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void Interact(ABaseCharacter* Player);

    // NPC1 is the default shopkeeper.  Other quest-giver Blueprints can turn
    // this off in their Details panel without changing player code.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    bool bIsShopkeeper = true;

    // Only the player who started the active dialogue can advance it or make
    // the quest decision. The server validates both the owner and session.
    void AdvanceDialogue(ABaseCharacter* Player, int32 DialogueSessionId);
    void ResolveQuestDecision(ABaseCharacter* Player, int32 DialogueSessionId, bool bAccepted);
    bool CanInteractWith(const ABaseCharacter* Player) const;

    // These can be authored in BP_QuestGiver; add or remove lines freely.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Dialogue")
    FText SpeakerName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Dialogue", meta = (MultiLine = "true"))
    TArray<FText> DialogueLines;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Quest")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    UStaticMeshComponent* DummyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Quest")
    USphereComponent* InteractionRange;

    UPROPERTY(VisibleAnywhere, Category = "Quest")
    UTextRenderComponent* InteractionText;

    UPROPERTY(VisibleAnywhere, Category = "Quest")
    UWidgetComponent* InteractionWidget;

    UFUNCTION()
    void OnRangeBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnRangeEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void OnDialogueOwnerDestroyed(AActor* DestroyedActor);

private:
    void BroadcastDialogueOpened();
    void BroadcastDialogueState();
    void FinishActiveDialogue(bool bAccepted, const FText& ResultMessage);
    void CancelActiveDialogue();

    UPROPERTY(Transient)
    ABaseCharacter* ActiveDialogueOwner = nullptr;

    UPROPERTY(Transient)
    TArray<FText> ActiveDialogueLines;

    int32 DialogueSessionCounter = 0;
    int32 ActiveDialogueSessionId = 0;
    int32 ActiveDialoguePage = 0;
    bool bActiveDialogueRequiresDecision = false;
    bool bShowingQuestChoice = false;
};
