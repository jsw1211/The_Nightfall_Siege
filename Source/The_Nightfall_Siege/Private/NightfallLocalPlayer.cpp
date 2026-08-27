#include "NightfallLocalPlayer.h"

#include "TheNightfallSiegeInstance.h"

FString UNightfallLocalPlayer::GetNickname() const
{
    if (const UTheNightfallSiegeInstance* GameInstance =
        Cast<UTheNightfallSiegeInstance>(GetGameInstance()))
    {
        if (!GameInstance->GetPlayerNickname().IsEmpty())
        {
            return GameInstance->GetPlayerNickname();
        }
    }

    return Super::GetNickname();
}
