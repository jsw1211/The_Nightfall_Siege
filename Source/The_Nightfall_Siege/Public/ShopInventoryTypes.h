#pragma once

#include "CoreMinimal.h"
#include "ShopInventoryTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EShopItemType : uint8
{
	HealPotion UMETA(DisplayName = "Heal Potion"),
	HPPotion UMETA(DisplayName = "HP Potion"),
	AttackPotion UMETA(DisplayName = "Attack Potion")
};

USTRUCT(BlueprintType)
struct FShopInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EShopItemType ItemType = EShopItemType::HealPotion;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 1;
};
