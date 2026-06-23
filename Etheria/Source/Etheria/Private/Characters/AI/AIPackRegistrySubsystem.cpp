/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIPackRegistrySubsystem - Source"
 */

#include "Characters/AI/AIPackRegistrySubsystem.h"
#include "Characters/AI/BaseAICharacter.h"

void UAIPackRegistrySubsystem::Register(ABaseAICharacter* AI, FName PackID)
{
	if (!AI || PackID == NAME_None) return;
	Packs.FindOrAdd(PackID).AddUnique(AI);
}

void UAIPackRegistrySubsystem::Unregister(ABaseAICharacter* AI, FName PackID)
{
	if (PackID == NAME_None) return;
	if (TArray<TWeakObjectPtr<ABaseAICharacter>>* Arr = Packs.Find(PackID))
	{
		Arr->RemoveAll([AI](const TWeakObjectPtr<ABaseAICharacter>& W) { return !W.IsValid() || W.Get() == AI; });
		if (Arr->Num() == 0) Packs.Remove(PackID);
	}
}

void UAIPackRegistrySubsystem::GetPackMembers(FName PackID, TArray<ABaseAICharacter*>& OutMembers) const
{
	OutMembers.Reset();
	if (PackID == NAME_None) return;
	if (const TArray<TWeakObjectPtr<ABaseAICharacter>>* Arr = Packs.Find(PackID))
	{
		OutMembers.Reserve(Arr->Num());
		for (const TWeakObjectPtr<ABaseAICharacter>& W : *Arr)
			if (ABaseAICharacter* M = W.Get())
				OutMembers.Add(M);
	}
}
