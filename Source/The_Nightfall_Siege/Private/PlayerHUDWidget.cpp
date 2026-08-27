// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"
#include "BaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryItemSlotWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "DarknessPrismWidget.h"
#include "BasePlayerState.h"
#include "GameFramework/GameStateBase.h"

UPlayerHUDWidget::UPlayerHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Store this on the widget CDO rather than loading by path at runtime.
    // This keeps the texture in the packaged asset registry and pak/IoStore.
    static ConstructorHelpers::FObjectFinder<UTexture2D> DarknessVignetteAsset(
        TEXT("/Game/BP_Character/Textures/T_DarknessVignette.T_DarknessVignette"));

    if (DarknessVignetteAsset.Succeeded())
    {
        DarknessVignetteTexture = DarknessVignetteAsset.Object;
    }
}

void UPlayerHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

bool UPlayerHUDWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryItemSlotWidget* SourceSlot = InOperation ? Cast<UInventoryItemSlotWidget>(InOperation->Payload) : nullptr;
    ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
    if (!SourceSlot || !Player || SourceSlot->GetOwnerCharacter() != Player || !Item4)
    {
        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
    }

    if (Item4->GetCachedGeometry().IsUnderLocation(InDragDropEvent.GetScreenSpacePosition()))
    {
        Player->AssignPurchasedItemToSlot4(SourceSlot->GetItemIndex());
        return true;
    }

    return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());

    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("Player NULL"));
        return;
    }

    UpdateAttackPower(Player->GetAttackPower());

    // Apply the authored UMG overlay directly.  Its PNG has a transparent
    // centre and a black falloff at the edges, so UI and the pawn stay clear
    // while the surrounding screen darkens.
    if (DarknessOverlay)
    {
        if (DarknessVignetteTexture)
        {
            DarknessOverlay->SetBrushFromTexture(DarknessVignetteTexture, true);
        }

        DarknessOverlay->SetColorAndOpacity(FLinearColor::White);
        DarknessOverlay->SetVisibility(Player->bDarknessDebuff
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Hidden);
    }

    if (DarknessPrismWidget)
    {
        const bool bDebuffActive = Player->bDarknessDebuff;

        DarknessPrismWidget->SetVisibility(
            bDebuffActive
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Hidden);

        if (bDebuffActive)
        {
            DarknessPrismWidget->UpdatePrismStatus();
        }
    }

    UpdateHealth(Player->CurrentHP, Player->MaxHP);

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

    if (Item1)
    {
        // Assign the empty brush as well, otherwise UMG keeps rendering the
        // lantern image after it is placed on an altar.
        Item1->SetBrushFromTexture(Player->Slot1Icon);
    }

    if (Item2)
    {
        // Always assign the brush, including nullptr.  Otherwise, when the
        // last potion is consumed and the empty-slot texture is unset, UMG
        // keeps rendering the previous potion image.
        Item2->SetBrushFromTexture(Player->Slot2Icon);
    }

    if (Item3)
    {
        Item3->SetBrushFromTexture(Player->Slot3Icon);
    }

    if (Item4)
    {
        // Item4 is a valid drop target for HP/attack potions from the inventory.
        Item4->SetVisibility(ESlateVisibility::Visible);
        Item4->SetBrushFromTexture(Player->Slot4Icon);
    }

    UpdateCoin(Player->Coin);
}

void UPlayerHUDWidget::UpdateCoin(int32 Coin)
{
    if (!CoinText)
    {
        return;
    }

    CoinText->SetText(FText::AsNumber(Coin));
}

void UPlayerHUDWidget::UpdateHealth(float CurrentHP, float MaxHP)
{
    if (HPBar)
    {
        HPBar->SetPercent(MaxHP > 0.f ? CurrentHP / MaxHP : 0.f);
    }
    if (HPText)
    {
        HPText->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f / %.0f"), CurrentHP, MaxHP)));
    }
}

void UPlayerHUDWidget::UpdateAttackPower(float AttackPower)
{
    if (Text_ATK)
    {
        Text_ATK->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f"), AttackPower)));
    }
}

