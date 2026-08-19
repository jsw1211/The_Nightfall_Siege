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
    void ShowConnectionError();
    bool IsConnecting() const { return bIsConnecting; }

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION()
    void OnOkClicked();

    UFUNCTION()
    void OnAddressCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    bool GetValidatedAddress(FString& OutAddress) const;
    void ShowError(const FText& Message);
    void SetConnecting(bool bConnecting);

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> AddressTextBox;

    UPROPERTY(Transient)
    TObjectPtr<UButton> OkButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ErrorText;

    bool bIsConnecting = false;
};
