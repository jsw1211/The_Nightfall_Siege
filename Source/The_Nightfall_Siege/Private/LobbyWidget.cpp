// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/TextBlock.h"
#include "BasePlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Components/Button.h"


void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    PlayerTexts =
    {
        Txt_Player1,
        Txt_Player2,
        Txt_Player3,
        Txt_Player4
    };

    CharacterTexts =
    {
        Txt_Character1,
        Txt_Character2,
        Txt_Character3,
        Txt_Character4
    };

    StatusTexts =
    {
        Txt_Status1,
        Txt_Status2,
        Txt_Status3,
        Txt_Status4
    };

    APlayerController* PC = GetOwningPlayer();

    if (PC)
    {
        Btn_StartGame->SetVisibility(
            PC->HasAuthority()
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
    }

    RefreshLobby();
}

void ULobbyWidget::RefreshLobby()
{
    AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();

    if (!GS)
    {
        return;
    }

    // 먼저 모든 칸을 초기화
    for (int32 i = 0; i < 4; i++)
    {
        if (PlayerTexts.IsValidIndex(i))
        {
            PlayerTexts[i]->SetText(FText::GetEmpty());
        }

        if (CharacterTexts.IsValidIndex(i))
        {
            CharacterTexts[i]->SetText(FText::GetEmpty());
        }

        if (StatusTexts.IsValidIndex(i))
        {
            StatusTexts[i]->SetText(FText::GetEmpty());
        }
    }

    // 접속한 플레이어 수만큼 채우기
    const int32 Count = FMath::Min(GS->PlayerArray.Num(), 4);

    for (int32 i = 0; i < Count; i++)
    {
        ABasePlayerState* PS =
            Cast<ABasePlayerState>(GS->PlayerArray[i]);

        if (!PS)
        {
            continue;
        }

        // 이름
        PlayerTexts[i]->SetText(FText::FromString(PS->GetPlayerName()));

        // 캐릭터
        FString CharacterName;

        switch (PS->SelectedCharacter)
        {
        case ECharacterType::Paladin:
            CharacterName = TEXT("Paladin");
            break;

        case ECharacterType::Warrior:
            CharacterName = TEXT("Warrior");
            break;

        case ECharacterType::Archer:
            CharacterName = TEXT("Archer");
            break;

        default:
            CharacterName = TEXT("Unknown");
            break;
        }

        CharacterTexts[i]->SetText(FText::FromString(CharacterName));

        // Ready
        StatusTexts[i]->SetText(
            FText::FromString(
                PS->IsReady() ? TEXT("Ready") : TEXT("Not Ready")));
    }
}

