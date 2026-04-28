// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"
#include "Quest.h"
#include "OrionSaveGameSubsystem.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "UObject/UObjectIterator.h"
#include "OrionSaveGame.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode_State.h"
#include "QuestBuilderNode_Checkpoint.h"
#include "QuestBuilderNode_Root.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderSetting.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"
#include "QuestData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"


#define LOCTEXT_NAMESPACE "QuestComponent"

UQuestComponent::UQuestComponent()
{

	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(true);
	PrimaryComponentTick.bCanEverTick = true;
	bAutoNavigateQuest = true;
}


void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();
	QuestMap.Empty();
	QuestRegistryMap.Empty();
	QuestCategories.Empty();
	
	//Set Dependency
	OwningController = GetOwningController();
	GetQuestRegistryMap();

	InitializeDelegate();

	//Begin Default quest 
	for(auto& DefaultQuestGraph : DefaultQuestGraphs)
	{
		BeginQuestGraph(DefaultQuestGraph);
	}

	//Register Console Commands
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionLogQuest"),
		TEXT("Dump all quest and it's state"),
		FConsoleCommandDelegate::CreateUObject(this, &ThisClass::execDumpQuestLog),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionCompleteQuestObjectives"),
		TEXT("[QuestTag] Complete specific quest objectives"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::execCompleteQuestObjectives),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionActivateQuest"),
		TEXT("[QuestTag] Activate specific quest"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::execActivateQuest),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionCompleteQuest"),
		TEXT("[QuestTag] Complete specific quest"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::execCompleteQuest),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionFailQuest"),
		TEXT("[QuestTag] Fail specific quest"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::execFailQuest),
		ECVF_Default);
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	OnQuestUpdated.RemoveDynamic(this, &UQuestComponent::QuestUpdated);
	OnQuestObjectiveBegin.RemoveDynamic(this, &UQuestComponent::QuestObjectiveBegin);
	OnQuestObjectiveUpdated.RemoveDynamic(this, &UQuestComponent::QuestObjectiveUpdated);
	OnStartNavigateQuest.RemoveDynamic(this, &UQuestComponent::StartNavigatingQuest);
	OnQuestFailed.RemoveDynamic(this, &UQuestComponent::QuestFailed);
	OnQuestCompleted.RemoveDynamic(this, &UQuestComponent::QuestCompleted);
	OnQuestAdded.RemoveDynamic(this, &UQuestComponent::QuestAdded);
	OnQuestUnlocked.RemoveDynamic(this, &UQuestComponent::QuestUnlocked);
}

void UQuestComponent::InitializeDelegate()
{

	OnQuestUpdated.RemoveDynamic(this, &UQuestComponent::QuestUpdated);
	OnQuestObjectiveBegin.RemoveDynamic(this, &UQuestComponent::QuestObjectiveBegin);
	OnQuestObjectiveUpdated.RemoveDynamic(this, &UQuestComponent::QuestObjectiveUpdated);
	OnStartNavigateQuest.RemoveDynamic(this, &UQuestComponent::StartNavigatingQuest);
	OnQuestFailed.RemoveDynamic(this, &UQuestComponent::QuestFailed);
	OnQuestCompleted.RemoveDynamic(this, &UQuestComponent::QuestCompleted);
	OnQuestAdded.RemoveDynamic(this, &UQuestComponent::QuestAdded);
	OnQuestUnlocked.RemoveDynamic(this, &UQuestComponent::QuestUnlocked);

	OnQuestUpdated.AddDynamic(this, &UQuestComponent::QuestUpdated);
	OnQuestObjectiveBegin.AddDynamic(this, &UQuestComponent::QuestObjectiveBegin);
	OnQuestObjectiveUpdated.AddDynamic(this, &UQuestComponent::QuestObjectiveUpdated);
	OnStartNavigateQuest.AddDynamic(this, &UQuestComponent::StartNavigatingQuest);
	OnQuestFailed.AddDynamic(this, &UQuestComponent::QuestFailed);
	OnQuestCompleted.AddDynamic(this, &UQuestComponent::QuestCompleted);
	OnQuestAdded.AddDynamic(this, &UQuestComponent::QuestAdded);
	OnQuestUnlocked.AddDynamic(this, &UQuestComponent::QuestUnlocked);
}

void UQuestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

bool UQuestComponent::BeginQuestGraph(UQuestBuilderGraph* QuestGraphAsset)
{
	if (!GetWorld())
	{
		return false;
	}
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	UOrionSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UOrionSaveGameSubsystem>();
	if (SaveSubsystem && SaveSubsystem->IsLoadingSaveGame())
	{
		return false;
	}

	if (QuestGraphAsset == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Asset is null"));
		return false;
	}

	TWeakObjectPtr<UQuestComponent> WeakThis = this;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, WeakThis, QuestGraphAsset](float DeltaTime)
	{
		if (WeakThis.IsValid())
		{
			for (auto& QuestTemplate : QuestGraphAsset->QuestList)
			{
				UQuest* NewQuest = MakeQuestInstance(QuestTemplate);
				if (NewQuest)
				{
					EvaluateQuestState(NewQuest);
				}
			}

		}
		return false;
	}));


	return true;
	
}

class UQuest* UQuestComponent::MakeQuestInstance(UQuest* QuestTemplate)
{
	if (IsValid(QuestTemplate))
	{
		//Duplicate the quest template
		TObjectPtr<UQuest> NewQuest = Cast<UQuest>(StaticDuplicateObject(QuestTemplate, this, NAME_None, RF_Transactional));
		NewQuest->Initialize(this);
		if (QuestMap.FindRef(NewQuest->ID))
		{
			//UE_LOG(LogTemp, Log, TEXT("%s Already Registered."), *QuestTemplate->GetName());
			return nullptr;
		}

		//Add New Quest into Quest Categories
		TArray<FText> CategoryTextList;
		for (auto& QuestCategory : QuestCategories)
		{
			CategoryTextList.Add(QuestCategory.Category);
			if (QuestCategory.Category.EqualTo(NewQuest->QuestCategory))
			{
				QuestCategory.QuestList.AddUnique(NewQuest);			
			}
		}
		
        if (!CategoryTextList.ContainsByPredicate([&](const FText& CategoryText) { return CategoryText.EqualTo(NewQuest->QuestCategory); }))
        {
			FQuestCategory QuestCategory;
			QuestCategory.Category = NewQuest->QuestCategory;
			QuestCategory.QuestList.AddUnique(NewQuest);	
			QuestCategories.Add(QuestCategory);
		}
		QuestMap.Emplace(NewQuest->ID, NewQuest);
		return NewQuest;
	}

	return nullptr;
}

class UQuest* UQuestComponent::TryMakeQuestInstanceFromTag(FGameplayTag QuestTag)
{
	FName QuestID = QuestTag.GetTagName();

	TObjectPtr<UQuest> QuestPtr = GetQuestRegistryMap().FindRef(QuestID);
	UQuest* QuestTemplate = QuestPtr.Get();
	if (IsValid(QuestTemplate))
	{
		if (UQuest* QuestInstance = MakeQuestInstance(QuestTemplate))
		{
			return QuestInstance;
		}
	}

	return nullptr;
}

void UQuestComponent::AutoNavigateQuest()
{
	if (!bAutoNavigateQuest)
		return;
	

	TArray<FName> QuestKeys;
	QuestMap.GetKeys(QuestKeys);

	for (int32 i = 0; i < QuestKeys.Num(); i++)
	{
		FName Key = QuestKeys[i];
		UQuest* Quest = QuestMap.FindRef(Key);
		if (Quest && Quest->QuestState == EQuestState::E_Active)
		{
			StartNavigateQuest(Quest);
			return;
		}
	}
}

void UQuestComponent::UpdateAllQuest()
{
	TArray<FName> QuestKeys;
	QuestMap.GetKeys(QuestKeys);

	for (int32 i = 0; i < QuestKeys.Num(); i++)
	{
		FName Key = QuestKeys[i];
		UQuest* Quest = QuestMap.FindRef(Key);
		if (Quest && Quest->QuestState == EQuestState::E_Active)
		{
			Quest->UpdateQuest();
		}
	}

}

void UQuestComponent::ActivateQuestFromTag(FGameplayTag QuestTag, const bool bNotifyQuestAdded)
{
	FName QuestID = QuestTag.GetTagName();
	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	if (IsValid(QuestPtr.Get()))
	{
		ActivateQuest(QuestPtr.Get(), bNotifyQuestAdded);
	}
	else
    {
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: ActivateQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		ActivateQuest(TryMakeQuestInstanceFromTag(QuestTag), bNotifyQuestAdded);
	}
}

void UQuestComponent::UnlockQuestFromTag(FGameplayTag QuestTag)
{
	FName QuestID = QuestTag.GetTagName();
	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	if (IsValid(QuestPtr.Get()))
	{
		UnlockQuest(QuestPtr.Get());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: UnlockQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		UnlockQuest(TryMakeQuestInstanceFromTag(QuestTag));
	}
}

void UQuestComponent::LockQuestFromTag(FGameplayTag QuestTag)
{
	FName QuestID = QuestTag.GetTagName();
	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	if (IsValid(QuestPtr.Get()))
	{
		LockQuest(QuestPtr.Get());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: LockQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		LockQuest(TryMakeQuestInstanceFromTag(QuestTag));
	}
}

void UQuestComponent::RestartQuestFromTag(FGameplayTag QuestTag, FGameplayTag NodeTag, const bool bNotifyQuestAdded)
{
	FName QuestID = QuestTag.GetTagName();
	FName NodeID = NodeTag.GetTagName();

	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	UQuest* FoundedQuest = QuestPtr.Get();

	TObjectPtr<UQuestBuilderNode> NodePtr = FoundedQuest ? FoundedQuest->NodeMap.FindRef(NodeID) : nullptr;
	UQuestBuilderNode* FoundedNode = NodePtr.Get();
	if (IsValid(FoundedQuest))
	{
		RestartQuest(FoundedQuest, FoundedNode, bNotifyQuestAdded);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: RestartQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		RestartQuest(TryMakeQuestInstanceFromTag(QuestTag), nullptr, bNotifyQuestAdded);
	}
}

void UQuestComponent::FailQuestFromTag(FGameplayTag QuestTag)
{
	FName QuestID = QuestTag.GetTagName();
	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	if (IsValid(QuestPtr.Get()))
	{
		FailQuest(QuestPtr.Get());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: FailQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		FailQuest(TryMakeQuestInstanceFromTag(QuestTag));
	}
}

void UQuestComponent::CompleteQuestFromTag(FGameplayTag QuestTag)
{
	FName QuestID = QuestTag.GetTagName();
	TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(QuestID);
	if (IsValid(QuestPtr.Get()))
	{
		CompleteQuest(QuestPtr.Get());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuestComponent: CompleteQuestFromTag: %s not found, try make quest instance instead"), *QuestTag.ToString());
		QuestMap.Remove(QuestID);
		CompleteQuest(TryMakeQuestInstanceFromTag(QuestTag));
	}
}




void UQuestComponent::StartNavigateQuest(UQuest* InQuest, UQuestBuilderNode_Objective* InObjective, bool bOverrideCurrentNavigatedQuest)
{
	if (!bOverrideCurrentNavigatedQuest)
	{
		if (CurrentNavigatedQuest && CurrentNavigatedQuest->QuestState == EQuestState::E_Active)
		{
			return;
		}
	}


	if (InQuest && InQuest->QuestState == EQuestState::E_Active)
	{
		CurrentNavigatedQuest = InQuest;
		if (InObjective && InObjective->IsObjectiveActive())
		{
			InQuest->CurrentNavigatedObjective = InObjective;
		}
		else
		{
			InQuest->GetNavigatedObjective();
		}

		OnStartNavigateQuest.Broadcast(InQuest);

		UE_LOG(LogTemp, Log, TEXT("QuestComponent: Navigate Quest: %s, Tag: %s"),
			*InQuest->QuestName.ToString(),
			*InQuest->QuestTag.ToString()
		);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Navigate Quest. Quest not found or has Completed/Failed"));
	}
}

void UQuestComponent::ActivateQuest(UQuest* InQuest, const bool bNotifyQuestAdded, const bool bLaunchEventOnLoad)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Activate Quest, Quest not found or invalid pointer"));
		return;
	}
	
	InQuest->ActivateQuest(bNotifyQuestAdded, bLaunchEventOnLoad);

	UE_LOG(LogTemp, Log, TEXT("QuestComponent: Activate Quest: %s, Tag: %s"),
		*InQuest->QuestName.ToString(),
		*InQuest->QuestTag.ToString()
	);

	

}

void UQuestComponent::UnlockQuest(UQuest* InQuest)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Unlock Quest, Quest not found or invalid pointer"));
		return;
	}

	if (InQuest->QuestState == EQuestState::E_Locked)
	{
		InQuest->QuestState = EQuestState::E_Unlocked;
		OnQuestUnlocked.Broadcast(InQuest);

		UE_LOG(LogTemp, Log, TEXT("QuestComponent: Unlock Quest: %s, Tag: %s"),
			*InQuest->QuestName.ToString(),
			*InQuest->QuestTag.ToString()
		);
	}
}

void UQuestComponent::LockQuest(UQuest* InQuest)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Lock Quest, Quest not found or invalid pointer"));
		return;
	}

	InQuest->QuestState = EQuestState::E_Locked;
	InQuest->CurrentNodes.Empty();
	InQuest->VisitedNodeIDs.Empty();

	UE_LOG(LogTemp, Log, TEXT("QuestComponent: Lock Quest: %s, Tag: %s"),
		*InQuest->QuestName.ToString(),
		*InQuest->QuestTag.ToString()
	);

}

void UQuestComponent::RestartQuest(UQuest* InQuest, UQuestBuilderNode* InNode, const bool bNotifyQuestAdded)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Restart Quest, Quest not found or invalid pointer"));
		return;
	}

	InQuest->RestartQuest(InNode, bNotifyQuestAdded);

	UE_LOG(LogTemp, Log, TEXT("QuestComponent: Restart Quest: %s, Tag: %s"),
		*InQuest->QuestName.ToString(),
		*InQuest->QuestTag.ToString()
	);

}

void UQuestComponent::FailQuest(UQuest* InQuest)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Fail Quest, Quest not found or invalid pointer"));
		return;
	}

	if (InQuest->QuestState == EQuestState::E_Active)
	{
		//fail all current nodes
		for (int32 i = 0; i < InQuest->CurrentNodes.Num(); i++)
		{
			if (InQuest->CurrentNodes.IsValidIndex(i))
			{
				if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(InQuest->CurrentNodes[i]))
				{
					ObjectiveNode->ObjectiveState = EObjectiveState::E_Failed;
				}
			}
		}
		InQuest->QuestState = EQuestState::E_Fail;
		InQuest->CurrentNodes.Empty();

		OnQuestFailed.Broadcast(InQuest);
		OnQuestUpdated.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("QuestComponent: Fail Quest: %s, Tag: %s"),
			*InQuest->QuestName.ToString(),
			*InQuest->QuestTag.ToString()
		);
	}


}

void UQuestComponent::CompleteQuest(UQuest* InQuest)
{
	if (!IsValid(InQuest))
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent : Failed to Complete Quest, Quest not found or invalid pointer"));
		return;
	}

	if (InQuest->QuestState == EQuestState::E_Active)
	{
		//fail all current nodes
		for (int32 i = 0; i < InQuest->CurrentNodes.Num(); i++)
		{
			if (InQuest->CurrentNodes.IsValidIndex(i))
			{
				if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(InQuest->CurrentNodes[i]))
				{
					ObjectiveNode->ObjectiveState = EObjectiveState::E_Completed;
				}
			}
		}
		InQuest->QuestState = EQuestState::E_Complete;

		InQuest->CurrentNodes.Empty();

		OnQuestCompleted.Broadcast(InQuest);
		OnQuestUpdated.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("QuestComponent: Complete Quest: %s, Tag: %s"),
			*InQuest->QuestName.ToString(),
			*InQuest->QuestTag.ToString()
		);
	}
}

bool UQuestComponent::IsQuestAtState(FGameplayTag QuestTag, EQuestState QuestState)
{
	UQuest* Quest = FindQuest(QuestTag);

	if (Quest && Quest->QuestState == QuestState)
	{
		return true;
	}

	return false;
}


APawn* UQuestComponent::GetOwningPawn() const
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

APlayerController* UQuestComponent::GetOwningController() const
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





void UQuestComponent::StopNavigateQuest()
{
	CurrentNavigatedQuest = nullptr;
	OnStopNavigateQuest.Broadcast();
}


UQuest* UQuestComponent::FindQuest(FGameplayTag QuestTag) const
{
	FName QuestID = QuestTag.GetTagName();
	if (UQuest* FoundedQuest = QuestMap.FindRef(QuestID))
	{
		return FoundedQuest;
	}

	UE_LOG(LogTemp, Log, TEXT("QuestComponent: Quest Not Found"));
	return nullptr;
}

void UQuestComponent::ResetAllQuest()
{
	TArray<FName> QuestKeys;
	QuestMap.GetKeys(QuestKeys);

	for (int32 j = QuestKeys.Num() - 1; j >= 0; --j)
	{
		FName Key = QuestKeys[j];
		TObjectPtr<UQuest> QuestPtr = QuestMap.FindRef(Key);
		UQuest* Quest = QuestPtr.Get();
		if (IsValid(Quest))
		{
			Quest->Deinitialize();

			Quest->CurrentNodes.Empty();
		}
		QuestMap.Remove(Key);
	}
	QuestMap.Empty();
	QuestCategories.Empty();
	QuestRegistryMap.Empty();
	CurrentNavigatedQuest = nullptr;
}

void UQuestComponent::EvaluateQuestState(UQuest* QuestParam)
{
	if (!IsValid(QuestParam))
	{
		return;
	}
	if (QuestParam->QuestState != EQuestState::E_Locked)
	{
		// only evaluate if the quest state is locked
		return;
	}

	switch (QuestParam->DesiredQuestState)
	{
	case EQuestState::E_Active:
		ActivateQuest(QuestParam);
		return;
	case EQuestState::E_Fail:
		FailQuest(QuestParam);
		return;
	case EQuestState::E_Complete:
		CompleteQuest(QuestParam);
		return;
	case EQuestState::E_Unlocked:
		UnlockQuest(QuestParam);
		return;
	}

}

FOrionQuestSaveData UQuestComponent::GetSaveGameData()
{
	FOrionQuestSaveData QuestSaveData;
	for (const auto& It : QuestMap)
	{
		FName Key = It.Key;
		UQuest* Quest = It.Value;
		QuestSaveData.QuestAssetIDs.Add(Quest->ID);
		TMap<FName, FNodeData> NodeMapData;
		for (auto& Node : Quest->AllNodes)
		{
			FNodeData NodeData;
			if (Node)
			{
				NodeData.NodeID = Node->ID;
				NodeData.bWaitForBranching = Node->bWaitForBranching;
				if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(Node))
				{
					NodeData.CurrentProgress = ObjectiveNode->CurrentProgress;
					NodeData.ObjectiveState = ObjectiveNode->ObjectiveState;
				}
			}
			NodeMapData.Emplace(Node->ID, NodeData);
		}
		FQuestData QuestData;
		for (int i = 0; i < Quest->CurrentNodes.Num(); i++)
		{
			if (Quest->CurrentNodes.IsValidIndex(i))
			{
				QuestData.CurrentNodeIDs.AddUnique(Quest->CurrentNodes[i]->ID);
			}
		}

		QuestData.QuestState = Quest->QuestState;
		QuestData.NodeMapData = NodeMapData;
		QuestData.VisitedNodeIDs = Quest->VisitedNodeIDs;
		QuestSaveData.QuestMapData.Emplace(Quest->ID, QuestData);
	}
	if (CurrentNavigatedQuest)
	{
		QuestSaveData.NavigatedQuestID = CurrentNavigatedQuest->ID;
		if (CurrentNavigatedQuest->CurrentNavigatedObjective)
		{
			QuestSaveData.NavigatedObjectiveID = CurrentNavigatedQuest->CurrentNavigatedObjective->ID;
		}
	}
	CurrentQuestSaveData = QuestSaveData;
	return QuestSaveData;
}

void UQuestComponent::InitializeFromSaveGame(FOrionQuestSaveData& QuestSaveData)
{
	CurrentQuestSaveData = QuestSaveData;
	TArray<UQuest*> LoadedQuestAssets;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> QuestGraphDataArray;
	AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestGraphDataArray);

	//reset all quest before load the new quest graph asset
	ResetAllQuest();

	//Begin Questasset which their asset id has registered
	for (const FAssetData& AssetData : QuestGraphDataArray)
	{
		UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
		if (QuestGraphAsset)
		{
			for (auto& QuestTemplate : QuestGraphAsset->QuestList)
			{
				if (QuestSaveData.QuestAssetIDs.Contains(QuestTemplate->ID) ||
					QuestSaveData.QuestAssetIDs.Contains(QuestTemplate->QuestTag.GetTagName()))
				{
					//make quest instance from template found
					if (UQuest* LoadedQuestAsset = MakeQuestInstance(QuestTemplate))
					{
						LoadedQuestAssets.Add(LoadedQuestAsset);
					}
				}
			}
		}
	}

	TMap<FName, UQuest*> QuestLoadMap;
	for (auto& Quest : LoadedQuestAssets)
	{
		QuestLoadMap.Add(Quest->ID, Quest);
	}

	for (const auto& QuestIt : QuestSaveData.QuestMapData)
	{
		FName ID = QuestIt.Key;
		FQuestData QuestData = QuestIt.Value;

		UQuest* FoundedQuest = QuestLoadMap.FindRef(ID);
		if (FoundedQuest)
		{
			//Fill Quest Data Here
			FoundedQuest->QuestState = QuestData.QuestState;
			for (int i = 0; i < QuestData.CurrentNodeIDs.Num(); i++)
			{
				if (QuestData.CurrentNodeIDs.IsValidIndex(i))
				{
					if (UQuestBuilderNode* FoundedCurrentNode = FoundedQuest->NodeMap.FindRef(QuestData.CurrentNodeIDs[i]))
					{
						FoundedQuest->CurrentNodes.AddUnique(FoundedCurrentNode);
					}
				}
			}
			FoundedQuest->VisitedNodeIDs = QuestData.VisitedNodeIDs;

			for (const auto& NodeIt : QuestData.NodeMapData)
			{
				FName NodeID = NodeIt.Key;
				FNodeData NodeData = NodeIt.Value;

				UQuestBuilderNode* FoundedNode = FoundedQuest->NodeMap.FindRef(NodeID);
				if (FoundedNode)
				{
					//Fill Node Data Here
					FoundedNode->bWaitForBranching = NodeData.bWaitForBranching;
					if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(FoundedNode))
					{
						ObjectiveNode->CurrentProgress = NodeData.CurrentProgress;
						ObjectiveNode->ObjectiveState = NodeData.ObjectiveState;
					}
				}
			}
		}
	}
	InitializeDelegate();//remove and add delegate binding, so other destroyed object that bind to the delegate will also be removed
	StopNavigateQuest();
	for (auto& LoadedQuestAsset : LoadedQuestAssets)
	{
		EvaluateQuestState(LoadedQuestAsset);
		if (!LoadedQuestAsset->CurrentNodes.IsEmpty() && LoadedQuestAsset->QuestState == EQuestState::E_Active)
		{
			ActivateQuest(LoadedQuestAsset, false /*bNotifyQuestAdded*/, true /*bLaunchEventOnLoad*/);
		}
	}


}




bool UQuestComponent::DeleteSave(const FString& SaveName, const int32 Slot)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(SaveName, Slot);
}


TMap<FName, TObjectPtr<UQuest>> UQuestComponent::GetQuestRegistryMap()
{
	if (QuestRegistryMap.Num() == 0)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		TArray<FAssetData> QuestGraphDataArray;
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestGraphDataArray);

		// Only clear once before populating.
		QuestRegistryMap.Empty();

		for (const FAssetData& AssetData : QuestGraphDataArray)
		{
			UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
			if (QuestGraphAsset)
			{
				for (const auto& Quest : QuestGraphAsset->QuestList)
				{
					if (IsValid(Quest))
					{
						if (Quest->QuestTag.IsValid())
						{
							QuestRegistryMap.Emplace(Quest->QuestTag.GetTagName(), Quest);
						}
						else
						{
							QuestRegistryMap.Emplace(Quest->ID, Quest);
						}
					}
				}
			}
		}
	}
	return QuestRegistryMap;
}

void UQuestComponent::QuestUpdated()
{
	UpdateAllQuest();
}

void UQuestComponent::QuestObjectiveBegin(UQuestBuilderNode_Objective* InObjective)
{
}


void UQuestComponent::QuestObjectiveUpdated(UQuest* Quest)
{
}

void UQuestComponent::StartNavigatingQuest(UQuest* Quest)
{

}

void UQuestComponent::QuestFailed(UQuest* Quest)
{
	AutoNavigateQuest();
}

void UQuestComponent::QuestCompleted(UQuest* Quest)
{
	AutoNavigateQuest();
}

void UQuestComponent::QuestAdded(UQuest* Quest)
{
	if (Quest)
	{
		UE_LOG(LogTemp, Log, TEXT("QuestComponent: Quest Added: %s"), *Quest->ID.ToString());
	}
}

void UQuestComponent::QuestUnlocked(UQuest* Quest)
{
}


// ================================================================================================ //
// CONSOLE COMMANDS
// ================================================================================================ //
void UQuestComponent::execDumpQuestLog()
{
#if !UE_BUILD_SHIPPING

	static FString Message;
	Message.Reset();

	Message += TEXT("\n==================== BEGIN Quest ====================\n");

	Message += CurrentNavigatedQuest ? FString::Printf(TEXT("Navigated Quest: %s \n\n"), *CurrentNavigatedQuest->QuestName.ToString()) : FString();

	for (int32 i = 0; i < QuestCategories.Num(); ++i)
	{
		const FQuestCategory& QuestCategory = QuestCategories[i];

		const FString CategoryStr = QuestCategory.Category.ToString();
		Message += FString::Printf(TEXT("Category: %s\n"), *CategoryStr);
		for (auto& Quest : QuestCategory.QuestList)
		{
			const FString QuestStateStr = UEnum::GetValueAsString(Quest->QuestState);
			Message += FString::Printf(TEXT("\tQuest: %s | Tag: %s, State: %s\n"), *Quest->QuestName.ToString(), *Quest->ID.ToString(), *QuestStateStr);
			for (auto& Objective : Quest->GetCurrentObjectives())
			{
				const FString ObjectiveStateStr = UEnum::GetValueAsString(Objective->ObjectiveState);
				Message += FString::Printf(TEXT("\t\tObjective: %s, Progress: %d/%d\n"), *Objective->Description.ToString(), Objective->CurrentProgress, Objective->RequiredAmount);
			}
		}
	}


	Message += TEXT("==================== END Quest ====================\n");

	UE_LOG(LogTemp, Log, TEXT(" %s"), *Message);

#endif // UE_BUILD_SHIPPING
}

void UQuestComponent::execCompleteQuestObjectives(const TArray<FString>& QuestTag)
{
	if (QuestTag.IsEmpty()) return;

	FName QuestID = FName(*QuestTag[0]);
	if (UQuest* FoundedQuest = QuestMap.FindRef(QuestID))
	{
		for (auto& CurrObjective : FoundedQuest->GetCurrentObjectives())
		{
			CurrObjective->CompleteObjective();
		}
	}

}

void UQuestComponent::execActivateQuest(const TArray<FString>& QuestTag)
{
	if (QuestTag.IsEmpty()) return;

	ActivateQuestFromTag(FGameplayTag::RequestGameplayTag(FName(*QuestTag[0])));
}

void UQuestComponent::execCompleteQuest(const TArray<FString>& QuestTag)
{
	if (QuestTag.IsEmpty()) return;

	CompleteQuestFromTag(FGameplayTag::RequestGameplayTag(FName(*QuestTag[0])));
}

void UQuestComponent::execFailQuest(const TArray<FString>& QuestTag)
{
	if (QuestTag.IsEmpty()) return;

	FailQuestFromTag(FGameplayTag::RequestGameplayTag(FName(*QuestTag[0])));
}

#undef LOCTEXT_NAMESPACE
