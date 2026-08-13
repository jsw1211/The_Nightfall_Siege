#include "DeathHUD.h"

#include "BaseCharacter.h"
#include "BaseController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ADeathHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !PlayerOwner)
    {
        return;
    }

    ABaseCharacter* Character = Cast<ABaseCharacter>(PlayerOwner->GetPawn());
    ABaseController* Controller = Cast<ABaseController>(PlayerOwner);
    UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
    if (!Font || !Controller)
    {
        return;
    }

    const bool bGameClear = Controller->IsGameClearVisible();
    if (!bGameClear && (!Character || !Character->IsDead())) return;

    const FText Title = FText::FromString(bGameClear ? TEXT("GAME CLEAR!") : TEXT("YOU DIED"));

    float TextWidth = 0.f;
    float TextHeight = 0.f;
    Canvas->StrLen(Font, Title.ToString(), TextWidth, TextHeight);

    FCanvasTextItem TextItem(
        FVector2D((Canvas->ClipX - TextWidth) * 0.5f, Canvas->ClipY * 0.38f),
        Title,
        Font,
        bGameClear ? FLinearColor(1.f, 0.78f, 0.05f, 1.f) : FLinearColor(0.9f, 0.02f, 0.02f, 1.f));
    TextItem.EnableShadow(FLinearColor::Black);
    Canvas->DrawItem(TextItem);

    if (bGameClear || Controller->IsRetryAvailable())
    {
        const FVector2D ButtonPosition(Canvas->ClipX * 0.5f - 150.f, Canvas->ClipY * 0.5f - 10.f);
        FCanvasTileItem ButtonBackground(ButtonPosition, FVector2D(300.f, 75.f), FLinearColor(0.06f, 0.06f, 0.06f, 0.9f));
        ButtonBackground.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(ButtonBackground);

        const FText ButtonText = FText::FromString(bGameClear ? TEXT("EXIT GAME") : TEXT("RETRY"));
        Canvas->StrLen(Font, ButtonText.ToString(), TextWidth, TextHeight);
        FCanvasTextItem ButtonLabel(FVector2D(Canvas->ClipX * 0.5f - TextWidth * 0.5f, Canvas->ClipY * 0.5f + 10.f), ButtonText, Font, FLinearColor::White);
        ButtonLabel.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(ButtonLabel);
    }
}
