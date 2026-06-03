// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"
#include "BaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"

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
}