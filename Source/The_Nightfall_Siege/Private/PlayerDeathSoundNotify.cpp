// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerDeathSoundNotify.h"

#include "BaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UPlayerDeathSoundNotify::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	const ABaseCharacter* Character = MeshComp
		? Cast<ABaseCharacter>(MeshComp->GetOwner())
		: nullptr;
	if (Character && Character->ShouldSuppressRestoredDeathSound())
	{
		return;
	}

	Super::Notify(MeshComp, Animation, EventReference);
}
