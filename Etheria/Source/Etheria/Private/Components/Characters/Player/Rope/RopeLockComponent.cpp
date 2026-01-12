/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeLockComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeLockComponent.h"

#include "Characters/Players/PlayerCharacter.h"
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

	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;
	
	DetectionComponent = OwnerCharacter->GetRopeDetectionComponent();

	if (!DetectionComponent)
	{
		LOCK_LOG(LogTemp, Warning, TEXT("[RopeLock] No RopeDetectionComponent found on %s"), *OwnerCharacter->GetName());
	}

	if (DetectionComponent)
	{
		DetectionComponent->OnDetectedPointChanged.AddDynamic(this, &URopeLockComponent::OnDetectedPointChanged);
	}
}

bool URopeLockComponent::TryLock()
{
	if (!DetectionComponent) return false;

	if (LockedPoint.IsValid()) return true;

	ARopeAttachPoint* DetectedPoint = DetectionComponent->GetCurrentDetectedPoint();

	if (!DetectedPoint)
	{
		LOCK_LOG(LogTemp, Warning, TEXT("[RopeLock] TryLock failed: no detected point"));
		return false;
	}

	LockedPoint = DetectedPoint;
	LOCK_LOG(LogTemp, Log, TEXT("[RopeLock] Locked point: %s"), *DetectedPoint->GetName());
    
	BroadcastLockedPoint();
	return true;
}

void URopeLockComponent::Unlock()
{
	if (LockedPoint.IsValid())
	{
		LOCK_LOG(LogTemp, Log, TEXT("[RopeLock] Unlocked point: %s"), *LockedPoint->GetName());
	}

	LockedPoint.Reset();
	BroadcastLockedPoint();
}

void URopeLockComponent::OnDetectedPointChanged(ARopeAttachPoint* NewPoint)
{
	if (bIsPhysicallyAttached)
	{
		LOCK_LOG(LogTemp, Verbose, TEXT("[RopeLock] Detected point changed but physically attached - skipping auto-unlock"));
		return;
	}

	if (!LockedPoint.IsValid())
	{
		LOCK_LOG(LogTemp, Verbose, TEXT("[RopeLock] Detected point changed but no locked point - skipping auto-unlock"));
		return;
	}
    
	if (!NewPoint || NewPoint != LockedPoint.Get())
	{
		LOCK_LOG(LogTemp, Log, TEXT("[RopeLock] Auto-unlock triggered (lost detection before attach)"));
		Unlock();
	}
}

void URopeLockComponent::BroadcastLockedPoint()
{
	OnLockedPointChanged.Broadcast(LockedPoint.Get());
}

