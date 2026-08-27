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
class UTexture2D;
class UDragDropOperation;
class FDragDropEvent;
class UDarknessPrismWidget;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

    UPlayerHUDWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
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

    UPROPERTY(meta = (BindWidgetOptional))
    UDarknessPrismWidget* DarknessPrismWidget;

    // A native hard reference makes the vignette part of the cook dependency
    // graph.  Loading it only from NativeTick left packaged builds free to
    // omit the texture even though it was available in the editor.
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> DarknessVignetteTexture;

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

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Text_ATK;

    UFUNCTION(BlueprintCallable)
    void UpdateCoin(int32 Coin);

    void UpdateHealth(float CurrentHP, float MaxHP);
    void UpdateAttackPower(float AttackPower);
};
