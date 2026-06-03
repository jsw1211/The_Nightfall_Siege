// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"
#include "BaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UE_LOG(LogTemp, Warning, TEXT("Tick Start"));

    if (!HPBar)
    {
        UE_LOG(LogTemp, Error, TEXT("HPBar NULL"));
        return;
    }

    if (!HPText)
    {
        UE_LOG(LogTemp, Error, TEXT("HPText NULL"));
        return;
    }

    ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("Player NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("All Valid"));

    HPBar->SetPercent(Player->CurrentHP / Player->MaxHP);

    HPText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Player->CurrentHP, Player->MaxHP)));

    if (Portrait && Player->PortraitTexture)
    {
        Portrait->SetBrushFromTexture(Player->PortraitTexture);
    }

    if (SkillQImage && Player->QSkillIcon)
    {
        SkillQImage->SetBrushFromTexture(Player->QSkillIcon);
    }

    if (SkillWImage && Player->WSkillIcon)
    {
        SkillWImage->SetBrushFromTexture(Player->WSkillIcon);
    }

    if (SkillEImage && Player->ESkillIcon)
    {
        SkillEImage->SetBrushFromTexture(Player->ESkillIcon);
    }

    if (SkillRImage && Player->RSkillIcon)
    {
        SkillRImage->SetBrushFromTexture(Player->RSkillIcon);
    }

    SkillQCooldown->SetText(Player->QRemainingCooldown > 0.f ? FText::FromString(FString::Printf(TEXT("%.1f"), Player->QRemainingCooldown)) : FText::GetEmpty());

    SkillWCooldown->SetText(Player->WRemainingCooldown > 0.f ? FText::FromString(FString::Printf(TEXT("%.1f"), Player->WRemainingCooldown)) : FText::GetEmpty());

    SkillECooldown->SetText(Player->ERemainingCooldown > 0.f ? FText::FromString(FString::Printf(TEXT("%.1f"), Player->ERemainingCooldown)) : FText::GetEmpty());

    SkillRCooldown->SetText(Player->RRemainingCooldown > 0.f ? FText::FromString(FString::Printf(TEXT("%.1f"), Player->RRemainingCooldown)) : FText::GetEmpty());

    if (Player->QRemainingCooldown > 0.f)
    {
        SkillQImage->SetOpacity(0.3f);
    }
    else
    {
        SkillQImage->SetOpacity(1.f);
    }

    if (Player->WRemainingCooldown > 0.f)
    {
        SkillWImage->SetOpacity(0.3f);
    }
    else
    {
        SkillWImage->SetOpacity(1.f);
    }

    if (Player->ERemainingCooldown > 0.f)
    {
        SkillEImage->SetOpacity(0.3f);
    }
    else
    {
        SkillEImage->SetOpacity(1.f);
    }

    if (Player->RRemainingCooldown > 0.f)
    {
        SkillRImage->SetOpacity(0.3f);
    }
    else
    {
        SkillRImage->SetOpacity(1.f);
    }
}