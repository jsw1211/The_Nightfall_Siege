// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/TextBlock.h"
#include "BasePlayerState.h"
#include "GameFramework/GameStateBase.h"


void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshLobby();
}

void ULobbyWidget::RefreshLobby()
{
    AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();

    if (!GS)
    {
        return;
    }

    if (GS->PlayerArray.Num() == 0)
    {
        return;
    }

    ABasePlayerState* PS =
        Cast<ABasePlayerState>(GS->PlayerArray[0]);

    if (!PS)
    {
        return;
    }

    if (Txt_Player1)
    {
        Txt_Player1->SetText(FText::FromString(PS->GetPlayerName()));
        if (Txt_Character1)
        {
            switch (PS->SelectedCharacter)
            {
            case ECharacterType::Paladin:
                Txt_Character1->SetText(FText::FromString("Paladin"));
                break;

            case ECharacterType::Warrior:
                Txt_Character1->SetText(FText::FromString("Warrior"));
                break;

            case ECharacterType::Archer:
                Txt_Character1->SetText(FText::FromString("Archer"));
                break;
            }
        }
    }

    if (GS->PlayerArray.Num() > 1)
    {
        ABasePlayerState* PS2 =
            Cast<ABasePlayerState>(GS->PlayerArray[1]);

        if (PS2 && Txt_Player2)
        {
            Txt_Player2->SetText(
                FText::FromString(PS2->GetPlayerName()));
            if (Txt_Character2)
            {
                switch (PS2->SelectedCharacter)
                {
                case ECharacterType::Paladin:
                    Txt_Character2->SetText(FText::FromString("Paladin"));
                    break;

                case ECharacterType::Warrior:
                    Txt_Character2->SetText(FText::FromString("Warrior"));
                    break;

                case ECharacterType::Archer:
                    Txt_Character2->SetText(FText::FromString("Archer"));
                    break;
                }
            }
        }
    }
}

