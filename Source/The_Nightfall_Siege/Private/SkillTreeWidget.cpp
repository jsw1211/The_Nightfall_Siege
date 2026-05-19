// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"
#include "Components/Button.h"
#include "BaseCharacter.h"
#include "Kismet/GameplayStatics.h"

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateSkillPointText();

	UpdateSkillLevelText();

	if (Btn_Q_Damage)
	{
		Btn_Q_Damage->OnClicked.AddDynamic(
			this,
			&USkillTreeWidget::OnClick_Q_Damage
		);
	}
}

void USkillTreeWidget::OnClick_Q_Damage()
{
	ABaseCharacter* Player =
		Cast<ABaseCharacter>(
			UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
		);

	if (!Player)
		return;

	FSkillUpgradeData Upgrade;

	Upgrade.SkillType = ESkillType::Q;
	Upgrade.UpgradeType = EUpgradeType::Damage;
	Upgrade.Value = 10.f;

	Player->UpgradeSkill(Upgrade);

	UpdateSkillPointText();
	UpdateSkillLevelText();

	UE_LOG(LogTemp, Warning, TEXT("Q Damage Upgrade"));
}

void USkillTreeWidget::UpdateSkillPointText()
{
	ABaseCharacter* Player =
		Cast<ABaseCharacter>(
			UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
		);

	if (!Player)
		return;

	if (Txt_SkillPoint)
	{
		Txt_SkillPoint->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("Skill Point : %d"),
					Player->SkillPoints
				)
			)
		);
	}
}

void USkillTreeWidget::UpdateSkillLevelText()
{
	ABaseCharacter* Player =
		Cast<ABaseCharacter>(
			UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
		);

	if (!Player)
		return;

	if (Txt_Q_Level)
	{
		int32 QLevel =
			Player->SkillLevels[ESkillType::Q];

		Txt_Q_Level->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("Q Level : %d"),
					QLevel
				)
			)
		);
	}
}

