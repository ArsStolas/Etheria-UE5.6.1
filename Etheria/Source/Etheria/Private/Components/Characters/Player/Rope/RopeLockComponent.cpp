#include "Components/Characters/Player/Rope/RopeLockComponent.h"

#include "Components/Characters/Player/Rope/RopeDetectionComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "GameFramework/Actor.h"

URopeLockComponent::URopeLockComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URopeLockComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	DetectionComponent = Owner->FindComponentByClass<URopeDetectionComponent>();

	if (!DetectionComponent && bDebugMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RopeLock] No RopeDetectionComponent found on %s"), *Owner->GetName());
	}

	if (DetectionComponent)
	{
		DetectionComponent->OnDetectedPointChanged.AddDynamic(this, &URopeLockComponent::OnDetectedPointChanged);
	}
}

bool URopeLockComponent::TryLock()
{
	if (!DetectionComponent)
	{
		return false;
	}

	if (LockedPoint.IsValid())
	{
		return true;
	}

	ARopeAttachPoint* DetectedPoint =
		DetectionComponent->GetCurrentDetectedPoint();

	if (!DetectedPoint)
	{
		if (bDebugMode)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RopeLock] TryLock failed: no detected point"));
		}
		return false;
	}

	LockedPoint = DetectedPoint;

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[RopeLock] Locked point: %s"),
			*DetectedPoint->GetName());
	}

	return true;
}

void URopeLockComponent::Unlock()
{
	if (LockedPoint.IsValid() && bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("[RopeLock] Unlocked point: %s"), *LockedPoint->GetName());
	}

	LockedPoint.Reset();
}

bool URopeLockComponent::HasLockedPoint() const
{
	return LockedPoint.IsValid();
}

void URopeLockComponent::OnDetectedPointChanged(ARopeAttachPoint* NewPoint)
{
	if (!LockedPoint.IsValid())
	{
		return;
	}

	// Si on n'est PAS encore attaché physiquement
	// et que le point détecté change / disparaît
	if (!NewPoint || NewPoint != LockedPoint.Get())
	{
		if (bDebugMode)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[RopeLock] Auto-unlock (detection lost via event)"));
		}

		Unlock();
	}
}

ARopeAttachPoint* URopeLockComponent::GetLockedPoint() const
{
	return LockedPoint.Get();
}
