#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

UCLASS()
class THE_NIGHTFALL_SIEGE_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void ResumeGame();

	UFUNCTION()
	void ExitGame();

	void ApplyKoreanButtonFont();

	UPROPERTY()
	class UButton* ResumeButton = nullptr;

	UPROPERTY()
	class UButton* ExitButton = nullptr;

};
