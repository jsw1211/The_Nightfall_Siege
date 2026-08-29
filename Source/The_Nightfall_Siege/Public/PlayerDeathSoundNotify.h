// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "PlayerDeathSoundNotify.generated.h"

/** Plays the normal death sound unless this pawn is only restoring an existing death state. */
UCLASS(const, collapsecategories, meta = (DisplayName = "Player Death Sound"))
class THE_NIGHTFALL_SIEGE_API UPlayerDeathSoundNotify : public UAnimNotify_PlaySound
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
