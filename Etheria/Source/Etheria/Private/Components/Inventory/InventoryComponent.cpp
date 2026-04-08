/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "InventoryComponent" - Source
 * Note: 3-slot player inventory with use/drop/select and events
 */

#include "Components/Inventory/InventoryComponent.h"
#include "Items/ItemPickup.h"
#include "Data/Items/ItemDefinition.h"
#include "Data/Items/ItemUseEffect.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	slots.SetNum(kMaxSlots);
	pickupClass = AItemPickup::StaticClass();
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	selectedIndex = 0;
}

bool UInventoryComponent::CanStackInto(const FItemStack& Existing, const UItemDefinition* Def, int32& InOutQuantity, int32& OutAddable) const
{
	OutAddable = 0;
	if (!Existing.IsValid() || !Def) return false;
	if (Existing.def != Def) return false;
	if (!Def->bStackable) return false;

	const int32 capacity = FMath::Max(1, Def->maxStack);
	const int32 canAdd = FMath::Clamp(capacity - Existing.quantity, 0, InOutQuantity);
	OutAddable = canAdd;
	return canAdd > 0;
}

bool UInventoryComponent::AddToExistingStack(const UItemDefinition* Def, int32& InOutQuantity)
{
	for (int32 i=0;i<slots.Num();++i)
	{
		int32 addable=0;
		if (CanStackInto(slots[i].stack, Def, InOutQuantity, addable))
		{
			slots[i].stack.quantity += addable;
			InOutQuantity -= addable;
			BroadcastSlot(i);
			if (InOutQuantity <= 0) return true;
		}
	}
	return InOutQuantity <= 0;
}

bool UInventoryComponent::AddToEmptySlot(const UItemDefinition* Def, int32& InOutQuantity)
{
	for (int32 i=0;i<slots.Num();++i)
	{
		if (slots[i].IsEmpty())
		{
			const int32 capacity = (Def->bStackable ? FMath::Max(1, Def->maxStack) : 1);
			const int32 toPlace = FMath::Clamp(InOutQuantity, 1, capacity);
			slots[i].stack.def = const_cast<UItemDefinition*>(Def);
			slots[i].stack.quantity = toPlace;
			InOutQuantity -= toPlace;
			BroadcastSlot(i);
			OnItemAdded.Broadcast(Def, i);
			return true;
		}
	}
	return false;
}

bool UInventoryComponent::TryAddItem(UItemDefinition* Def, int32 Quantity)
{
	if (!Def || Quantity <= 0) return false;

	int32 remaining = Quantity;

	// Stack if possible
	if (Def->bStackable) AddToExistingStack(Def, remaining);

	// Slot Empty
	if (remaining > 0 && !AddToEmptySlot(Def, remaining))
	{
		OnInventoryFull.Broadcast(Def);
		return false;
	}

	// Inventory too small
	if (remaining > 0)
	{
		if (!AddToEmptySlot(Def, remaining))
		{
			OnInventoryFull.Broadcast(Def);
			return false;
		}
	}

	return true;
}

bool UInventoryComponent::TryAddPickup(AItemPickup* Pickup)
{
	if (!Pickup || !Pickup->itemDef) return false;
	return TryAddItem(Pickup->itemDef, FMath::Max(1, Pickup->quantity));
}

bool UInventoryComponent::SpawnDrop(const FItemStack& StackToDrop)
{
    if (!StackToDrop.IsValid()) return false;

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return false;
	
    UClass* SpawnClass = pickupClass ? *pickupClass : AItemPickup::StaticClass();
    if (!SpawnClass)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: SpawnClass is null."));
        return false;
    }

    // Compute a drop point in front of the character mesh (ground-aligned)
    FVector BaseLoc = Owner->GetActorLocation();
    FRotator FacingRot = Owner->GetActorRotation();
    FVector Forward = Owner->GetActorForwardVector();

    if (const ACharacter* Char = Cast<ACharacter>(Owner))
    {
        if (const USkeletalMeshComponent* Mesh = Char->GetMesh())
        {
            BaseLoc = Mesh->GetComponentLocation();
            FacingRot = Char->GetActorRotation();
            Forward = Char->GetActorForwardVector();
        }
    }

    const FVector Desired = BaseLoc + Forward * DropForwardDistance + FVector(0.f, 0.f, DropUpOffset);

    // Ground probe
    FHitResult GroundHit;
    const FVector TraceStart = Desired + FVector(0.f, 0.f, DropDownProbe * 0.5f);
    const FVector TraceEnd   = Desired - FVector(0.f, 0.f, DropDownProbe);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InventoryDropProbe), false, Owner);

    const bool bHitGround = World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params);
    FVector FinalLoc = bHitGround ? (GroundHit.ImpactPoint + FVector(0,0,10.f)) : Desired;
    FRotator FinalRot = FacingRot;

    if (bDropDrawDebug)
    {
        DrawDebugLine(World, TraceStart, TraceEnd, FColor::Green, false, 2.f, 0, 1.5f);
        DrawDebugSphere(World, FinalLoc, 8.f, 12, FColor::Yellow, false, 2.f);
    }

    // Deferred spawn to set item data before construction visuals
    const FTransform SpawnTM(FinalRot, FinalLoc);
    AItemPickup* Spawned = World->SpawnActorDeferred<AItemPickup>(
        SpawnClass,
        SpawnTM,
        Owner,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );
    if (!Spawned) return false;

    // Set data before FinishSpawning so OnConstruction can build the visuals correctly
    Spawned->itemDef = StackToDrop.def;
    Spawned->quantity = StackToDrop.quantity;

    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTM);
    return true;
}

bool UInventoryComponent::DropSelected(bool bDropAll, int32 Amount)
{
	if (!slots.IsValidIndex(selectedIndex) || slots[selectedIndex].IsEmpty()) return false;

	const int32 toDrop = bDropAll ? slots[selectedIndex].stack.quantity : FMath::Clamp(Amount, 1, slots[selectedIndex].stack.quantity);

	FItemStack dropStack = slots[selectedIndex].stack;
	dropStack.quantity = toDrop;

	if (SpawnDrop(dropStack))
	{
		slots[selectedIndex].stack.quantity -= toDrop;
		if (slots[selectedIndex].stack.quantity <= 0) slots[selectedIndex].Clear();
		BroadcastSlot(selectedIndex);
		OnItemRemoved.Broadcast(dropStack.def, selectedIndex);
		ClampSelectedToNonEmpty();
		return true;
	}
	return false;
}

bool UInventoryComponent::UseSelected()
{
	if (!slots.IsValidIndex(selectedIndex) || slots[selectedIndex].IsEmpty()) return false;

	FItemStack& st = slots[selectedIndex].stack;
	bool bConsumed = false;

	// Pluggable (Data -> useEffect)
	if (st.def && st.def->useEffect)
	{
		bConsumed = st.def->useEffect->ApplyEffect(GetOwner(), this, selectedIndex);
	}
	else if (st.def && st.def->bBroadcastUseEventIfNoEffect)
	{
		// No effect -> broadcast for external systems (weapons, potions, etc...)
		OnRequestUseSelected.Broadcast(st);
	}

	// Use if possible
	if (bConsumed)
	{
		st.quantity -= 1;
		if (st.quantity <= 0) slots[selectedIndex].Clear();
		BroadcastSlot(selectedIndex);
		ClampSelectedToNonEmpty();
	}

	OnItemUsed.Broadcast(st.def, bConsumed);
	return true;
}

void UInventoryComponent::SelectNext()
{
	if (slots.Num() == 0) return;
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, TEXT("SELECT NEXT CALLED ONCE!"));
	}
	int32 start = selectedIndex;
	for (int32 i=0;i<slots.Num();++i)
	{
		selectedIndex = (selectedIndex + 1) % slots.Num();
		if (!slots[selectedIndex].IsEmpty()) { OnSelectedIndexChanged.Broadcast(selectedIndex); return; }
	}
	selectedIndex = start;
}

void UInventoryComponent::SelectPrevious()
{
	if (slots.Num() == 0) return;
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, TEXT("SELECT PREVIOUS CALLED ONCE!"));
	}
	int32 start = selectedIndex;
	for (int32 i=0;i<slots.Num();++i)
	{
		selectedIndex = (selectedIndex - 1 + slots.Num()) % slots.Num();
		if (!slots[selectedIndex].IsEmpty()) { OnSelectedIndexChanged.Broadcast(selectedIndex); return; }
	}
	selectedIndex = start;
}

void UInventoryComponent::ClampSelectedToNonEmpty()
{
	if (slots.Num() == 0) return;
	if (!slots[selectedIndex].IsEmpty()) return;
	for (int32 i=0;i<slots.Num();++i)
	{
		if (!slots[i].IsEmpty())
		{
			selectedIndex = i;
			OnSelectedIndexChanged.Broadcast(selectedIndex);
			return;
		}
	}
	selectedIndex = 0;
	//OnSelectedIndexChanged.Broadcast(selectedIndex);
}

void UInventoryComponent::BroadcastSlot(int32 Slot)
{
	if (slots.IsValidIndex(Slot))
	{
		OnInventorySlotChanged.Broadcast(Slot, slots[Slot].stack);
	}
}

UItemDefinition* UInventoryComponent::GetItemDefInSlot(int32 SlotIndex) const
{
	if (!slots.IsValidIndex(SlotIndex)) return nullptr;
	const FItemStack& St = slots[SlotIndex].stack;
	return (St.IsValid() ? St.def : nullptr);
}

