// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerState.h"
#include "Net/UnrealNetwork.h"



ABasePlayerState::ABasePlayerState()
{
    bReplicates = true;
}

void ABasePlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABasePlayerState, SelectedCharacter);
    DOREPLIFETIME(ABasePlayerState, bReady);

    DOREPLIFETIME(ABasePlayerState, bHasLantern);
    DOREPLIFETIME(ABasePlayerState, bLanternEquipped);

    DOREPLIFETIME(ABasePlayerState, bHasPrism);
    DOREPLIFETIME(ABasePlayerState, bPrismEquipped);

    DOREPLIFETIME(ABasePlayerState, Coin);
    DOREPLIFETIME(ABasePlayerState, SkillPoints);
}

void ABasePlayerState::OnRep_SelectedCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("Rep Character Changed"));
}

void ABasePlayerState::SetReady(bool bNewReady)
{
    bReady = bNewReady;
}

bool ABasePlayerState::IsReady() const
{
    return bReady;
}

void ABasePlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    UE_LOG(LogTemp, Error,
        TEXT("===== CopyProperties ====="));

    UE_LOG(LogTemp, Error,
        TEXT("Copy Character = %d"),
        (int32)SelectedCharacter);

    ABasePlayerState* NewPS =
        Cast<ABasePlayerState>(PlayerState);

    if (!NewPS)
    {
        return;
    }

    NewPS->SelectedCharacter = SelectedCharacter;
    NewPS->bReady = bReady;

    NewPS->bHasLantern = bHasLantern;
    NewPS->bLanternEquipped = bLanternEquipped;

    NewPS->bHasPrism = bHasPrism;
    NewPS->bPrismEquipped = bPrismEquipped;

    NewPS->Coin = Coin;
    NewPS->SkillPoints = SkillPoints;

    UE_LOG(LogTemp, Warning,
        TEXT("Copy Lantern : %d"),
        bHasLantern);

    UE_LOG(LogTemp, Warning,
        TEXT("Copy Prism : %d"),
        bHasPrism);

}

