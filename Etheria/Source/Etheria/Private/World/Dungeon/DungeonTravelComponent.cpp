/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDungeonTravelComponent" - Source
 */

#include "World/Dungeon/DungeonTravelComponent.h"

#include "World/Dungeon/DungeonPortal.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

UDungeonTravelComponent::UDungeonTravelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonTravelComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDungeonTravelComponent::SetEnterPortalActor(AActor* PortalActor)
{
	enterPortalActor = PortalActor;
}

void UDungeonTravelComponent::SetExitPortalActor(AActor* PortalActor)
{
	exitPortalActor = PortalActor;
}

float UDungeonTravelComponent::ComputeLiftZ() const
{
	if (!bAutoLiftByCapsuleHalfHeight)
	{
		return extraTeleportZ;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return extraTeleportZ;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!Capsule)
	{
		return extraTeleportZ;
	}

	return Capsule->GetScaledCapsuleHalfHeight() + extraTeleportZ;
}

void UDungeonTravelComponent::BeginPreloadFromPortal(TSoftObjectPtr<UWorld> DungeonLevel, const FVector& InstanceLocation, const FTransform& ReturnTransform, const FRotator& ReturnControlRotation)
{
	if (!DungeonLevel.ToSoftObjectPath().IsValid())
	{
		return;
	}

	cachedInstanceLocation = InstanceLocation;

	returnData.ReturnTransform = ReturnTransform;
	returnData.ReturnControlRotation = ReturnControlRotation;
	returnData.OriginLevel = GetWorld();

	if (streamingLevel)
	{
		streamingLevel->SetIsRequestingUnloadAndRemoval(true);
		streamingLevel = nullptr;
	}

	pendingLevel = DungeonLevel;
	travelState = EDungeonTravelState::Preloading;

	bool bSuccess = false;

	streamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(),
		DungeonLevel,
		InstanceLocation,
		FRotator::ZeroRotator,
		bSuccess
	);

	if (!bSuccess || !streamingLevel)
	{
		travelState = EDungeonTravelState::Idle;
		return;
	}

	streamingLevel->SetShouldBeVisible(false);
	streamingLevel->SetShouldBeLoaded(true);

	StartPollingPreload();
}

void UDungeonTravelComponent::CommitEnterDungeon(const FName SpawnTag)
{
	if (travelState != EDungeonTravelState::ReadyToEnter)
	{
		bPendingEnterCommit = true;
		pendingSpawnTag = SpawnTag;
		return;
	}

	if (!streamingLevel || !streamingLevel->IsLevelLoaded())
	{
		return;
	}

	streamingLevel->SetShouldBeVisible(true);

	FTransform SpawnTransform;
	const bool bFound = TryResolveSpawnTransform(SpawnTag, SpawnTransform);
	if (!bFound)
	{
		SpawnTransform = FTransform(FRotator::ZeroRotator, cachedInstanceLocation);
	}

	SpawnTransform.AddToTranslation(FVector(0.f, 0.f, ComputeLiftZ()));
	TeleportOwnerTo(SpawnTransform, SpawnTransform.Rotator());

	TryTriggerArriveInDungeonEvent();

	travelState = EDungeonTravelState::InDungeon;
	bPendingEnterCommit = false;
	pendingSpawnTag = NAME_None;
}

void UDungeonTravelComponent::CommitReturnToOrigin()
{
	FTransform ReturnTransform = returnData.ReturnTransform;
	ReturnTransform.AddToTranslation(FVector(0.f, 0.f, ComputeLiftZ()));

	TeleportOwnerTo(ReturnTransform, returnData.ReturnControlRotation);

	// IMPORTANT:
	// Fire arrival-back event on the ENTER portal (main world) so its BP Timeline can finish.
	TryTriggerArriveBackToOriginEvent();

	// Now unload streamed dungeon.
	if (streamingLevel)
	{
		streamingLevel->SetIsRequestingUnloadAndRemoval(true);
		streamingLevel = nullptr;
	}

	travelState = EDungeonTravelState::Idle;
	bPendingEnterCommit = false;
	pendingSpawnTag = NAME_None;

	exitPortalActor = nullptr;
}

void UDungeonTravelComponent::StartPollingPreload()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(preloadPollHandle, this, &UDungeonTravelComponent::PollPreload, 0.05f, true);
}

void UDungeonTravelComponent::StopPollingPreload()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(preloadPollHandle);
}

void UDungeonTravelComponent::PollPreload()
{
	if (!streamingLevel)
	{
		StopPollingPreload();
		travelState = EDungeonTravelState::Idle;
		return;
	}

	if (!streamingLevel->IsLevelLoaded())
	{
		return;
	}

	StopPollingPreload();

	travelState = EDungeonTravelState::ReadyToEnter;
	OnPreloadReady.Broadcast(pendingLevel);

	if (bPendingEnterCommit)
	{
		const FName SpawnTag = pendingSpawnTag;
		CommitEnterDungeon(SpawnTag);
	}
}

bool UDungeonTravelComponent::TryResolveSpawnTransform(const FName SpawnTag, FTransform& OutTransform) const
{
	if (!streamingLevel || !streamingLevel->IsLevelLoaded())
	{
		return false;
	}

	ULevel* LoadedLevel = streamingLevel->GetLoadedLevel();
	if (!LoadedLevel)
	{
		return false;
	}

	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (SpawnTag.IsNone() || Actor->ActorHasTag(SpawnTag))
		{
			OutTransform = Actor->GetActorTransform();

			const FVector InstanceBase = streamingLevel->LevelTransform.GetTranslation();
			const FVector Loc = OutTransform.GetLocation();

			if (Loc.Size() < 500000.f && InstanceBase.Size() > 2000000.f)
			{
				OutTransform.AddToTranslation(InstanceBase);
			}

			return true;
		}
	}

	if (!SpawnTag.IsNone())
	{
		TArray<AActor*> TaggedActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), SpawnTag, TaggedActors);

		for (AActor* Actor : TaggedActors)
		{
			if (IsValid(Actor) && Actor->GetLevel() == LoadedLevel)
			{
				OutTransform = Actor->GetActorTransform();
				return true;
			}
		}
	}

	return false;
}

void UDungeonTravelComponent::TeleportOwnerTo(const FTransform& Target, const FRotator& ControlRot) const
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}

	const FVector DestLocation = Target.GetLocation();
	const FRotator DestRotation = Target.Rotator();

	const bool bTeleported = Character->TeleportTo(DestLocation, DestRotation, false, false);
	if (!bTeleported)
	{
		Character->SetActorLocationAndRotation(DestLocation, DestRotation, true, nullptr, ETeleportType::TeleportPhysics);
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		PC->SetControlRotation(ControlRot);
	}

	Character->SetActorRotation(FRotator(0.f, ControlRot.Yaw, 0.f));
}

void UDungeonTravelComponent::TryTriggerArriveInDungeonEvent() const
{
	if (enterPortalActor.IsValid())
	{
		if (ADungeonPortal* Portal = Cast<ADungeonPortal>(enterPortalActor.Get()))
		{
			Portal->OnArriveInDungeon(GetOwner());
			return;
		}
	}

	if (exitPortalActor.IsValid())
	{
		if (ADungeonPortal* Portal = Cast<ADungeonPortal>(exitPortalActor.Get()))
		{
			Portal->OnArriveInDungeon(GetOwner());
		}
	}
}

void UDungeonTravelComponent::TryTriggerArriveBackToOriginEvent() const
{
	if (enterPortalActor.IsValid())
	{
		if (ADungeonPortal* Portal = Cast<ADungeonPortal>(enterPortalActor.Get()))
		{
			Portal->OnArriveBackToOrigin(GetOwner());
			return;
		}
	}

	if (exitPortalActor.IsValid())
	{
		if (ADungeonPortal* Portal = Cast<ADungeonPortal>(exitPortalActor.Get()))
		{
			Portal->OnArriveBackToOrigin(GetOwner());
		}
	}
}
