#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "NightfallLocalPlayer.generated.h"

/**
 * Supplies the nickname entered in the multiplayer dialog to Unreal's normal
 * login flow. The game mode then initializes and replicates PlayerState's
 * PlayerName before the lobby roster is drawn.
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API UNightfallLocalPlayer : public ULocalPlayer
{
    GENERATED_BODY()

public:
    virtual FString GetNickname() const override;
};
