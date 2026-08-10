#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestGiver.generated.h"

class ABaseCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

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

    // Called only by the accepting player's server RPC.
    void ResolveQuestDecision(ABaseCharacter* Player, bool bAccepted);
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

    UFUNCTION()
    void OnRangeBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnRangeEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
