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

	UpdateSkillIcons();

	if (Btn_Q_Damage)
	{
		Btn_Q_Damage->OnClicked.AddDynamic(
			this,
			&USkillTreeWidget::OnClick_Q_Damage
		);
	}
	if (Btn_W_Damage)
	{
		Btn_W_Damage->OnClicked.AddDynamic(
			this,
			&USkillTreeWidget::OnClick_W_Damage
		);
	}
	if (Btn_E_Damage)
	{
		Btn_E_Damage->OnClicked.AddDynamic(
			this,
			&USkillTreeWidget::OnClick_E_Damage
		);
	}
	if (Btn_R_Damage)
	{
		Btn_R_Damage->OnClicked.AddDynamic(
			this,
			&USkillTreeWidget::OnClick_R_Damage
		);
	}
}

void USkillTreeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		return;
	}

	const int32 QLevel = Player->SkillLevels.FindRef(ESkillType::Q);
	const int32 WLevel = Player->SkillLevels.FindRef(ESkillType::W);
	const int32 ELevel = Player->SkillLevels.FindRef(ESkillType::E);
	const int32 RLevel = Player->SkillLevels.FindRef(ESkillType::R);
	if (CachedSkillPoints != Player->SkillPoints)
	{
		CachedSkillPoints = Player->SkillPoints;
		UpdateSkillPointText();
	}
	if (CachedQLevel != QLevel || CachedWLevel != WLevel
		|| CachedELevel != ELevel || CachedRLevel != RLevel)
	{
		CachedQLevel = QLevel;
		CachedWLevel = WLevel;
		CachedELevel = ELevel;
		CachedRLevel = RLevel;
		UpdateSkillLevelText();
	}
}

void USkillTreeWidget::OnClick_Q_Damage()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

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

void USkillTreeWidget::OnClick_W_Damage()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

	if (!Player)
		return;

	FSkillUpgradeData Upgrade;

	Upgrade.SkillType = ESkillType::W;
	Upgrade.UpgradeType = EUpgradeType::Damage;
	Upgrade.Value = 10.f;

	Player->UpgradeSkill(Upgrade);

	UpdateSkillPointText();
	UpdateSkillLevelText();

	UE_LOG(LogTemp, Warning, TEXT("W Damage Upgrade"));
}

void USkillTreeWidget::OnClick_E_Damage()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

	if (!Player)
		return;

	FSkillUpgradeData Upgrade;

	Upgrade.SkillType = ESkillType::E;
	Upgrade.UpgradeType = EUpgradeType::Damage;
	Upgrade.Value = 10.f;

	Player->UpgradeSkill(Upgrade);

	UpdateSkillPointText();
	UpdateSkillLevelText();

	UE_LOG(LogTemp, Warning, TEXT("E Damage Upgrade"));
}

void USkillTreeWidget::OnClick_R_Damage()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

	if (!Player)
		return;

	FSkillUpgradeData Upgrade;

	Upgrade.SkillType = ESkillType::R;
	Upgrade.UpgradeType = EUpgradeType::Damage;
	Upgrade.Value = 10.f;

	Player->UpgradeSkill(Upgrade);

	UpdateSkillPointText();
	UpdateSkillLevelText();

	UE_LOG(LogTemp, Warning, TEXT("R Damage Upgrade"));
}

void USkillTreeWidget::UpdateSkillPointText()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

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
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

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

		if (QLevel >= 4)
		{
			Btn_Q_Damage->SetIsEnabled(false);
		}
		else
		{
			Btn_Q_Damage->SetIsEnabled(true);
		}
	}

	if (Txt_W_Level)
	{
		int32 WLevel =
			Player->SkillLevels[ESkillType::W];

		Txt_W_Level->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("W Level : %d"),
					WLevel
				)
			)
		);

		if (WLevel >= 4)
		{
			Btn_W_Damage->SetIsEnabled(false);
		}
		else
		{
			Btn_W_Damage->SetIsEnabled(true);
		}
	}

	if (Txt_E_Level)
	{
		int32 ELevel =
			Player->SkillLevels[ESkillType::E];

		Txt_E_Level->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("E Level : %d"),
					ELevel
				)
			)
		);

		if (ELevel >= 4)
		{
			Btn_E_Damage->SetIsEnabled(false);
		}
		else
		{
			Btn_E_Damage->SetIsEnabled(true);
		}
	}

	if (Txt_R_Level)
	{
		int32 RLevel =
			Player->SkillLevels[ESkillType::R];

		Txt_R_Level->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("R Level : %d"),
					RLevel
				)
			)
		);

		if (RLevel >= 4)
		{
			Btn_R_Damage->SetIsEnabled(false);
		}
		else
		{
			Btn_R_Damage->SetIsEnabled(true);
		}
	}
}

void USkillTreeWidget::UpdateSkillIcons()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

	if (!Player)
		return;

	if (Img_Q && Player->QSkillIcon)
	{
		Img_Q->SetBrushFromTexture(Player->QSkillIcon);
	}

	if (Img_W && Player->WSkillIcon)
	{
		Img_W->SetBrushFromTexture(Player->WSkillIcon);
	}

	if (Img_E && Player->ESkillIcon)
	{
		Img_E->SetBrushFromTexture(Player->ESkillIcon);
	}

	if (Img_R && Player->RSkillIcon)
	{
		Img_R->SetBrushFromTexture(Player->RSkillIcon);
	}
}

