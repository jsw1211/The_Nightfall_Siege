// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/TextBlock.h"


void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshLobby();
}

void ULobbyWidget::RefreshLobby()
{
    if (Txt_Player1)
    {
        Txt_Player1->SetText(FText::FromString(TEXT("TEST")));
    }
}

