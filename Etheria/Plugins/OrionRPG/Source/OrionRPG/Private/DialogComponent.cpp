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

		const TWeakObjectPtr<UDialogComponent> WeakThis(this);
		const TWeakObjectPtr<UDialogBuilderGraph> WeakDialogGraph(NewDialogGraph);

		NewDialogGraph->OnDialogSetupFinished.Clear();
		NewDialogGraph->OnDialogSetupFinished.AddLambda([WeakThis, WeakDialogGraph]()
			{
				UDialogComponent* DialogComponent = WeakThis.Get();
				UDialogBuilderGraph* DialogGraph = WeakDialogGraph.Get();

				if (!DialogComponent || !DialogGraph || DialogComponent->CurrentActiveDialog != DialogGraph)
				{
					return;
				}

				auto BeginDialogFlow = [DialogComponent, DialogGraph]()
					{
						DialogGraph->StartFromRoot();
						DialogComponent->OnBeginDialog.Broadcast(DialogGraph);
					};

				if (DialogGraph->bFadeCameraOnDialogBegin)
				{
					DialogComponent->OnBeginDialog.Broadcast(DialogGraph);
					APlayerController* LocalController = DialogComponent->OwningController;
					if (LocalController && LocalController->IsLocalPlayerController())
					{
						if (APlayerCameraManager* PCM = LocalController->PlayerCameraManager)
						{
							PCM->StartCameraFade(1.0f, 1.0f, 0.1f, FLinearColor::Black, false, true);

							const float HoldDurationSeconds = 1.0f;
							FTimerDelegate FadeDel = FTimerDelegate::CreateLambda([WeakThis, WeakDialogGraph]()
								{
									UDialogComponent* FadeDialogComponent = WeakThis.Get();
									UDialogBuilderGraph* FadeDialogGraph = WeakDialogGraph.Get();

									if (!FadeDialogComponent || !FadeDialogGraph || FadeDialogComponent->CurrentActiveDialog != FadeDialogGraph)
									{
										return;
									}

									if (APlayerController* FadeController = FadeDialogComponent->OwningController)
									{
										if (APlayerCameraManager* FadePCM = FadeController->PlayerCameraManager)
										{
											// Unfade first, then begin node
											FadePCM->StartCameraFade(1.0f, 0.0f, 0.75f, FLinearColor::Black, false, true);
										}
									}

									FadeDialogGraph->StartFromRoot();
								});

							if (DialogComponent->GetWorld())
							{
								FTimerHandle FadeHandle;
								DialogComponent->GetWorld()->GetTimerManager().SetTimer(FadeHandle, FadeDel, HoldDurationSeconds, false);
							}
							return;
						}
					}
				}

				BeginDialogFlow();
			});

		NewDialogGraph->StartSetupPrerequisites();
		bSuccess = true;
	}

	return bSuccess;
}

void UDialogComponent::SelectDialogChoice(UDialogBuilderNode_PlayerChoice* PlayerChoice, int32 ChoiceIndex)
{
	if (CurrentActiveDialog && PlayerChoice)
	{
		OnPLayerChoiceSelected.Broadcast(PlayerChoice, ChoiceIndex);
		PlayerChoice->SelectChoice(ChoiceIndex);
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

AActor* UDialogComponent::GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition)
{
	if(CurrentActiveDialog)
	{
		return CurrentActiveDialog->GetDialogDefinitionActor(InDialogDefinition);
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

void UDialogComponent::EnterChoiceSelection(UDialogBuilderNode_PlayerChoice* InPLayerChoice)
{
}

void UDialogComponent::PlayerChoiceSelected(UDialogBuilderNode_PlayerChoice* PlayerOption, int32 ChoiceIndex)
{
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