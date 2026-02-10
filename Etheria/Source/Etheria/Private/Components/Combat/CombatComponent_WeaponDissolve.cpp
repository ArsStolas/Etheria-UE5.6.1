/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCombatComponent" - Source (Weapon Dissolve segment)
 */

#include "Components/Combat/CombatComponent.h"

#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

void UCombatComponent::WeaponDissolve_RefreshCaches()
{
	WeaponMeshesCached.Reset();
	WeaponMIDsCached.Reset();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 1) Explicit references (recommended)
	for (const FComponentReference& Ref : WeaponMeshReferences)
	{
		if (UActorComponent* C = Ref.GetComponent(Owner))
		{
			if (UMeshComponent* M = Cast<UMeshComponent>(C))
			{
				WeaponMeshesCached.AddUnique(M);
			}
		}
	}

	// 2) Optional auto-collect by tag on owner
	if (bWeaponAutoCollectByTag && WeaponAutoCollectTag != NAME_None)
	{
		TArray<UActorComponent*> Tagged = Owner->GetComponentsByTag(UMeshComponent::StaticClass(), WeaponAutoCollectTag);
		for (UActorComponent* C : Tagged)
		{
			if (UMeshComponent* M = Cast<UMeshComponent>(C))
			{
				WeaponMeshesCached.AddUnique(M);
			}
		}
	}

	// Build MIDs for all materials
	for (UMeshComponent* Mesh : WeaponMeshesCached)
	{
		if (!IsValid(Mesh)) continue;

		const int32 MatCount = Mesh->GetNumMaterials();
		for (int32 Index = 0; Index < MatCount; ++Index)
		{
			if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(Index))
			{
				WeaponMIDsCached.Add(MID);
			}
		}
	}
}

void UCombatComponent::WeaponDissolve_ApplyParams(float Dissolve, float ColorOpacity, float StrengthVN)
{
	// Apply to cached MIDs (timeline update calls this each frame)
	for (UMaterialInstanceDynamic* MID : WeaponMIDsCached)
	{
		if (!MID) continue;

		MID->SetScalarParameterValue(DissolveParamName, Dissolve);
		MID->SetScalarParameterValue(ColorOpacityParamName, ColorOpacity);
		MID->SetScalarParameterValue(StrengthVNParamName, StrengthVN);
	}
}

void UCombatComponent::WeaponDissolve_SetMeshesHidden(bool bHidden)
{
	for (UMeshComponent* Mesh : WeaponMeshesCached)
	{
		if (!IsValid(Mesh)) continue;

		Mesh->SetHiddenInGame(bHidden, true);
		Mesh->SetVisibility(!bHidden, true);

		// Optionnel : shadows off quand hidden (souvent mieux)
		Mesh->SetCastShadow(!bHidden);
	}
}

void UCombatComponent::WeaponDissolve_RequestShow_Internal()
{
	if (!bWeaponRequestedVisible)
	{
		bWeaponRequestedVisible = true;
		WeaponDissolve_SetMeshesHidden(false); // important: visible before show timeline
		OnWeaponShowRequested.Broadcast();
	}
}

void UCombatComponent::WeaponDissolve_RequestHide_Internal()
{
	if (bWeaponRequestedVisible)
	{
		bWeaponRequestedVisible = false;
		OnWeaponHideRequested.Broadcast(); // BP will Reverse timeline, and on Finished call SetMeshesHidden(true)
	}
}

void UCombatComponent::WeaponDissolve_PingActivity()
{
	// Any weapon/combat activity keeps weapons visible and resets hide delay
	WeaponDissolve_RequestShow_Internal();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeaponAutoHideTimer);
		World->GetTimerManager().SetTimer(
			WeaponAutoHideTimer,
			this,
			&UCombatComponent::WeaponDissolve_OnAutoHideTimer,
			FMath::Max(WeaponAutoHideDelay, 0.0f),
			false
		);
	}
}

void UCombatComponent::WeaponDissolve_PushHold()
{
	WeaponVisibilityHoldCount = FMath::Max(WeaponVisibilityHoldCount + 1, 0);

	// While holding (charge), ensure visible and cancel pending hide
	WeaponDissolve_RequestShow_Internal();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeaponAutoHideTimer);
	}
}

void UCombatComponent::WeaponDissolve_PopHold()
{
	WeaponVisibilityHoldCount = FMath::Max(WeaponVisibilityHoldCount - 1, 0);

	// When hold ends, restart the normal delay from "now"
	if (WeaponVisibilityHoldCount == 0)
	{
		WeaponDissolve_PingActivity();
	}
}

void UCombatComponent::WeaponDissolve_OnAutoHideTimer()
{
	// Never hide during a hold (charged attack, aim hold, etc.)
	if (WeaponVisibilityHoldCount > 0)
	{
		// Re-check shortly, but no hiding
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				WeaponAutoHideTimer,
				this,
				&UCombatComponent::WeaponDissolve_OnAutoHideTimer,
				0.15f,
				false
			);
		}
		return;
	}

	WeaponDissolve_RequestHide_Internal();
}
