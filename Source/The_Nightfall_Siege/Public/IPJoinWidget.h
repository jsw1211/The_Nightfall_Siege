#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IPJoinWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

/**
 * Native implementation used by the WBP_IP join dialog.  Keeping the
 * connection UI here means it is also available when the menu is loaded in a
 * packaged build without relying on a hard-coded console command.
 */
UCLASS()
class THE_NIGHTFALL_SIEGE_API UIPJoinWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Configures the dialog for either opening a listen server or joining one. */
    void SetHostMode(bool bInHostMode);

    void ShowConnectionError();
    bool IsConnecting() const { return bIsConnecting; }

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION()
    void OnOkClicked();

    UFUNCTION()
    void OnAddressCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnNicknameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    bool GetValidatedAddress(FString& OutAddress) const;
    bool GetValidatedNickname(FString& OutNickname) const;
    void ShowError(const FText& Message);
    void SetConnecting(bool bConnecting);

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> NicknameTextBox;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> AddressTextBox;

    UPROPERTY(Transient)
    TObjectPtr<class UHorizontalBox> AddressRow;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AddressLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> OkButtonText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> OkButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ErrorText;

    bool bHostMode = false;
    bool bIsConnecting = false;
};
