/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemUseEffect" - Source
 * Note: Pluggable effect executed when an item is used (Blueprintable)
 */

#include "Data/Items/ItemUseEffect.h"
#include "Components/Inventory/InventoryComponent.h"

bool UItemUseEffect::ApplyEffect_Implementation(AActor* User, UInventoryComponent* Inventory, int32 SlotIndex)
{
	// Default: no effect, not consumed
	return false;
}
