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
