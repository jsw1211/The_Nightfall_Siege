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
class UDragDropOperation;
class FDragDropEvent;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HPBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HPText;

    UPROPERTY(meta = (BindWidget))
    UImage* Portrait;

    // The original Blueprint overlay is a solid black full-screen image.
    // Keep it bound so the native HUD can replace it with a camera vignette.
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* DarknessOverlay;

    UPROPERTY(meta = (BindWidget))
    UImage* SkillQImage;

    UPROPERTY(meta = (BindWidget))
    UImage* SkillWImage;

    UPROPERTY(meta = (BindWidget))
    UImage* SkillEImage;

    UPROPERTY(meta = (BindWidget))
    UImage* SkillRImage;
	
    UPROPERTY(meta = (BindWidget))
    UTextBlock* SkillQCooldown;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SkillWCooldown;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SkillECooldown;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SkillRCooldown;

    UPROPERTY(meta = (BindWidget))
    UImage* Item1;

    UPROPERTY(meta = (BindWidget))
    UImage* Item2;

    UPROPERTY(meta = (BindWidget))
    UImage* Item3;

    UPROPERTY(meta = (BindWidget))
    UImage* Item4;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CoinText;

    UFUNCTION(BlueprintCallable)
    void UpdateCoin(int32 Coin);
};
