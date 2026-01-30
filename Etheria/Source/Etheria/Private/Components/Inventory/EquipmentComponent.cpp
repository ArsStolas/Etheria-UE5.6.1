/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EquipmentComponent" - Source
 */
#include "Components/Inventory/EquipmentComponent.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Components/Inventory/InventoryTypes.h"
#include "Components/Combat/CombatComponent.h"
#include "Data/Items/ItemDefinition.h"
#include "Data/Weapons/WeaponData.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
    Combat    = GetOwner() ? GetOwner()->FindComponentByClass<UCombatComponent>()    : nullptr;

    if (Inventory.IsValid())
    {
        Inventory->OnSelectedIndexChanged.AddDynamic(this, &UEquipmentComponent::HandleActiveSlotChanged);
        Inventory->OnInventorySlotChanged.AddDynamic(this, &UEquipmentComponent::HandleSlotContentChanged);
    }

    RefreshFromActiveSlot();
}

UItemDefinition* UEquipmentComponent::GetItemDefInActiveSlot() const
{
    if (!Inventory.IsValid()) return nullptr;

    const int32 idx = Inventory->GetSelectedIndex();
    if (idx < 0) return nullptr;
    return Inventory->GetItemDefInSlot(idx);
}

void UEquipmentComponent::RefreshFromActiveSlot()
{
    if (!Combat.IsValid()) return;

    UWeaponData* DataToApply = DefaultUnarmedData;

    if (UItemDefinition* Def = GetItemDefInActiveSlot())
    {
        // Only treat as weapon if explicitly marked and has a weapon profile.
        const bool bIsWeapon = (Def->type == EItemType::Weapon);
        if (bIsWeapon && Def->weaponData)
        {
            DataToApply = Def->weaponData;
        }
    }

    Combat->SetWeaponData(DataToApply);
}

void UEquipmentComponent::HandleActiveSlotChanged(int32 /*NewIndex*/)
{
    RefreshFromActiveSlot();
}

void UEquipmentComponent::HandleSlotContentChanged(int32 /*SlotIndex*/, const FItemStack& /*NewStack*/)
{
    RefreshFromActiveSlot();
}
