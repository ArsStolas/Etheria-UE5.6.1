/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemUsableInterface" - Header
 * Note: Alternate/useful hook for Actor-based items
 */

#pragma once
#include "UObject/Interface.h"
#include "ItemUsableInterface.generated.h"

UINTERFACE(Blueprintable)
class UItemUsableInterface : public UInterface
{
	GENERATED_BODY()
};

class IItemUsableInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Use")
	bool UseItem(AActor* User);
};
