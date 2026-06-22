// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "Interaction/OrionInteractionTargetComponent.h"
#include "Interaction/OrionInteractionFunctionLibrary.h"
#include "Interaction/OrionInteractionComponent.h"
#include "OrionSetting.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"



UOrionInteractionTargetComponent::UOrionInteractionTargetComponent()
{
	InteractionNetMode = EOrionInteractionNetMode::E_Server;
	Mode = EOrionInteractionMode::Instant;
	HoldDuration = 1.0f;
	bFacingTarget = false;
	CurrentInteractor = nullptr;
	CurrentProgress = 0.0f;
	bAutoActivate = true;
	bShowActionDisplayText = true;
	SetIsReplicatedByDefault(true);
	//SetComponentTickEnabled(false);
}

void UOrionInteractionTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	
	bNativeCanInteract = true;
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->Tags.AddUnique(Tag.GetTagName());
	}

	//Begin event setup to load async any soft reference
	for(auto& Event : Events)
	{
		if (Event)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			Event->BeginSetup(PC, Pawn);
		}
	}

	//Bind Delegate
	Local_OnCompleted.AddDynamic(this, &ThisClass::OnLocalCompleted);
	
}

void UOrionInteractionTargetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentInteractor);
	DOREPLIFETIME(ThisClass, CurrentProgress);
}



bool UOrionInteractionTargetComponent::CanInteract() const
{
	return bNativeCanInteract && (Mode != EOrionInteractionMode::None) && (CurrentInteractor == nullptr) && FMath::IsNearlyEqual(CurrentProgress, 0.0f, UE_KINDA_SMALL_NUMBER);
}



bool UOrionInteractionTargetComponent::DecoratorConditionMet() const
{
	int SumConditionMet = 0;
	//Evaluate Decorator
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;

	for (auto& Condition : InteractConditions)
	{
		if (Condition && PC && Pawn)
		{
			bool bConditionMet = Condition && Condition->InvertCondition ? !Condition->PerformConditionCheck(PC, Pawn) : Condition->PerformConditionCheck(PC, Pawn);
			if (bConditionMet)
			{
				SumConditionMet++;
			}
		}
		
	}
	return SumConditionMet >= InteractConditions.Num();
}

FText UOrionInteractionTargetComponent::GetActionDisplayText_Implementation() const
{
	UOrionSetting* OrionSetting = GetMutableDefault<UOrionSetting>();
	FText ActionText = !ActionDisplayText.IsEmpty() ? ActionDisplayText : FText::FromString("Interact");

	return OrionSetting->bShowActionDisplayText && bShowActionDisplayText? ActionText : FText();
}


void UOrionInteractionTargetComponent::SetFocusState(bool bFocused, AActor* Interactor)
{// Blueprint event
	if (OnFocusChanged.IsBound())
	{
		OnFocusChanged.Broadcast(Interactor, this, bFocused);
	}
}

void UOrionInteractionTargetComponent::StartInteraction(AActor* Interactor)
{
	check(Mode != EOrionInteractionMode::None);

	CurrentInteractor = Interactor;
	CurrentProgress = 0.0f;
	CurrentHoldTime = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("Interaction STARTED! (Actor: %s, Interactor: %s)"), *GetOwner()->GetName(), *CurrentInteractor->GetName());

	// Blueprint event
	NotifyInteraction(EOrionInteractionResult::E_Started, CurrentInteractor);

	if (Mode == EOrionInteractionMode::Instant)
	{
		if (OnProcessed.IsBound())
		{
			OnProcessed.Broadcast(CurrentInteractor, this);
		}
		else
		{
			EndInteraction(true);
		}
	}
	else if (Mode == EOrionInteractionMode::Hold)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_UpdateProgressValue, this, &UOrionInteractionTargetComponent::UpdateProgressValue, 0.0333f, true);
	}

}


void UOrionInteractionTargetComponent::EndInteraction(bool bCompleted)
{
	check(Mode != EOrionInteractionMode::None);

	// Blueprint event
	if (bCompleted)
	{
		UE_LOG(LogTemp, Log, TEXT("Interaction COMPLETED! (Actor: %s, Interactor: %s)"), *GetOwner()->GetName(), *CurrentInteractor->GetName());

		
		
		NotifyInteraction(EOrionInteractionResult::E_Completed, CurrentInteractor);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Interaction CANCELED! (Actor: %s, Interactor: %s)"), *GetOwner()->GetName(), *CurrentInteractor->GetName());


		NotifyInteraction(EOrionInteractionResult::E_Canceled, CurrentInteractor);
	}

	UOrionInteractionComponent* InteractionComp = CurrentInteractor ? UOrionInteractionFunctionLibrary::GetInteractionComponentFromTarget(CurrentInteractor) : nullptr;
	if (InteractionComp)
	{
		InteractionComp->EndInteraction(this, bCompleted);
	}

	bNativeCanInteract = false;
	FTimerDelegate ResetInteractionTimerDel;
	TWeakObjectPtr<UOrionInteractionTargetComponent> WeakThis = this;
	//Ensure user dont spam interact, it will have .25 sec cooldown before it can be interacted again
	ResetInteractionTimerDel.BindLambda([this, WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				bNativeCanInteract = true;
			}
		});

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ResetInteraction,
		ResetInteractionTimerDel,
		0.25f,
		false
	);

	CurrentInteractor = nullptr;
	CurrentProgress = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_UpdateProgressValue);
	
}


bool UOrionInteractionTargetComponent::IsInteractionInProgress() const
{
	return (CurrentInteractor != nullptr) || (CurrentProgress > 0.0f && CurrentProgress < 1.0f);
}


void UOrionInteractionTargetComponent::UpdateInteractionProgress(float Progress)
{
	CurrentProgress = Progress;
}


void UOrionInteractionTargetComponent::UpdateProgressValue()
{
	CurrentHoldTime = FMath::Clamp(CurrentHoldTime + 0.0333f, 0.0f, HoldDuration);
	CurrentProgress = FMath::Clamp(CurrentHoldTime / HoldDuration, 0.f, 1.f);
	if (CurrentProgress >= 1.f)
	{
		CurrentProgress = 0.0f;
		CurrentHoldTime= 0.0f;
		EndInteraction(true);
	}
}



void UOrionInteractionTargetComponent::NotifyInteraction(EOrionInteractionResult NewInteractionResult, AActor* InInteractor)
{
	switch (InteractionNetMode)
	{
		case EOrionInteractionNetMode::E_Server:
			CallInteractionResult(NewInteractionResult, InInteractor);
			break;
		case EOrionInteractionNetMode::E_All:
			Multicast_NotifyInteraction(NewInteractionResult, InInteractor);
			break;
		default:
			break;
	}
}

void UOrionInteractionTargetComponent::Client_NotifyInteraction(EOrionInteractionResult NewInteractionResult, AActor* InInteractor)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	switch (NewInteractionResult)
	{
	case EOrionInteractionResult::E_Started:
		Local_OnStarted.Broadcast(InInteractor, this);
		break;
	case EOrionInteractionResult::E_Completed:
		Local_OnCompleted.Broadcast(InInteractor, this);
		break;
	case EOrionInteractionResult::E_Canceled:
		Local_OnCanceled.Broadcast(InInteractor, this);
		break;
	default:
		break;
	}
}

void UOrionInteractionTargetComponent::Multicast_NotifyInteraction_Implementation(EOrionInteractionResult NewInteractionResult, AActor* InInteractor)
{
	CallInteractionResult(
		NewInteractionResult,
		InInteractor
	);
}

void UOrionInteractionTargetComponent::CallInteractionResult(EOrionInteractionResult NewInteractionResult, AActor* InInteractor)
{

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	switch (NewInteractionResult)
	{
	case EOrionInteractionResult::E_Started:
		OnStarted.Broadcast(InInteractor, this);
		break;
	case EOrionInteractionResult::E_Completed:
		for (auto& Event : Events)
		{
			if (Event)
			{
				if (PC && Pawn)
				{
					Event->BeginEvent(PC, Pawn);
				}
			}
		}
		OnCompleted.Broadcast(InInteractor, this);
		break;
	case EOrionInteractionResult::E_Canceled:
		OnCanceled.Broadcast(InInteractor, this);
		break;
	default:
		break;
	}
}

void UOrionInteractionTargetComponent::OnLocalCompleted_Implementation(AActor* Interactor, UOrionInteractionTargetComponent* Component)
{

}