#include "IPJoinWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TheNightfallSiegeInstance.h"

namespace
{
    constexpr float DialogWidth = 620.0f;

    bool IsValidIpv4(const FString& Address)
    {
        TArray<FString> Octets;
        Address.ParseIntoArray(Octets, TEXT("."), true);
        if (Octets.Num() != 4)
        {
            return false;
        }

        for (const FString& Octet : Octets)
        {
            int32 Value = 0;
            if (Octet.IsEmpty() || !LexTryParseString(Value, *Octet) || Value < 0 || Value > 255)
            {
                return false;
            }
        }

        return true;
    }
}

void UIPJoinWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (WidgetTree->RootWidget)
    {
        return;
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("IPJoinRoot"));
    WidgetTree->RootWidget = Root;

    UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
    Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.68f));
    Root->AddChildToOverlay(Dimmer);

    UBorder* Dialog = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IPJoinDialog"));
    Dialog->SetBrushColor(FLinearColor(0.035f, 0.045f, 0.075f, 0.98f));
    Dialog->SetPadding(FMargin(34.f));
    UOverlaySlot* DialogSlot = Root->AddChildToOverlay(Dialog);
    DialogSlot->SetHorizontalAlignment(HAlign_Center);
    DialogSlot->SetVerticalAlignment(VAlign_Center);

    USizeBox* DialogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialogSize"));
    DialogSize->SetWidthOverride(DialogWidth);
    Dialog->SetContent(DialogSize);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogContent"));
    DialogSize->SetContent(Content);

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("닉네임 / 서버 IP 입력")));
    TitleText->SetJustification(ETextJustify::Center);
    TitleText->SetFont(FSlateFontInfo(TitleText->GetFont().FontObject, 30));
    Content->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));

    UTextBlock* NicknameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NicknameLabel"));
    NicknameLabel->SetText(FText::FromString(TEXT("닉네임")));
    Content->AddChildToVerticalBox(NicknameLabel)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));

    NicknameTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("Player_Nickname"));
    NicknameTextBox->SetHintText(FText::FromString(TEXT("닉네임 (최대 16자)")));
    NicknameTextBox->SetSelectAllTextOnCommit(true);
    NicknameTextBox->OnTextCommitted.AddDynamic(this, &UIPJoinWidget::OnNicknameCommitted);
    Content->AddChildToVerticalBox(NicknameTextBox)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

    AddressLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddressLabel"));
    AddressLabel->SetText(FText::FromString(TEXT("서버 IP")));
    Content->AddChildToVerticalBox(AddressLabel)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));

    AddressRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AddressRow"));
    Content->AddChildToVerticalBox(AddressRow);

    AddressTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("IP_Address"));
    AddressTextBox->SetHintText(FText::FromString(TEXT("예: 127.0.0.1 또는 192.168.0.10:7777")));
    AddressTextBox->SetText(FText::FromString(TEXT("127.0.0.1")));
    AddressTextBox->SetSelectAllTextOnCommit(true);
    AddressTextBox->OnTextCommitted.AddDynamic(this, &UIPJoinWidget::OnAddressCommitted);
    UHorizontalBoxSlot* AddressSlot = AddressRow->AddChildToHorizontalBox(AddressTextBox);
    AddressSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AddressSlot->SetPadding(FMargin(0.f));

    OkButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_OK"));
    OkButton->OnClicked.AddDynamic(this, &UIPJoinWidget::OnOkClicked);
    OkButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OkText"));
    OkButtonText->SetText(FText::FromString(TEXT("접속")));
    OkButtonText->SetJustification(ETextJustify::Center);
    OkButton->SetContent(OkButtonText);
    UVerticalBoxSlot* OkSlot = Content->AddChildToVerticalBox(OkButton);
    OkSlot->SetHorizontalAlignment(HAlign_Right);
    OkSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

    ErrorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ErrorText"));
    ErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.25f, 0.25f)));
    ErrorText->SetJustification(ETextJustify::Center);
    ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    Content->AddChildToVerticalBox(ErrorText)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
}

void UIPJoinWidget::SetHostMode(bool bInHostMode)
{
    bHostMode = bInHostMode;

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(
            bHostMode ? TEXT("닉네임 입력") : TEXT("닉네임 / 서버 IP 입력")));
    }
    if (OkButtonText)
    {
        OkButtonText->SetText(FText::FromString(bHostMode ? TEXT("서버 열기") : TEXT("접속")));
    }

    const ESlateVisibility AddressVisibility =
        bHostMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
    if (AddressLabel)
    {
        AddressLabel->SetVisibility(AddressVisibility);
    }
    if (AddressRow)
    {
        AddressRow->SetVisibility(AddressVisibility);
    }

    if (const UTheNightfallSiegeInstance* GameInstance =
        GetGameInstance<UTheNightfallSiegeInstance>())
    {
        if (NicknameTextBox && !GameInstance->GetPlayerNickname().IsEmpty())
        {
            NicknameTextBox->SetText(FText::FromString(GameInstance->GetPlayerNickname()));
        }
    }

    if (NicknameTextBox)
    {
        NicknameTextBox->SetKeyboardFocus();
    }
}

void UIPJoinWidget::OnAddressCommitted(const FText&, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        OnOkClicked();
    }
}

void UIPJoinWidget::OnNicknameCommitted(const FText&, ETextCommit::Type CommitMethod)
{
    if (CommitMethod != ETextCommit::OnEnter)
    {
        return;
    }

    if (bHostMode)
    {
        OnOkClicked();
    }
    else if (AddressTextBox)
    {
        AddressTextBox->SetKeyboardFocus();
    }
}

bool UIPJoinWidget::GetValidatedNickname(FString& OutNickname) const
{
    if (!NicknameTextBox)
    {
        return false;
    }

    OutNickname = NicknameTextBox->GetText().ToString().TrimStartAndEnd();
    if (OutNickname.IsEmpty() || OutNickname.Len() > 16)
    {
        return false;
    }

    for (const TCHAR Character : OutNickname)
    {
        if (FChar::IsControl(Character) || Character == TEXT('?') ||
            Character == TEXT('&') || Character == TEXT('=') || Character == TEXT('#'))
        {
            return false;
        }
    }

    return true;
}

bool UIPJoinWidget::GetValidatedAddress(FString& OutAddress) const
{
    if (!AddressTextBox)
    {
        return false;
    }

    OutAddress = AddressTextBox->GetText().ToString().TrimStartAndEnd();
    FString Host = OutAddress;
    FString Port;
    if (OutAddress.Split(TEXT(":"), &Host, &Port, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
    {
        int32 PortNumber = 0;
        if (Port.IsEmpty() || !LexTryParseString(PortNumber, *Port) || PortNumber < 1 || PortNumber > 65535)
        {
            return false;
        }
    }

    return IsValidIpv4(Host);
}

void UIPJoinWidget::OnOkClicked()
{
    FString Nickname;
    if (!GetValidatedNickname(Nickname))
    {
        ShowError(FText::FromString(TEXT("닉네임을 1~16자로 입력해주세요. (?, &, =, # 사용 불가)")));
        if (NicknameTextBox)
        {
            NicknameTextBox->SetKeyboardFocus();
        }
        return;
    }

    FString Address;
    if (!bHostMode && !GetValidatedAddress(Address))
    {
        ShowError(FText::FromString(TEXT("올바른 IP 주소를 입력해주세요.")));
        if (AddressTextBox)
        {
            AddressTextBox->SetKeyboardFocus();
        }
        return;
    }

    UTheNightfallSiegeInstance* GameInstance =
        GetGameInstance<UTheNightfallSiegeInstance>();
    if (!GameInstance)
    {
        ShowConnectionError();
        return;
    }

    // NightfallLocalPlayer returns this value from GetNickname(). Unreal then
    // places it in the login URL and initializes the replicated PlayerState
    // name before the lobby is shown.
    GameInstance->SetPlayerNickname(Nickname);

    if (bHostMode)
    {
        GameInstance->bIsHost = true;
        SetConnecting(true);
        UGameplayStatics::OpenLevel(
            this,
            FName(TEXT("/Game/Level/Lvl_Lobby")),
            true,
            TEXT("listen"));
        return;
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        ShowConnectionError();
        return;
    }

    GameInstance->bIsHost = false;
    SetConnecting(true);
    PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void UIPJoinWidget::ShowConnectionError()
{
    SetConnecting(false);
    ShowError(FText::FromString(TEXT("서버에 연결할 수 없습니다. 다시 입력해주세요.")));
    if (AddressTextBox)
    {
        AddressTextBox->SetKeyboardFocus();
    }
}

void UIPJoinWidget::ShowError(const FText& Message)
{
    if (ErrorText)
    {
        ErrorText->SetText(Message);
        ErrorText->SetVisibility(ESlateVisibility::Visible);
    }
}

void UIPJoinWidget::SetConnecting(bool bConnecting)
{
    bIsConnecting = bConnecting;
    if (OkButton)
    {
        OkButton->SetIsEnabled(!bConnecting);
    }
    if (AddressTextBox)
    {
        AddressTextBox->SetIsReadOnly(bConnecting);
    }
    if (NicknameTextBox)
    {
        NicknameTextBox->SetIsReadOnly(bConnecting);
    }
    if (bConnecting)
    {
        ShowError(FText::FromString(
            bHostMode ? TEXT("로비를 여는 중입니다...") : TEXT("서버에 연결 중입니다...")));
    }
}
