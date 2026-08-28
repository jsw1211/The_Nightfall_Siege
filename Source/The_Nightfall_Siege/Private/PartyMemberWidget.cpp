#include "PartyMemberWidget.h"

#include "BaseCharacter.h"
#include "BasePlayerState.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UPartyMemberWidget::UpdatePartyMember(ABaseCharacter* Character)
{
    if (!Character)
    {
        return;
    }

    TrackedCharacter = Character;

    if (NicknameText)
    {
        if (ABasePlayerState* PS =
            Character->GetPlayerState<ABasePlayerState>())
        {
            NicknameText->SetText(
                FText::FromString(PS->GetPlayerName()));
        }
    }

    if (HPBar)
    {
        const float MaxHP = Character->MaxHP;
        const float CurrentHP = Character->CurrentHP;

        const float HPPercent =
            MaxHP > 0.f
            ? CurrentHP / MaxHP
            : 0.f;

        HPBar->SetPercent(
            FMath::Clamp(HPPercent, 0.f, 1.f));
    }
}

void UPartyMemberWidget::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!TrackedCharacter || !HPBar)
    {
        return;
    }

    if (TrackedCharacter->IsDead())
    {
        HPBar->SetPercent(0.f);
        return;
    }

    const float MaxHP = TrackedCharacter->MaxHP;
    const float CurrentHP = TrackedCharacter->CurrentHP;

    const float HPPercent =
        MaxHP > 0.f
        ? CurrentHP / MaxHP
        : 0.f;

    HPBar->SetPercent(
        FMath::Clamp(HPPercent, 0.f, 1.f));
}