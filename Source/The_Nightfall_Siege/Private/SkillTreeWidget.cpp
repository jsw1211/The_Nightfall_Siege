// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"
#include "Components/Button.h"
#include "BaseCharacter.h"
#include "Kismet/GameplayStatics.h"

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

	UE_LOG(LogTemp, Warning, TEXT("Q Damage Upgrade"));
}

