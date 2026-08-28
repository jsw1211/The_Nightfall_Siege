#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PartyMemberWidget.generated.h"

class UTextBlock;
class UProgressBar;
class ABaseCharacter;

UCLASS()
class THE_NIGHTFALL_SIEGE_API UPartyMemberWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    virtual void NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* NicknameText;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HPBar;

    void UpdatePartyMember(ABaseCharacter* Character);

private:

    UPROPERTY()
    ABaseCharacter* TrackedCharacter = nullptr;
};