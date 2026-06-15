// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */

class UTextBlock;

UCLASS()
class THE_NIGHTFALL_SIEGE_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    virtual void NativeConstruct() override;

    void RefreshLobby();

protected:

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Player1;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Character1;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Status1;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Player2;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Character2;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Status2;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Player3;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Character3;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Status3;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Player4;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Character4;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Status4;

    TArray<UTextBlock*> PlayerTexts;
    TArray<UTextBlock*> CharacterTexts;
    TArray<UTextBlock*> StatusTexts;

};
