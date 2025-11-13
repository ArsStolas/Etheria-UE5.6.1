/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EquipmentComponent" - Header
 */
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UCombatComponent;
class UItemDefinition;
class UWeaponData;
struct FItemStack;

/**
 * Lightweight bridge between Inventory and Combat: the active slot dictates the weapon profile.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UEquipmentComponent();

	// Fallback when no weapon is selected (unarmed/punches)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment") UWeaponData* DefaultUnarmedData = nullptr;
	UFUNCTION(BlueprintCallable, Category="Equipment") void RefreshFromActiveSlot(); // Call when active slot changes or its content changes.

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UInventoryComponent> Inventory;
	TWeakObjectPtr<UCombatComponent>    Combat;

	UItemDefinition* GetItemDefInActiveSlot() const;

	UFUNCTION() void HandleActiveSlotChanged(int32 NewIndex);
	UFUNCTION() void HandleSlotContentChanged(int32 SlotIndex, const FItemStack& NewStack);
};
