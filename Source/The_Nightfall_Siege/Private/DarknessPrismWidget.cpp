#include "DarknessPrismWidget.h"

#include "BasePlayerState.h"
#include "GameFramework/GameStateBase.h"

void UDarknessPrismWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UDarknessPrismWidget::UpdatePrismStatus()
{
    if (!StatusText)
    {
        return;
    }

    FString StatusString;

    AGameStateBase* GameState = GetWorld()
        ? GetWorld()->GetGameState<AGameStateBase>()
        : nullptr;

    if (!GameState)
    {
        return;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        ABasePlayerState* PS = Cast<ABasePlayerState>(PlayerState);

        if (!PS)
        {
            continue;
        }

        StatusString += FString::Printf(
            TEXT("%s : %s\n"),
            *PS->GetPlayerName(),
            PS->bPrismCleansePressed
            ? TEXT("완료")
            : TEXT("미완료"));
    }

    StatusText->SetText(FText::FromString(StatusString));
}