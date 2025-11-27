/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemDefinition" - Header
 * Note: Data asset describing an item (safe UDataAsset variant)
 */

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/Inventory/InventoryTypes.h"
#include "ItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;
class UItemUseEffect;
class UWeaponData;

UCLASS(BlueprintType)
class ETHERIA_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item") FName itemId = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item") FText displayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item") EItemType type = EItemType::Consumable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stack") bool bStackable = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stack", meta=(EditCondition="bStackable", ClampMin="1")) int32 maxStack = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|UI") UTexture2D* icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World") UStaticMesh* worldMesh = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Item|Use") UItemUseEffect* useEffect = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Use") bool bBroadcastUseEventIfNoEffect = true; // If true, Event Dispatcher in Inventory
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Weapon") UWeaponData* weaponData = nullptr;
};
