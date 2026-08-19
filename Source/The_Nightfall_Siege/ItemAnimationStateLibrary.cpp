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

bool UItemAnimationStateLibrary::IsNotItemAnimationState(
	const UAnimInstance* AnimInstance,
	EItemAnimationState State)
{
	return !IsItemAnimationState(AnimInstance, State);
}

float UItemAnimationStateLibrary::GetPotionPoseWeight(const UAnimInstance* AnimInstance)
{
	return IsItemAnimationState(AnimInstance, EItemAnimationState::PotionUsing) ? 1.0f : 0.0f;
}
