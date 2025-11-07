/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "InventoryTypes" - Header
 * Note: Common enums & structs for the inventory system (3-slot)
 */

#pragma once
#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable UMETA(DisplayName="Consumable"),
	Weapon     UMETA(DisplayName="Weapon"),
	Tool       UMETA(DisplayName="Tool"),
	KeyItem    UMETA(DisplayName="Key Item"),
	Resource   UMETA(DisplayName="Resource"),
	Quest      UMETA(DisplayName="Quest"),
};

class UItemDefinition;

USTRUCT(BlueprintType)
struct FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	UItemDefinition* def = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="1"))
	int32 quantity = 0;

	bool IsValid() const { return def != nullptr && quantity > 0; }
	void Reset() { def = nullptr; quantity = 0; }
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FItemStack stack;

	bool IsEmpty() const { return !stack.IsValid(); }
	void Clear() { stack.Reset(); }
};
