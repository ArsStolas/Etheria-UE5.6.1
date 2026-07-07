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
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

UDungeonTravelComponent::UDungeonTravelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonTravelComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDungeonTravelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(fadeTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
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
	if (bTransitionInProgress)
	{
		return;
	}

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

	bTransitionInProgress = true;
	bPendingTransitionIsEnter = true;
	pendingTransitionSpawnTag = SpawnTag;
	StartCommitSequence();
}

void UDungeonTravelComponent::PerformEnterDungeon(const FName SpawnTag)
{
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

	// Kill any lingering departure dissolve timeline BEFORE the appear timeline starts,
	// otherwise both write the same material parameters and the character can stay invisible.
	if (bStopPortalTimelinesOnArrive)
	{
		StopPortalTimelines();
	}

	TryTriggerArriveInDungeonEvent();

	travelState = EDungeonTravelState::InDungeon;
	bPendingEnterCommit = false;
	pendingSpawnTag = NAME_None;
}

void UDungeonTravelComponent::CommitReturnToOrigin()
{
	if (bTransitionInProgress)
	{
		return;
	}

	bTransitionInProgress = true;
	bPendingTransitionIsEnter = false;
	pendingTransitionSpawnTag = NAME_None;
	StartCommitSequence();
}

void UDungeonTravelComponent::PerformReturnToOrigin()
{
	FTransform ReturnTransform = returnData.ReturnTransform;
	ReturnTransform.AddToTranslation(FVector(0.f, 0.f, ComputeLiftZ()));

	TeleportOwnerTo(ReturnTransform, returnData.ReturnControlRotation);

	// Kill lingering dissolve timelines on both portals before the appear timeline plays.
	if (bStopPortalTimelinesOnArrive)
	{
		StopPortalTimelines();
	}

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

// ------------------------------------------------------------------
// Fade transition
// ------------------------------------------------------------------
void UDungeonTravelComponent::NotifyPortalInteractionStarted()
{
	// Idle means the preload failed or nothing is set up: never black out the screen
	// for a travel that will not happen.
	if (travelState == EDungeonTravelState::Idle)
	{
		return;
	}

	// Snapshot the character's visible appearance BEFORE any dissolve VFX runs.
	CaptureOwnerAppearance();

	if (bUseFadeTransition)
	{
		FadeToBlack();
	}
}

void UDungeonTravelComponent::StartCommitSequence()
{
	if (bUseFadeTransition && GetCameraManager())
	{
		// The fade usually started at interaction time; wait for the remaining
		// fade-out (if any), plus the black hold, before teleporting.
		if (!bFadeOutStarted)
		{
			FadeToBlack();
		}

		const double Now = GetWorld()->GetTimeSeconds();
		const double Delay = FMath::Max(fadeBlackAtTime - Now, 0.0) + fadeHoldDuration;

		GetWorld()->GetTimerManager().SetTimer(
			fadeTimerHandle,
			this,
			&UDungeonTravelComponent::OnFadeOutFinished,
			FMath::Max(static_cast<float>(Delay), 0.02f),
			false
		);
		return;
	}

	OnFadeOutFinished();
}

void UDungeonTravelComponent::OnFadeOutFinished()
{
	if (bPendingTransitionIsEnter)
	{
		PerformEnterDungeon(pendingTransitionSpawnTag);
	}
	else
	{
		PerformReturnToOrigin();
	}

	pendingTransitionSpawnTag = NAME_None;

	// Verification delay, still fully black: streaming settles, the character
	// appearance is restored, and the camera/spring arm finishes blending.
	if (postTeleportSettleDelay > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			fadeTimerHandle,
			this,
			&UDungeonTravelComponent::FinishTransition,
			postTeleportSettleDelay,
			false
		);
	}
	else
	{
		FinishTransition();
	}
}

void UDungeonTravelComponent::FinishTransition()
{
	// Kill any dissolve timeline still running (departure OR arrival), then snap
	// the character back to the exact appearance captured at interaction time.
	if (bStopPortalTimelinesOnArrive)
	{
		StopPortalTimelines();
	}

	if (bEnsureVisibleAfterTravel)
	{
		RestoreOwnerAppearance();
	}

	if (bFadeOutStarted)
	{
		FadeFromBlack();
	}

	bFadeOutStarted = false;
	bTransitionInProgress = false;
}

void UDungeonTravelComponent::FadeToBlack()
{
	if (APlayerCameraManager* CameraManager = GetCameraManager())
	{
		CameraManager->StartCameraFade(0.f, 1.f, FMath::Max(fadeOutDuration, 0.05f), fadeColor, bFadeAudio, /*bHoldWhenFinished*/ true);
		bFadeOutStarted = true;
		fadeBlackAtTime = GetWorld()->GetTimeSeconds() + FMath::Max(fadeOutDuration, 0.05f);
	}
}

void UDungeonTravelComponent::FadeFromBlack() const
{
	if (APlayerCameraManager* CameraManager = GetCameraManager())
	{
		CameraManager->StartCameraFade(1.f, 0.f, FMath::Max(fadeInDuration, 0.05f), fadeColor, bFadeAudio, /*bHoldWhenFinished*/ false);
	}
}

APlayerCameraManager* UDungeonTravelComponent::GetCameraManager() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	return PC ? PC->PlayerCameraManager : nullptr;
}

// ------------------------------------------------------------------
// Character appearance safety
// ------------------------------------------------------------------
void UDungeonTravelComponent::StopPortalTimelines() const
{
	const auto StopTimelinesOn = [](const TWeakObjectPtr<AActor>& PortalActor)
	{
		if (!PortalActor.IsValid())
		{
			return;
		}

		TArray<UTimelineComponent*> Timelines;
		PortalActor->GetComponents<UTimelineComponent>(Timelines);
		for (UTimelineComponent* Timeline : Timelines)
		{
			if (Timeline && Timeline->IsPlaying())
			{
				Timeline->Stop();
			}
		}
	};

	StopTimelinesOn(enterPortalActor);
	StopTimelinesOn(exitPortalActor);
}

void UDungeonTravelComponent::CaptureOwnerAppearance()
{
	if (bAppearanceCaptured)
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	capturedOverlayMaterial = Mesh->GetOverlayMaterial();

	bCapturedDissolve = false;
	bCapturedColorOpacity = false;

	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		const UMaterialInterface* Material = Mesh->GetMaterial(Index);
		if (!Material)
		{
			continue;
		}

		float Value = 0.f;
		if (!bCapturedDissolve && Material->GetScalarParameterValue(FMaterialParameterInfo(dissolveParamName), Value))
		{
			capturedDissolveValue = Value;
			bCapturedDissolve = true;
		}
		if (!bCapturedColorOpacity && Material->GetScalarParameterValue(FMaterialParameterInfo(colorOpacityParamName), Value))
		{
			capturedColorOpacityValue = Value;
			bCapturedColorOpacity = true;
		}

		if (bCapturedDissolve && bCapturedColorOpacity)
		{
			break;
		}
	}

	bAppearanceCaptured = true;
}

void UDungeonTravelComponent::RestoreOwnerAppearance()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	Character->SetActorHiddenInGame(false);

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		Mesh->SetVisibility(true, true);
		Mesh->SetScalarParameterValueOnMaterials(dissolveParamName, bCapturedDissolve ? capturedDissolveValue : visibleDissolveValue);
		Mesh->SetScalarParameterValueOnMaterials(colorOpacityParamName, bCapturedColorOpacity ? capturedColorOpacityValue : visibleColorOpacityValue);

		if (bAppearanceCaptured)
		{
			Mesh->SetOverlayMaterial(capturedOverlayMaterial);
		}
	}

	bAppearanceCaptured = false;
	bCapturedDissolve = false;
	bCapturedColorOpacity = false;
	capturedOverlayMaterial = nullptr;
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
