#include "ItemAnimationStateLibrary.h"

#include "Animation/AnimInstance.h"

bool UItemAnimationStateLibrary::IsItemAnimationState(
	const UAnimInstance* AnimInstance,
	EItemAnimationState ExpectedState)
{
	const ABaseCharacter* Character = AnimInstance
		? Cast<ABaseCharacter>(AnimInstance->TryGetPawnOwner())
		: nullptr;

	return Character && Character->ItemAnimationState == ExpectedState;
}
