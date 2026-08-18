#include "PauseMenuWidget.h"

#include "BaseController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"

void UPauseMenuWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyKoreanButtonFont();
}

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
	ApplyKoreanButtonFont();

	ResumeButton = Cast<UButton>(GetWidgetFromName(TEXT("PauseResumeButton")));
	ExitButton = Cast<UButton>(GetWidgetFromName(TEXT("PauseExitButton")));
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::ResumeGame);
		ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::ResumeGame);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::ExitGame);
		ExitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::ExitGame);
	}
}

void UPauseMenuWidget::ApplyKoreanButtonFont()
{
	UFont* KoreanFont = LoadObject<UFont>(nullptr,
		TEXT("/Game/Asset/UI/Font/NOTOSANSKR-VF_Font.NOTOSANSKR-VF_Font"));
	if (!KoreanFont)
	{
		return;
	}

	const FSlateFontInfo ButtonFont(KoreanFont, 48, FName(TEXT("Regular")));
	if (UTextBlock* ResumeText = Cast<UTextBlock>(GetWidgetFromName(TEXT("PauseResumeButton_Text"))))
	{
		ResumeText->SetText(FText::FromString(TEXT("게임 재개")));
		ResumeText->SetFont(ButtonFont);
	}
	if (UTextBlock* ExitText = Cast<UTextBlock>(GetWidgetFromName(TEXT("PauseExitButton_Text"))))
	{
		ExitText->SetText(FText::FromString(TEXT("게임 종료")));
		ExitText->SetFont(ButtonFont);
	}
}

void UPauseMenuWidget::ResumeGame()
{
    if (ABaseController* Controller = GetOwningPlayer<ABaseController>())
    {
        Controller->ResumePausedGame();
    }
}

void UPauseMenuWidget::ExitGame()
{
    if (ABaseController* Controller = GetOwningPlayer<ABaseController>())
    {
        Controller->ExitPausedGame();
    }
}
