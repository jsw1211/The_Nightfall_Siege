#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BaseCharacter.h"
#include "ItemAnimationStateLibrary.generated.h"

class UAnimInstance;

/** Thread-safe transition-rule access to the replicated held-item state. */
UCLASS()
class THE_NIGHTFALL_SIEGE_API UItemAnimationStateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Animation|Item State", meta = (BlueprintThreadSafe, DefaultToSelf = "AnimInstance"))
	static bool IsItemAnimationState(const UAnimInstance* AnimInstance, EItemAnimationState ExpectedState);
};
