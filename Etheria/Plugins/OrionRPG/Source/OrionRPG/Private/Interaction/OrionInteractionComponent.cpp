// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "Interaction/OrionInteractionComponent.h"
#include "Interaction/OrionInteractionTargetComponent.h"
#include "CollisionQueryParams.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"



UOrionInteractionComponent::UOrionInteractionComponent()
{
	// Initialize variables
	bInteracting = false;
	DetectionRadius = 450.0f;
	InteractionNetMode = EOrionInteractionNetMode::E_OwnerOnly;
	InteractionTraceChannel = ECC_WorldDynamic;
	InteractionType = EOrionInteractionType::E_InteractionPawn;
	InteractionRadius = 80.0f;
	InteractionTraceDistance = 750.0f;
	InteractionTraceIntervalSecond = 0.3f;
	InteractionOffset = FVector(75.f, 0.f, 0.f);
	bIsInteractionEnabled = false;
	bDebugTrace = false;

	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UOrionInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickInterval(InteractionTraceIntervalSecond);

	// Cache the owning controller
	OwningController = GetOwningController();

	if (OwningController)
	{
        // Fix: Use a delegate to bind the function properly
        OwningController->OnPossessedPawnChanged.AddDynamic(this, &UOrionInteractionComponent::OnPossessedPawnChanged);
     
	}
	SetInteractionEnabled(true);

	
}
void UOrionInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if ((EndPlayReason == EEndPlayReason::Destroyed ||
		EndPlayReason == EEndPlayReason::RemovedFromWorld) && bInteracting)
	{
		CancelInteraction();
	}
}

void UOrionInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentInteractionTarget);
	DOREPLIFETIME(ThisClass, bIsInteractionEnabled);
	DOREPLIFETIME(ThisClass, InteractionIndex);
	DOREPLIFETIME(ThisClass, bInteracting);
}

void UOrionInteractionComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (NewPawn == nullptr)
	{
		SetInteractionEnabled(false);
	}
	else
	{
		SetInteractionEnabled(true);
	}
}


void UOrionInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DetectAvailableInteractionTargets();
	UpdateInteractionTrace();

}

void UOrionInteractionComponent::OnRep_bInteracting()
{
	if (OnInteractingStateChanged.IsBound())
	{
		OnInteractingStateChanged.Broadcast(bInteracting);
	}
}

APawn* UOrionInteractionComponent::GetOwningPawn() const
{
	if (OwningController)
	{
		return OwningController->GetPawn();
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (OwningPawn)
	{
		return OwningPawn;
	}

	if (!OwningPawn && PC)
	{
		return PC->GetPawn();
	}

	return nullptr;
}

APlayerController* UOrionInteractionComponent::GetOwningController() const
{
	if (OwningController)
	{
		return OwningController;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (PC)
	{
		return PC;
	}

	if (!PC && OwningPawn)
	{
		return Cast<APlayerController>(OwningPawn->GetController());
	}

	return nullptr;
}

void UOrionInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bIsInteractionEnabled == bEnabled)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Set interaction enabled: %d"), bEnabled);

	bIsInteractionEnabled = bEnabled;
	UWorld* World = GetWorld();
	if (World)
	{
		if (bIsInteractionEnabled)
		{
			SetComponentTickEnabled(
				ShouldTickInstance()
			);
		}
		else
		{
			SetComponentTickEnabled(false);
			CancelInteraction();
			CurrentInteractionTarget = nullptr;

		}
	}
}


void UOrionInteractionComponent::RegisterNewInteractionTarget(UOrionInteractionTargetComponent* NewInteractionTarget)
{
	/* Prevent Duplicate Registration */
	if (CurrentInteractionTarget == NewInteractionTarget)
	{
		return;
	}

	CurrentInteractionTarget = NewInteractionTarget;

	/* Local Interactor */
	if (IsLocalInteractor())
	{
		if (IsValid(CurrentInteractionTarget))
		{
			NewInteractionTarget->SetFocusState(true, GetOwningPawn());
		}

		if (OnInteractionChanged.IsBound())
		{
			OnInteractionChanged.Broadcast(NewInteractionTarget, NewInteractionTarget ? NewInteractionTarget->Tag : FGameplayTag().EmptyTag);
		}
	}
}

void UOrionInteractionComponent::DeRegisterInteractionTarget()
{
	/* Cancel Interaction If Already Interacting */
	if (bInteracting)
	{
		CancelInteraction();
	}

	/* Local Interactor */
	if (IsLocalInteractor())
	{
		if (IsValid(CurrentInteractionTarget))
		{
			CurrentInteractionTarget->SetFocusState(false, nullptr);
		}

		if (OnInteractionChanged.IsBound())
		{
			OnInteractionChanged.Broadcast(nullptr, FGameplayTag().EmptyTag);
		}
	}

	CurrentInteractionTarget = nullptr;
}

void UOrionInteractionComponent::NotifyInteraction(EOrionInteractionResult NewInteractionResult)
{
	Client_NotifyInteractionTarget(NewInteractionResult);
	switch (InteractionNetMode)
	{
		case EOrionInteractionNetMode::E_Server:
			CallInteractionResult(NewInteractionResult, CurrentInteractionTarget, CurrentInteractionTarget ? CurrentInteractionTarget->Tag : FGameplayTag().EmptyTag);
			break;
		case EOrionInteractionNetMode::E_OwnerOnly:
			Client_NotifyInteraction(NewInteractionResult);
			break;
		case EOrionInteractionNetMode::E_All:
			Multicast_NotifyInteraction(NewInteractionResult);
			break;
		default:
			break;
	}
}

void UOrionInteractionComponent::Client_NotifyInteraction_Implementation(EOrionInteractionResult NewInteractionResult)
{
	CallInteractionResult(
		NewInteractionResult,
		CurrentInteractionTarget,
		CurrentInteractionTarget ? CurrentInteractionTarget->Tag : FGameplayTag().EmptyTag
	);
	
	
}

void UOrionInteractionComponent::Client_NotifyInteractionTarget_Implementation(EOrionInteractionResult NewInteractionResult)         
{
	if (IsValid(CurrentInteractionTarget))
	{
		CurrentInteractionTarget->Client_NotifyInteraction(NewInteractionResult, GetOwningPawn());
	}
}

void UOrionInteractionComponent::Multicast_NotifyInteraction_Implementation(EOrionInteractionResult NewInteractionResult)
{
	CallInteractionResult(
		NewInteractionResult,
		CurrentInteractionTarget,
		CurrentInteractionTarget ? CurrentInteractionTarget->Tag : FGameplayTag().EmptyTag
	);
}


void UOrionInteractionComponent::CallInteractionResult(EOrionInteractionResult NewInteractionResult, UOrionInteractionTargetComponent* InteractionTarget, FGameplayTag Tag)
{
	switch (NewInteractionResult)
	{
		case EOrionInteractionResult::E_Started:
			OnInteractionStarted.Broadcast(InteractionTarget, Tag);
			break;
		case EOrionInteractionResult::E_Completed:
			OnInteractionCompleted.Broadcast(InteractionTarget, Tag);
			break;
		case EOrionInteractionResult::E_Canceled:
			OnInteractionCanceled.Broadcast(InteractionTarget, Tag);
			break;
		case EOrionInteractionResult::E_Blocked:
			OnInteractionBlocked.Broadcast(InteractionTarget, Tag);
			break;
		default:
			break;
	}
}

void UOrionInteractionComponent::UpdateInteractionTrace()
{
	if (bInteracting)
	{
		/* If Interacting Get the New Interaction Candidate and Compare to the Current Interacting Component*/
		const UOrionInteractionTargetComponent* NewInteraction = GetInteractionTrace();

		if (NewInteraction != CurrentInteractionTarget)
		{
			/* Cancel Interaction If not Valid Interaction */
			DeRegisterInteractionTarget();
		}

	}
	else if (IsLocalInteractor())
	{
		/* Locally Get Interaction and Validate the Component */
		UOrionInteractionTargetComponent* NewInteraction = GetInteractionTrace();

		/* Register If New Interaction is Not Equal to the Current Candidate */
		if (NewInteraction != CurrentInteractionTarget)
		{
			RegisterNewInteractionTarget(NewInteraction);
		}
		

	}

	
	/*if (LastInteractionTarget && LastInteractionTarget->Mode == EOrionInteractionMode::Hold && LastInteractionTarget->IsInteractionInProgress() && CurrentInteractionTarget == nullptr)
	{
		LastInteractionTarget->EndInteraction(false);
	}*/
	

}

UOrionInteractionTargetComponent* UOrionInteractionComponent::GetInteractionTrace()
{
	if (!bIsInteractionEnabled)
	{
		return nullptr;
	}

	FVector TraceStart;
	FRotator Rotation;
	TArray<FHitResult> InteractionhitResults;
	TArray<AActor*> InteractionActors;

	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(AOrionInteractionComponent_UpdateInteracionTrace));
	CollisionQueryParams.AddIgnoredActor(GetOwningPawn());

	switch (InteractionType)
	{
		case EOrionInteractionType::E_InteractionPawn:
		{
			TraceStart = GetOwningPawn()->GetActorLocation();
			Rotation = GetOwningPawn()->GetActorRotation();
			TraceStart += GetOwningPawn()->GetActorForwardVector() * InteractionOffset.X;
			TraceStart += GetOwningPawn()->GetActorRightVector() * InteractionOffset.Y;
			TraceStart += GetOwningPawn()->GetActorUpVector() * InteractionOffset.Z;
			FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionRadius);

			TArray<FHitResult> HitResults;
			GetWorld()->SweepMultiByChannel(HitResults, TraceStart, TraceStart, FQuat::Identity, InteractionTraceChannel, Sphere);


			for (auto& Hit : HitResults)
			{
				if (!Hit.GetActor())
					continue;

				if (UOrionInteractionTargetComponent* TargetInteractable = Hit.GetActor()->FindComponentByClass<UOrionInteractionTargetComponent>())
				{
					if (Hit.GetActor()->IsPendingKillPending())
						continue;

					if (!TargetInteractable->IsActive())
						continue;

					if (Hit.GetActor()->IsHidden())
						continue;

					FVector Direction = TargetInteractable->GetOwner()->GetActorLocation() - GetOwningPawn()->GetActorLocation();
					Direction.Normalize();
					FVector InteractableDirection =  GetOwningPawn()->GetActorLocation() - TargetInteractable->GetOwner()->GetActorLocation();
					InteractableDirection.Normalize();

					bool bPawnFacingInteractable = FVector::DotProduct(Direction, GetOwningPawn()->GetActorForwardVector()) > 0.5f;
					bool bInteractableFacingPawn = FVector::DotProduct(InteractableDirection, TargetInteractable->GetOwner()->GetActorForwardVector()) > 0.5f;
					bool bInteractableIsNotSelf = TargetInteractable->GetOwner() != GetOwningPawn();


					bool bAllowInteractable = TargetInteractable->bFacingTarget
						? (bPawnFacingInteractable && bInteractableFacingPawn)
						: bPawnFacingInteractable;

					if (bAllowInteractable && bInteractableIsNotSelf)
					{
						InteractionhitResults.Add(Hit);
						InteractionActors.AddUnique(Hit.GetActor());
					}
				}
			}

			if (!InteractionActors.IsEmpty())
			{
				float Dist;
				return UGameplayStatics::FindNearestActor(GetOwningPawn()->GetActorLocation(), InteractionActors, Dist)->FindComponentByClass<UOrionInteractionTargetComponent>();
			}
	#if WITH_EDITOR
			if (bDebugTrace)
			{
				DrawDebugSphere(
					GetWorld(),
					TraceStart,
					InteractionRadius,
					12,
					FColor::Red,
					false,
					2.f,
					0,
					1.f
				);
			}
	#endif // WITH_EDITOR

			break;
		}

		case EOrionInteractionType::E_InteractionCamera:
		{
			OwningController->GetPlayerViewPoint(TraceStart, Rotation);

			const FVector TraceDirection = Rotation.Vector();
			FVector TraceEnd = TraceStart + TraceDirection * InteractionTraceDistance;

			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, InteractionTraceChannel, CollisionQueryParams);
			InteractionhitResults.Add(HitResult);

			if (!InteractionhitResults.IsValidIndex(0)) return nullptr;
			if (!InteractionhitResults[0].GetActor()) return nullptr;

			if (UOrionInteractionTargetComponent* TargetInteractable = InteractionhitResults[0].GetActor()->FindComponentByClass<UOrionInteractionTargetComponent>())
			{
				if (InteractionhitResults[0].GetActor()->IsPendingKillPending()) return nullptr;

				if (InteractionhitResults[0].GetActor()->IsHidden()) return nullptr;

				if (!TargetInteractable->IsActive()) return nullptr;

				
				bool bInteractableIsNotSelf = TargetInteractable->GetOwner() != GetOwningPawn();
				if (bInteractableIsNotSelf)
				{
					return TargetInteractable;
				}
			}
	#if WITH_EDITOR
			if (bDebugTrace)
			{
				DrawDebugLine(
					GetWorld(),
					TraceStart,
					TraceEnd,
					FColor::Red,
					false,
					2.f,
					0,
					1.f
				);
			}
	#endif // WITH_EDITOR
			break;
		}
	}

	return nullptr;


	
}

void UOrionInteractionComponent::DetectAvailableInteractionTargets()
{
	if (!bIsInteractionEnabled) return;
	if (!GetOwningPawn()) return;

	FVector TraceStart;
	FRotator Rotation;
	TArray<FHitResult> InteractionhitResults;
	TArray<TObjectPtr<UOrionInteractionTargetComponent>> CachedInteractionTargets;

	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(AOrionInteractionComponent_DetectAvailableInteractionTargets));
	CollisionQueryParams.AddIgnoredActor(GetOwningPawn());

	TraceStart = GetOwningPawn()->GetActorLocation();
	Rotation = GetOwningPawn()->GetActorRotation();
	FCollisionShape Sphere = FCollisionShape::MakeSphere(DetectionRadius);

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByChannel(HitResults, TraceStart, TraceStart, FQuat::Identity, InteractionTraceChannel, Sphere);

	for (auto& Hit : HitResults)
	{
		if (!Hit.GetActor())
			continue;

		if (UOrionInteractionTargetComponent* TargetInteractable = Hit.GetActor()->FindComponentByClass<UOrionInteractionTargetComponent>())
		{
			if (Hit.GetActor()->IsPendingKillPending())
				continue;

			if (Hit.GetActor()->IsHidden())
				continue;

			if (!TargetInteractable->IsActive())
				continue;

			bool bInteractableIsNotSelf = TargetInteractable->GetOwner() != GetOwningPawn();

			if (bInteractableIsNotSelf)
			{
				InteractionhitResults.Add(Hit);
				CachedInteractionTargets.AddUnique(TargetInteractable);
			}
			
		}
	}

	bool bChanged = false;
	TSet<TObjectPtr<UOrionInteractionTargetComponent>> OldSet(AvailableInteractionTargets);
	TSet<TObjectPtr<UOrionInteractionTargetComponent>> NewSet(CachedInteractionTargets);
	if (!OldSet.Includes(NewSet) || !NewSet.Includes(OldSet))
	{
		bChanged = true;
	}

	// Broadcast added and removed interaction targets
	if (bChanged)
	{
		// Find added targets (in NewSet but not in OldSet)
		for (TObjectPtr<UOrionInteractionTargetComponent> AddedTarget : NewSet.Difference(OldSet))
		{
			if (OnAvailableInteractionAdded.IsBound())
			{
				OnAvailableInteractionAdded.Broadcast(AddedTarget, AddedTarget ? AddedTarget->Tag : FGameplayTag().EmptyTag);
			}
		}

		// Find removed targets (in OldSet but not in NewSet)
		for (TObjectPtr<UOrionInteractionTargetComponent> RemovedTarget : OldSet.Difference(NewSet))
		{
			if (OnAvailableInteractionRemoved.IsBound())
			{
				OnAvailableInteractionRemoved.Broadcast(RemovedTarget, RemovedTarget ? RemovedTarget->Tag : FGameplayTag().EmptyTag);
			}
		}

		AvailableInteractionTargets = CachedInteractionTargets;
		if (OnAvailableInteractionChanged.IsBound())
		{
			OnAvailableInteractionChanged.Broadcast(AvailableInteractionTargets);
		}
	}

#if WITH_EDITOR
	if (bDebugTrace)
	{
		DrawDebugSphere(
			GetWorld(),
			TraceStart,
			DetectionRadius,
			12,
			FColor::Blue,
			false,
			2.f,
			0,
			1.f
		);
	}
#endif // WITH_EDITOR
}

void UOrionInteractionComponent::StartInteraction()
{
	/* Make Sure Interaction Starts on Authority */
	if (GetInteractorRole() != ROLE_Authority)
	{
		Server_StartInteraction();
		return;
	}

	/* Prevent New Interaction If One Already In Progress */
	if (bInteracting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to Start Interaction Due to In Progress Interaction"));
		return;
	}

    /* Get Server Sided Interaction */
    CurrentInteractionTarget = GetInteractionTrace();

    if (!IsValid(CurrentInteractionTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to Start Interaction Due to No Interaction Available On Server Side"));
		return;
	}

	/* Start the Interaction */

	if (bIsInteractionEnabled &&
		CurrentInteractionTarget &&
		CurrentInteractionTarget->CanInteract())
	{
		if (CurrentInteractionTarget->DecoratorConditionMet())
		{
			SetInteracting(true);
			CurrentInteractionTarget->StartInteraction(GetOwningPawn());
			NotifyInteraction(EOrionInteractionResult::E_Started);
		}
		else
		{
			NotifyInteraction(EOrionInteractionResult::E_Blocked);
		}
	}
	
	
}

void UOrionInteractionComponent::Server_StartInteraction_Implementation()
{
	StartInteraction();
}

void UOrionInteractionComponent::CancelInteraction()
{
	if (GetInteractorRole() != ROLE_Authority)
	{
		Server_CancelInteraction();
		return;
	}
	if (!bInteracting || !IsValid(CurrentInteractionTarget))
	{
		return;
	}

	SetInteracting(false);

	if (CurrentInteractionTarget &&
		CurrentInteractionTarget->Mode == EOrionInteractionMode::Hold &&
		CurrentInteractionTarget->IsInteractionInProgress())
	{
		CurrentInteractionTarget->EndInteraction(false);
		NotifyInteraction(EOrionInteractionResult::E_Canceled);
	}
}

void UOrionInteractionComponent::Server_CancelInteraction_Implementation()
{
	CancelInteraction();
}


void UOrionInteractionComponent::EndInteraction(UOrionInteractionTargetComponent* InteractionTarget, bool bCompleted)
{
	check(InteractionTarget);

	SetInteracting(false);

	if (bCompleted)
	{
		NotifyInteraction(EOrionInteractionResult::E_Completed);
	}
}



