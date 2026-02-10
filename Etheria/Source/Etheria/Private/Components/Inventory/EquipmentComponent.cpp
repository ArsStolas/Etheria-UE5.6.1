/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EquipmentComponent" - Source
 */
#include "Components/Inventory/EquipmentComponent.h"

#include "Components/Combat/CombatComponent.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Components/Inventory/InventoryTypes.h"
#include "Data/Items/ItemDefinition.h"
#include "Data/Weapons/WeaponData.h"

#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
    Combat    = GetOwner() ? GetOwner()->FindComponentByClass<UCombatComponent>()    : nullptr;

    CacheHandMeshComponents();

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

void UEquipmentComponent::CacheHandMeshComponents()
{
    CachedRightHand.Reset();
    CachedLeftHand.Reset();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    TArray<UActorComponent*> Components;
    Owner->GetComponents(Components);

    for (UActorComponent* C : Components)
    {
        if (!C) continue;

        if (!CachedRightHand.IsValid() && C->GetFName() == RightHandComponentName)
        {
            CachedRightHand = Cast<UMeshComponent>(C);
        }
        else if (!CachedLeftHand.IsValid() && C->GetFName() == LeftHandComponentName)
        {
            CachedLeftHand = Cast<UMeshComponent>(C);
        }

        if (CachedRightHand.IsValid() && CachedLeftHand.IsValid())
        {
            break;
        }
    }

#if !UE_BUILD_SHIPPING
    if (bAutoApplyWeaponVisuals)
    {
        if (!CachedRightHand.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] RightHand component '%s' not found or not a UMeshComponent."), *RightHandComponentName.ToString());
        }
        if (!CachedLeftHand.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[EquipmentComponent] LeftHand component '%s' not found or not a UMeshComponent."), *LeftHandComponentName.ToString());
        }
    }
#endif
}

static void ClearAnyMesh(UMeshComponent* Target)
{
    if (!Target) return;

    if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Target))
    {
        SMC->SetStaticMesh(nullptr);
    }
    else if (USkeletalMeshComponent* SKC = Cast<USkeletalMeshComponent>(Target))
    {
        SKC->SetSkeletalMesh(nullptr);
    }
}

static bool ApplyVisualToMeshComponent(UMeshComponent* Target, const FWeaponHandVisual& Visual)
{
    if (!Target) return false;

    // We only consider the visual "applied" if it matches the component type.
    if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Target))
    {
        SMC->SetStaticMesh(Visual.StaticMesh);
        return (Visual.StaticMesh != nullptr);
    }

    if (USkeletalMeshComponent* SKC = Cast<USkeletalMeshComponent>(Target))
    {
        SKC->SetSkeletalMesh(Visual.SkeletalMesh);
        return (Visual.SkeletalMesh != nullptr);
    }

    return false;
}

void UEquipmentComponent::ApplyWeaponVisuals(const UWeaponData* DataToApply)
{
    if (!bAutoApplyWeaponVisuals) return;

    // If components were renamed or created later (construction script), allow a lazy refresh.
    if (!CachedRightHand.IsValid() || !CachedLeftHand.IsValid())
    {
        CacheHandMeshComponents();
    }

    UMeshComponent* RightComp = CachedRightHand.Get();
    UMeshComponent* LeftComp  = CachedLeftHand.Get();

    // Clear previous meshes first to avoid leftovers when the new weapon only fills one hand.
    ClearAnyMesh(RightComp);
    ClearAnyMesh(LeftComp);

    bool bRightHasMesh = false;
    bool bLeftHasMesh  = false;

    if (DataToApply)
    {
        bRightHasMesh = ApplyVisualToMeshComponent(RightComp, DataToApply->RightHand);
        bLeftHasMesh  = ApplyVisualToMeshComponent(LeftComp,  DataToApply->LeftHand);

        if (RightComp && DataToApply->RightHand.bApplyRelativeTransform)
        {
            RightComp->SetRelativeTransform(DataToApply->RightHand.RelativeTransform);
        }
        if (LeftComp && DataToApply->LeftHand.bApplyRelativeTransform)
        {
            LeftComp->SetRelativeTransform(DataToApply->LeftHand.RelativeTransform);
        }
    }

    if (bHideHandsWhenEmpty)
    {
        if (RightComp)
        {
            RightComp->SetHiddenInGame(!bRightHasMesh, true);
            RightComp->SetVisibility(bRightHasMesh, true);
        }
        if (LeftComp)
        {
            LeftComp->SetHiddenInGame(!bLeftHasMesh, true);
            LeftComp->SetVisibility(bLeftHasMesh, true);
        }
    }
}

void UEquipmentComponent::RefreshFromActiveSlot()
{
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

    // 1) Combat profile
    if (Combat.IsValid())
    {
        Combat->SetWeaponData(DataToApply);
    }

    // 2) Visuals (hands)
    ApplyWeaponVisuals(DataToApply);
}

void UEquipmentComponent::HandleActiveSlotChanged(int32 /*NewIndex*/)
{
    RefreshFromActiveSlot();
}

void UEquipmentComponent::HandleSlotContentChanged(int32 /*SlotIndex*/, const FItemStack& /*NewStack*/)
{
    RefreshFromActiveSlot();
}
