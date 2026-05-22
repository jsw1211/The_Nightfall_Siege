// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
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
	class UButton* Btn_W_Damage;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_E_Damage;
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_R_Damage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_SkillPoint;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_Q_Level;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_W_Level;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_E_Level;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_R_Level;

	UFUNCTION()
	void OnClick_Q_Damage();
	UFUNCTION()
	void OnClick_W_Damage();
	UFUNCTION()
	void OnClick_E_Damage();
	UFUNCTION()
	void OnClick_R_Damage();

	void UpdateSkillPointText();

	void UpdateSkillLevelText();

	UPROPERTY(meta = (BindWidget))
	class UImage* Img_Q;

	UPROPERTY(meta = (BindWidget))
	class UImage* Img_W;

	UPROPERTY(meta = (BindWidget))
	class UImage* Img_E;

	UPROPERTY(meta = (BindWidget))
	class UImage* Img_R;

	void UpdateSkillIcons();
};
