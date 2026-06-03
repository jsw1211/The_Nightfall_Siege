// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

/**
 * 
 */

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HPBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HPText;

    UPROPERTY(meta = (BindWidget))
    UImage* Portrait;
	
};
