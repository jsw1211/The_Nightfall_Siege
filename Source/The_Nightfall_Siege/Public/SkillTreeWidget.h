// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "SkillTreeWidget.generated.h"

/**
 * 
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API USkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

public:

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Q_Damage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_SkillPoint;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_Q_Level;

	UFUNCTION()
	void OnClick_Q_Damage();

	void UpdateSkillPointText();

	void UpdateSkillLevelText();
};
