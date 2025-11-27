/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "InventoryComponent" - Header
 * Note: 3-slot player inventory with use/drop/select and events (solo-safe)
 */

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;
class AItemPickup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotChanged, int32, SlotIndex, const FItemStack&, NewStack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedIndexChanged, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUsed, const UItemDefinition*, ItemDef, bool, bConsumed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFull, const UItemDefinition*, FailedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestUseSelected, const FItemStack&, Stack);

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInventoryComponent();

	// ---- Config
	static constexpr int32 kMaxSlots = 3;

protected:
	UPROPERTY(VisibleAnywhere, Category="Inventory") TArray<FInventorySlot> slots;
	UPROPERTY(VisibleAnywhere, Category="Inventory") int32 selectedIndex = 0;

	// Classe actor to drop items (BP child autorized)
	UPROPERTY(EditDefaultsOnly, Category="Inventory|Drop") TSubclassOf<AItemPickup> pickupClass;

public:
	// ---- Events (UI & Gameplay can bind)
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events") FOnInventorySlotChanged OnInventorySlotChanged;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events") FOnSelectedIndexChanged OnSelectedIndexChanged;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events") FOnItemUsed OnItemUsed;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events") FOnInventoryFull OnInventoryFull;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events") FOnRequestUseSelected OnRequestUseSelected;

	// ---- Drop
	UPROPERTY(EditAnywhere, Category="Inventory|Drop") float DropForwardDistance = 100.f;
	UPROPERTY(EditAnywhere, Category="Inventory|Drop") float DropUpOffset = 20.f;
	UPROPERTY(EditAnywhere, Category="Inventory|Drop") float DropDownProbe = 120.f;

	// ---- Debug
	UPROPERTY(EditAnywhere, Category="Inventory|Drop") bool bDropDrawDebug = false;

	
	// ---- Core API
	UFUNCTION(BlueprintCallable, Category="Inventory") bool TryAddItem(UItemDefinition* Def, int32 Quantity=1);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool TryAddPickup(AItemPickup* Pickup);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool DropSelected(bool bDropAll=true, int32 Amount=1);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool UseSelected();
	UFUNCTION(BlueprintPure, Category="Inventory") int32 GetSelectedIndex() const { return selectedIndex; }
	UFUNCTION(BlueprintCallable, Category="Inventory") void SelectNext();
	UFUNCTION(BlueprintCallable, Category="Inventory") void SelectPrevious();
	UFUNCTION(BlueprintPure, Category="Inventory") const TArray<FInventorySlot>& GetSlots() const { return slots; }
	UFUNCTION(BlueprintPure, Category="Inventory") UItemDefinition* GetItemDefInSlot(int32 SlotIndex) const;

protected:
	virtual void BeginPlay() override;
	
	bool CanStackInto(const FItemStack& Existing, const UItemDefinition* Def, int32& InOutQuantity, int32& OutAddable) const;
	bool AddToExistingStack(const UItemDefinition* Def, int32& InOutQuantity);
	bool AddToEmptySlot(const UItemDefinition* Def, int32& InOutQuantity);

	void ClampSelectedToNonEmpty();
	void BroadcastSlot(int32 Slot);
	bool SpawnDrop(const FItemStack& StackToDrop);
};
