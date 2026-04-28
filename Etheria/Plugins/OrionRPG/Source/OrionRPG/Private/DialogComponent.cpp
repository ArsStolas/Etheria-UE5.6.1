// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogComponent.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"
#include "DialogData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"




#define LOCTEXT_NAMESPACE "DialogComponent"

UDialogComponent::UDialogComponent()
{
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(true);
	PrimaryComponentTick.bCanEverTick = true;
}

void UDialogComponent::BeginPlay()
{
	Super::BeginPlay();
	DialogMap.Empty();
	//Set Dependency
	OwningController = GetOwningController();
	
	OnDialogUpdated.AddDynamic(this, &UDialogComponent::DialogLineUpdated);
	OnEnterChoiceSelection.AddDynamic(this, &UDialogComponent::EnterChoiceSelection);
	OnPLayerChoiceSelected.AddDynamic(this, &UDialogComponent::PlayerChoiceSelected);
	OnBeginDialog.AddDynamic(this, &UDialogComponent::DialogBegin);
	OnEndDialog.AddDynamic(this, &UDialogComponent::DialogEnd);
}


void UDialogComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

bool UDialogComponent::BeginDialog(UDialogBuilderGraph* DialogAsset)
{
	if (DialogAsset == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialog Asset is null"));
		return false;
	}
	if (CurrentActiveDialog && CurrentActiveDialog->bCanInterruptDialog == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Can't begin dialog, make sure current active dialog has finished before starting another dialog"));
		return false;
	}

	bool bSuccess = false;

	UDialogBuilderGraph* NewDialogGraph = MakeDialogGraphInstance(DialogAsset);
	if (NewDialogGraph)
	{
		if (CurrentActiveDialog)
		{
			CurrentActiveDialog->EndDialog();
		}
		NewDialogGraph->Initialize(this);
		CurrentActiveDialog = NewDialogGraph;

		if (NewDialogGraph->bUseDialogCameraFade)
		{
			if (OwningController && OwningController->IsLocalPlayerController())
			{
				if (APlayerCameraManager* PCM = OwningController->PlayerCameraManager)
				{
					PCM->StartCameraFade(1.0f, 1.0f, .1f, NewDialogGraph->FadeColor, false, true);

					const float HoldDurationSeconds = 2.0f;

					FTimerDelegate FadeDel = FTimerDelegate::CreateLambda([this, PCM, NewDialogGraph]()
						{
							if (PCM)
							{
								// Fade from black (1.0) back to clear (0.0)
								PCM->StartCameraFade(1.0f, 0.0f, NewDialogGraph->FadeDuration, NewDialogGraph->FadeColor, false, false);
							}
							NewDialogGraph->StartFromRoot();
							OnBeginDialog.Broadcast(NewDialogGraph);
						});

					if (GetWorld())
					{
						FTimerHandle FadeHandle;
						GetWorld()->GetTimerManager().SetTimer(FadeHandle, FadeDel, HoldDurationSeconds, false);
					}
				}
			}
		}
		else
		{
			NewDialogGraph->StartFromRoot();
			OnBeginDialog.Broadcast(NewDialogGraph);
		}

		bSuccess = true;
	}
	
	return bSuccess;
}

void UDialogComponent::SelectDialogChoice(UDialogBuilderNode_PlayerChoice* InOption)
{
	if (CurrentActiveDialog && InOption)
	{
		CurrentActiveDialog->bOptionSelectionActive = false;
		CurrentActiveDialog->BeginNode((UDialogBuilderNode*)InOption);
		OnPLayerChoiceSelected.Broadcast(InOption);
	}
}

class UDialogBuilderGraph* UDialogComponent::MakeDialogGraphInstance(UDialogBuilderGraph* DialogGraphTemplate)
{
	if (IsValid(DialogGraphTemplate))
	{
		//Duplicate the dialog graph
		UDialogBuilderGraph* NewDialogGraph = Cast<UDialogBuilderGraph>(StaticDuplicateObject(DialogGraphTemplate, this, NAME_None, RF_Transactional));
		return NewDialogGraph;
	}

	return nullptr;
}

APawn* UDialogComponent::GetOwningPawn() const
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

APlayerController* UDialogComponent::GetOwningController() const
{
	//We cache this on beginplay as to not re-find it every time 
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

void UDialogComponent::Save_Implementation(const FString& SaveFileName, int SlotIndex)
{
}

void UDialogComponent::Load_Implementation(const FString& SaveFileName, int SlotIndex) 
{
	
}

bool UDialogComponent::DeleteSave(const FString& SaveName, const int32 Slot)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(SaveName, Slot);
}


void UDialogComponent::DialogLineUpdated(UDialogBuilderNode* DialogNode)
{
}

void UDialogComponent::EnterChoiceSelection(const TArray<class UDialogBuilderNode_PlayerChoice*>& PlayerOptions)
{
}

void UDialogComponent::PlayerChoiceSelected(UDialogBuilderNode_PlayerChoice* PlayerOption)
{
    UE_LOG(LogTemp, Log, TEXT("Dialog Option Selected: %s"), *PlayerOption->ChoiceText.ToString());
}

void UDialogComponent::DialogBegin(UDialogBuilderGraph* Dialog)
{
	if (Dialog)
	{
		UE_LOG(LogTemp, Log, TEXT("Begin Dialog: %s"), *Dialog->ID.ToString());
	}
}

void UDialogComponent::DialogEnd(UDialogBuilderGraph* Dialog)
{
	CurrentActiveDialog = nullptr;
	if (Dialog)
	{
		UE_LOG(LogTemp, Log, TEXT("End Dialog: %s"), *Dialog->ID.ToString());
	}
}

#undef LOCTEXT_NAMESPACE