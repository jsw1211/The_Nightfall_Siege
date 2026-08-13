#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DeathHUD.generated.h"

/**
 * Canvas fallback for the death state.  It deliberately does not rely on a
 * UMG Blueprint asset, so the message is always visible on every local client.
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API ADeathHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
