/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemUseEffect" - Header
 * Note: Pluggable effect executed when an item is used (Blueprintable)
 */

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemUseEffect.generated.h"

class UInventoryComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class ETHERIA_API UItemUseEffect : public UObject
{
	GENERATED_BODY()
public:
	// True if item used (ex: potion), or false (ex: weapon equipped)
	UFUNCTION(BlueprintNativeEvent, Category="Item|Use")
	bool ApplyEffect(AActor* User, UInventoryComponent* Inventory, int32 SlotIndex);
	virtual bool ApplyEffect_Implementation(AActor* User, UInventoryComponent* Inventory, int32 SlotIndex);
};
