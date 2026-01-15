/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "LowHealthPostProcessComponent" - Source
 */

#include "Components/Characters/Player/LowHealthPostProcessComponent.h"

#include "Components/Characters/HealthComponent.h"

#include "Components/PostProcessComponent.h"
#include "Engine/World.h"

ULowHealthPostProcessComponent::ULowHealthPostProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULowHealthPostProcessComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bEnableLowHealthEffect)
	{
		return;
	}

	CachedPostProcess = ResolvePostProcessComponent();
	CachedHealth = ResolveHealthComponent();

	// Register the PP blendable material with 0 weight by default.
	EnsureBlendableRegistered();

	// Bind to health.
	if (CachedHealth.IsValid())
	{
		CachedHealth->OnHealthChanged.AddDynamic(this, &ULowHealthPostProcessComponent::HandleHealthChanged);

		// Initialize state from current health.
		HandleHealthChanged(CachedHealth->GetHealth(), CachedHealth->GetMaxHealth());
	}
}

void ULowHealthPostProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHeartbeat();

	if (CachedHealth.IsValid())
	{
		CachedHealth->OnHealthChanged.RemoveDynamic(this, &ULowHealthPostProcessComponent::HandleHealthChanged);
	}

	Super::EndPlay(EndPlayReason);
}

// =============================================================
// Health binding
// =============================================================

void ULowHealthPostProcessComponent::HandleHealthChanged(float NewHealth, float InMaxHealth)
{
	if (!bEnableLowHealthEffect || InMaxHealth <= 0.f)
	{
		StopHeartbeat();
		return;
	}

	const float Pct = FMath::Clamp(NewHealth / InMaxHealth, 0.f, 1.f);
	if (Pct <= ThresholdPercent)
	{
		StartHeartbeat();
	}
	else
	{
		StopHeartbeat();
	}
}

// =============================================================
// Heartbeat
// =============================================================

void ULowHealthPostProcessComponent::StartHeartbeat()
{
	if (bHeartbeatActive)
	{
		return;
	}

	if (!GetWorld() || !CachedPostProcess.IsValid() || !PostProcessMaterial)
	{
		return;
	}

	EnsureBlendableRegistered();
	if (BlendableIndex == INDEX_NONE)
	{
		return;
	}

	bHeartbeatActive = true;
	HeartbeatStartTime = GetWorld()->GetTimeSeconds();
	ApplyBlendWeight(PulseMaxWeight);

	// Timer-based update (preferred over Tick for this project).
	GetWorld()->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		HeartbeatTimerHandle,
		this,
		&ULowHealthPostProcessComponent::UpdateHeartbeat,
		FMath::Max(0.005f, UpdateInterval),
		true
	);
}

void ULowHealthPostProcessComponent::StopHeartbeat()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
	}

	bHeartbeatActive = false;
	ApplyBlendWeight(0.f);
}

void ULowHealthPostProcessComponent::UpdateHeartbeat()
{
	if (!GetWorld() || !bHeartbeatActive)
	{
		StopHeartbeat();
		return;
	}

	const float TimeNow = GetWorld()->GetTimeSeconds();
	const float BeatsPerSecond = FMath::Max(HeartbeatBPM, 10.f) / 60.f;
	const float Phase01 = FMath::Fmod((TimeNow - HeartbeatStartTime) * BeatsPerSecond, 1.f);
	const float Alpha = ComputeHeartbeatAlpha(Phase01);

	const float Weight = FMath::Lerp(PulseMinWeight, PulseMaxWeight, Alpha);
	ApplyBlendWeight(Weight);
}

float ULowHealthPostProcessComponent::ComputeHeartbeatAlpha(const float Phase01) const
{
	// Two gaussian-like peaks for a "lub-dub" feel.
	// Peak #1 at phase 0.0, Peak #2 at phase 0.22
	const float PeakSpread = 0.06f;
	const float Peak1 = FMath::Exp(-FMath::Square((Phase01 - 0.0f) / PeakSpread));
	const float Peak2 = 0.65f * FMath::Exp(-FMath::Square((Phase01 - 0.22f) / PeakSpread));
	return FMath::Clamp(Peak1 + Peak2, 0.f, 1.f);
}

// =============================================================
// Component resolution
// =============================================================

UPostProcessComponent* ULowHealthPostProcessComponent::ResolvePostProcessComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UActorComponent* Comp = PostProcessComponentRef.GetComponent(Owner))
	{
		return Cast<UPostProcessComponent>(Comp);
	}

	return Owner->FindComponentByClass<UPostProcessComponent>();
}

UHealthComponent* ULowHealthPostProcessComponent::ResolveHealthComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UActorComponent* Comp = HealthComponentRef.GetComponent(Owner))
	{
		return Cast<UHealthComponent>(Comp);
	}

	return Owner->FindComponentByClass<UHealthComponent>();
}

// =============================================================
// Apply blendable weight
// =============================================================

void ULowHealthPostProcessComponent::ApplyBlendWeight(const float NewWeight)
{
	if (!CachedPostProcess.IsValid() || !PostProcessMaterial)
	{
		return;
	}

	EnsureBlendableRegistered();
	if (BlendableIndex == INDEX_NONE)
	{
		return;
	}

	auto& Blendables = CachedPostProcess->Settings.WeightedBlendables.Array;
	Blendables[BlendableIndex].Weight = FMath::Clamp(NewWeight, 0.f, 1.f);
}

void ULowHealthPostProcessComponent::EnsureBlendableRegistered()
{
	if (!CachedPostProcess.IsValid() || !PostProcessMaterial)
	{
		BlendableIndex = INDEX_NONE;
		return;
	}

	auto& Blendables = CachedPostProcess->Settings.WeightedBlendables.Array;

	// If we already have a valid index pointing to the correct material, keep it.
	if (Blendables.IsValidIndex(BlendableIndex) && Blendables[BlendableIndex].Object == PostProcessMaterial)
	{
		return;
	}

	// Otherwise, search for the material.
	for (int32 i = 0; i < Blendables.Num(); ++i)
	{
		if (Blendables[i].Object == PostProcessMaterial)
		{
			BlendableIndex = i;
			return;
		}
	}

	// Not found -> add it with 0 weight.
	BlendableIndex = Blendables.Add(FWeightedBlendable(0.f, PostProcessMaterial));
}

// =============================================================
// Blueprint API
// =============================================================

void ULowHealthPostProcessComponent::SetPulseWeightRange(const float NewMin, const float NewMax)
{
	PulseMinWeight = FMath::Clamp(NewMin, 0.f, 1.f);
	PulseMaxWeight = FMath::Clamp(NewMax, 0.f, 1.f);
}

void ULowHealthPostProcessComponent::SetThresholdPercent(const float NewThresholdPercent)
{
	ThresholdPercent = FMath::Clamp(NewThresholdPercent, 0.f, 1.f);
}

void ULowHealthPostProcessComponent::SetHeartbeatBPM(const float NewBPM)
{
	HeartbeatBPM = FMath::Max(10.f, NewBPM);
}

void ULowHealthPostProcessComponent::ForceEnableEffect(const bool bEnable)
{
	bEnableLowHealthEffect = bEnable;
	if (!bEnableLowHealthEffect)
	{
		StopHeartbeat();
		return;
	}

	// Re-evaluate immediately.
	if (CachedHealth.IsValid())
	{
		HandleHealthChanged(CachedHealth->GetHealth(), CachedHealth->GetMaxHealth());
	}
}
