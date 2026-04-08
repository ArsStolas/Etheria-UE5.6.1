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
class UMeshComponent;
struct FItemStack;

/**
 * Lightweight bridge between Inventory and Combat: the active slot dictates the weapon profile.
 *
 * Also responsible for swapping the "weapon-in-hand" meshes on the owning character.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UEquipmentComponent();

	// Fallback when no weapon is selected (unarmed/punches)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	UWeaponData* DefaultUnarmedData = nullptr;

	/**
	 * Name of the mesh component on the owning character used to display the equipped weapon in the right hand.
	 * Default expects your BP_PlayerCharacter to contain a component named "WeaponRHand".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visual")
	FName RightHandComponentName = TEXT("WeaponRHand");

	/**
	 * Name of the mesh component on the owning character used to display the equipped weapon in the left hand.
	 * Default expects your BP_PlayerCharacter to contain a component named "WeaponLHand".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visual")
	FName LeftHandComponentName = TEXT("WeaponLHand");

	/** If true, this component will auto-swap the hand meshes using UWeaponData::RightHand / LeftHand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visual")
	bool bAutoApplyWeaponVisuals = true;

	/** If true and a hand has no mesh from the equipped weapon, the hand component will be hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visual", meta=(EditCondition="bAutoApplyWeaponVisuals"))
	bool bHideHandsWhenEmpty = true;

	/**
	 * Pour les IA / boss sans inventaire : si défini, cette arme est appliquée au CombatComponent
	 * et aux visuels (au lieu de lire l'équipement depuis l'inventaire).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|AI")
	TObjectPtr<UWeaponData> OverrideWeaponDataForAI = nullptr;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void SetOverrideWeaponDataForAI(UWeaponData* InWeaponData);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RefreshFromActiveSlot(); // Call when active slot changes or its content changes.

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UInventoryComponent> Inventory;
	TWeakObjectPtr<UCombatComponent>    Combat;

	TWeakObjectPtr<UMeshComponent> CachedRightHand;
	TWeakObjectPtr<UMeshComponent> CachedLeftHand;

	UItemDefinition* GetItemDefInActiveSlot() const;

	void CacheHandMeshComponents();
	void ApplyWeaponVisuals(const UWeaponData* DataToApply);

	UFUNCTION() void HandleActiveSlotChanged(int32 NewIndex);
	UFUNCTION() void HandleSlotContentChanged(int32 SlotIndex, const FItemStack& NewStack);
};
