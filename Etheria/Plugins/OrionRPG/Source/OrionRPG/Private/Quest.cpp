// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Quest.h"

#include "OrionRPG.h"
#include "QuestComponent.h"
#include "QuestBuilderSetting.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderEdge.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderNode_Root.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Containers/Queue.h"
#include "AssetRegistry/AssetRegistryModule.h"


#define LOCTEXT_NAMESPACE "Quest"


class FOrionRPGModule;

UQuest::UQuest()
{
	bSetupCompleted = false;
	bCanQuestBeAborted = false;
	bCanRetakeQuest = false;
	QuestState = EQuestState::E_Locked;
	
}

bool UQuest::Initialize(UQuestComponent* InitializingComp)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		//We need a valid Quest component to make a quest for 
		if (!InitializingComp)
		{
			return false;
		}
		OwningController = InitializingComp->GetOwningController();
		QuestComponent = InitializingComp;
		//ensure id matches with tag
		if (QuestTag.IsValid())
		{
			ID = QuestTag.GetTagName();
		}
		NodeMap.Reset();
		for (auto& Node : AllNodes)
		{
			if (Node)
			{
				Node->Quest = this;
				Node->OwningController = InitializingComp->GetOwningController();
				Node->QuestComponent = InitializingComp;
				
				if (Node->NodeTag.IsValid())
				{
					Node->ID = Node->NodeTag.GetTagName();
				}
				NodeMap.Emplace(Node->ID, Node);
			}
		}
		return true;
	}
	return false;
}

void UQuest::Deinitialize()
{
	VisitedNodeIDs.Empty();
	CurrentNodes.Empty();
	CurrentNavigatedObjective = nullptr;
	QuestComponent = nullptr;
	OwningController = nullptr;


	for (auto& Node : AllNodes)
	{
		if (IsValid(Node))
		{
			Node->Quest = nullptr;
			Node->QuestComponent = nullptr;
			Node->OwningController = nullptr;
			Node->ParentNodes.Empty();
			Node->ChildrenNodes.Empty();
			Node->Edges.Empty();
			Node->Reset();
		}
	}


	AllNodes.Empty();
	RootNodes.Empty();
	NodeMap.Empty();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_CheckPrerequisites);
	}
}

UWorld* UQuest::GetWorld() const
{
	if (QuestComponent)
	{
		return QuestComponent->GetWorld();
	}

	return nullptr;
}

void UQuest::UpdateQuest()
{
	if (QuestState != EQuestState::E_Active)
		return;
	
	//Cache Current nodes, because some node may be removed from current nodes after branching, 
	TArray<UQuestBuilderNode*> CachedCurrentNodes = CurrentNodes;
	for (int i = 0; i < CachedCurrentNodes.Num(); i++)
	{
		if (CachedCurrentNodes.IsValidIndex(i) && CurrentNodes.Contains(CachedCurrentNodes[i]))
		{
			CachedCurrentNodes[i]->EvaluateNextNode();
		}
	}

}

TArray<UQuestBuilderNode*> UQuest::GetVisitedNodes()
{
	SortVisitedNodes();

	TArray<UQuestBuilderNode*> ReturnNodes;
	for (auto& VisitedNodeID : VisitedNodeIDs)
	{
		UQuestBuilderNode* QuestNode = NodeMap.FindRef(VisitedNodeID);
		ReturnNodes.AddUnique(QuestNode);
	}
	return ReturnNodes;
}

TArray<UQuestBuilderNode_Objective*> UQuest::GetCurrentObjectives()
{
	TArray<UQuestBuilderNode_Objective*> ReturnNodes;
	for (auto& CurrNode : CurrentNodes)
	{
		UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(CurrNode);
		if (ObjectiveNode && ObjectiveNode->IsObjectiveActive())
		{
			ReturnNodes.AddUnique(ObjectiveNode);
		}
	}
	return ReturnNodes;
}

void UQuest::SortVisitedNodes()
{
	//Sort Visited nodes in this order:
	// Ongoing Objectives > Completed Objectives > Failed Objective
	TArray<FName> CachedVisitedNodeIDs;
	for (auto& VisitedNodeID : VisitedNodeIDs)
	{
		UQuestBuilderNode* QuestNode = NodeMap.FindRef(VisitedNodeID);
		if (QuestNode && !QuestNode->bHidden)
		{
			UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(QuestNode);
			if (ObjectiveNode && ObjectiveNode->IsObjectiveFailed())
			{
				CachedVisitedNodeIDs.Add(QuestNode->ID);
			}
		}
	}
	for (auto& VisitedNodeID : VisitedNodeIDs)
	{
		UQuestBuilderNode* QuestNode = NodeMap.FindRef(VisitedNodeID);
		if (QuestNode && !QuestNode->bHidden)
		{
			UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(QuestNode);
			if (ObjectiveNode && ObjectiveNode->IsObjectiveCompleted())
			{
				CachedVisitedNodeIDs.Add(QuestNode->ID);
			}
		}
	}
	
	for (auto& VisitedNodeID : VisitedNodeIDs)
	{
		UQuestBuilderNode* QuestNode = NodeMap.FindRef(VisitedNodeID);
		if (QuestNode && !QuestNode->bHidden)
		{
			if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(QuestNode))
			{
				if (ObjectiveNode->IsObjectiveActive())
				{
					CachedVisitedNodeIDs.Add(QuestNode->ID);
				}
			}
			else
			{
				CachedVisitedNodeIDs.Add(QuestNode->ID);
			}
		}
	}
	VisitedNodeIDs = CachedVisitedNodeIDs;
}


void UQuest::ActivateQuest(const bool bNotifyQuestAdded, const bool bLaunchEventOnLoad)
{
	if (bCanRetakeQuest &&  (QuestState == EQuestState::E_Complete || QuestState == EQuestState::E_Fail))
	{
		RestartQuest(nullptr, bNotifyQuestAdded);
	}

	QuestState = EQuestState::E_Active;
	if (CurrentNodes.IsEmpty())
	{
		//reset all Node
		for (auto& Node : AllNodes)
		{
			Node->Reset();
		}
		VisitedNodeIDs.Empty();
	}

	TWeakObjectPtr<UQuest> WeakThis = this;
	OnQuestSetupFinished.AddLambda(
		[this, WeakThis, bNotifyQuestAdded, bLaunchEventOnLoad]()
		{
			if (WeakThis.IsValid() && QuestComponent)
			{
				if (CurrentNodes.IsEmpty())
				{
					if (RootNodes.IsValidIndex(0))
					{
						BeginNode(RootNodes[0]);
					}
				}
				else
				{
					BeginMultipleNodes(CurrentNodes, bLaunchEventOnLoad);
				}
				if (bNotifyQuestAdded)
				{
					QuestComponent->OnQuestAdded.Broadcast(this);
				}

				bool bShouldNavigateThisQuest = QuestComponent->CurrentQuestSaveData.NavigatedQuestID == (QuestTag.IsValid() ? QuestTag.GetTagName() : ID);
				UQuestBuilderNode_Objective* FoundedObjective = Cast<UQuestBuilderNode_Objective>(NodeMap.FindRef(QuestComponent->CurrentQuestSaveData.NavigatedObjectiveID));

				if (bShouldNavigateThisQuest && QuestState == EQuestState::E_Active)
				{
					QuestComponent->StartNavigateQuest(this, FoundedObjective, true);
				}
				else
				{
					QuestComponent->AutoNavigateQuest();
				}
				OnQuestSetupFinished.Clear();
			}
		}
	);

	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(TimerHandle_CheckPrerequisites))
	{
		World->GetTimerManager().SetTimer(TimerHandle_CheckPrerequisites, this, &UQuest::CheckQuestPrerequisites, .2f, true);
	}
	
}


void UQuest::RestartQuest(UQuestBuilderNode* InNode, const bool bNotifyQuestAdded)
{
	QuestState = EQuestState::E_Active;

	if (InNode)
	{
		//Reset all visited node that is not visited by FoundedNode
		for (int32 i = VisitedNodeIDs.Num() - 1; i >= 0; --i)
		{
			FName CachedVisitedNodeID = VisitedNodeIDs[i];
			if (UQuestBuilderNode* VisitedNode = NodeMap.FindRef(CachedVisitedNodeID))
			{
				VisitedNode->Reset();
			}
			VisitedNodeIDs.RemoveAt(i);
			
			if (CachedVisitedNodeID == InNode->NodeTag.GetTagName())
				break;
		}
		CurrentNodes.Empty();
	}
	else
	{
		//reset all Node
		for (auto& Node : AllNodes)
		{
			Node->Reset();
		}
		VisitedNodeIDs.Empty();
		CurrentNodes.Empty();
	}

	TWeakObjectPtr<UQuest> WeakThis = this;
	OnQuestSetupFinished.AddLambda(
		[this, WeakThis, InNode, bNotifyQuestAdded]()
		{
			if(WeakThis.IsValid() && QuestComponent)
			{
				if (bNotifyQuestAdded)
				{
					QuestComponent->OnQuestAdded.Broadcast(this);
				}
				UQuestBuilderNode* NodeToStart = InNode ? InNode : RootNodes.IsValidIndex(0) ? RootNodes[0].Get() : nullptr;
				BeginNode(NodeToStart);
				OnQuestSetupFinished.Clear();
			}
		}
	);
	

	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(TimerHandle_CheckPrerequisites))
	{
		World->GetTimerManager().SetTimer(TimerHandle_CheckPrerequisites, this, &UQuest::CheckQuestPrerequisites, .2f, true);
	}
}


UQuestBuilderNode_Objective* UQuest::GetNavigatedObjective()
{
    for (int32 i = CurrentNodes.Num() - 1; i >= 0; --i)
    {
		UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(CurrentNodes[i]);
		if (ObjectiveNode && ObjectiveNode->IsObjectiveActive())
		{
			CurrentNavigatedObjective = ObjectiveNode;
			break;
		}
    }
	
	return CurrentNavigatedObjective;
}

void UQuest::BeginNode(UQuestBuilderNode* InNode, bool bLaunchEventOnLoad)
{
	if (InNode == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginNode: Node is null for Quest ID: %s"), *ID.ToString());
		return;
	}

	if (InNode)
	{
		CurrentNodes.AddUnique(InNode);
	}
	
	
	if (UQuestBuilderNode_Root* RootNode = Cast<UQuestBuilderNode_Root>(InNode))
	{
		RootNode->Initialize();
		return;
	}
	
	//add visited node
	VisitedNodeIDs.AddUnique(InNode->ID);
	
	//InitializeNode
	InNode->Initialize(bLaunchEventOnLoad);
}

void UQuest::BeginMultipleNodes(TArray<UQuestBuilderNode*> InNodes, bool bLaunchEventOnLoad)
{
	for (auto& node : InNodes)
	{
		BeginNode(node, bLaunchEventOnLoad);
	}
}

int UQuest::GetLevelNum() const
{
	int Level = 0;
	TArray<UQuestBuilderNode*> CurrLevelNodes = RootNodes;
	TArray<UQuestBuilderNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UQuestBuilderNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

	return Level;
}

void UQuest::GetNodesByLevel(int Level, TArray<UQuestBuilderNode*>& Nodes)
{
	int CurrLEvel = 0;
	TArray<UQuestBuilderNode*> NextLevelNodes;

	Nodes = RootNodes;

	while (Nodes.Num() != 0)
	{
		if (CurrLEvel == Level)
			break;

		for (int i = 0; i < Nodes.Num(); ++i)
		{
			UQuestBuilderNode* Node = Nodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		Nodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++CurrLEvel;
	}
}

APlayerController* UQuest::GetOwningController()
{
	return OwningController;
}

APawn* UQuest::GetOwningPawn()
{
	if (QuestComponent)
	{
		return QuestComponent->GetOwningPawn();
	}
	return nullptr;
}

UQuestComponent* UQuest::GetQuestComponent()
{
	return QuestComponent;
}

UQuestBuilderGraph* UQuest::GetOwningQuestGraph() const
{
	if (!QuestGraph)
		return nullptr;

	return QuestGraph;
}

void UQuest::GetNodePrerequisites(TArray<UQuestBuilderNode*>& Nodes)
{
	if (CurrentNodes.IsEmpty())
	{
		Nodes = AllNodes;
		return;
	}
	TSet<UQuestBuilderNode*> Visited;
	TQueue<UQuestBuilderNode*> NodeQueue;

	// Enqueue all current nodes
	for (UQuestBuilderNode* Node : CurrentNodes)
	{
		if (Node && !Visited.Contains(Node))
		{
			NodeQueue.Enqueue(Node);
			Visited.Add(Node);
		}
	}

	while (!NodeQueue.IsEmpty())
	{
		UQuestBuilderNode* Node = nullptr;
		NodeQueue.Dequeue(Node);
		if (!Node)
			continue;

		Nodes.AddUnique(Node);

		for (UQuestBuilderNode* Child : Node->ChildrenNodes)
		{
			if (Child && !Visited.Contains(Child))
			{
				NodeQueue.Enqueue(Child);
				Visited.Add(Child);
			}
		}
	}
}

void UQuest::SetupPrerequisites()
{
	if (QuestState != EQuestState::E_Active) return;
	if (!PrerequisiteNodes.IsEmpty()) return;


	PrerequisiteNodes.Empty();
	TArray<UQuestBuilderNode*> NodesToProcess;
	GetNodePrerequisites(NodesToProcess);

	for (auto& NodeToSetup : NodesToProcess)
	{
		PrerequisiteNodes.AddUnique(NodeToSetup);
		if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(NodeToSetup))
		{
			ObjectiveNode->K2_BeginSetup(QuestComponent->GetOwningController(), GetOwningPawn());
		}
		for (auto& Event : NodeToSetup->Events)
		{
			if (Event)
			{
				Event->BeginSetup(QuestComponent->GetOwningController(), GetOwningPawn());
			}
		}
	}
}


void UQuest::CheckQuestPrerequisites()
{
	bool bPrerequisiteMet = true;
	bool bShouldWaitForSetup = true;



	SetupPrerequisites();

	if (PrerequisiteNodes.IsEmpty())
	{
		SetupPrerequisites();
		bPrerequisiteMet = false;
	}
	else
	{
		//if there is any prerequisite node, it will keep checking until all of them has finished setup
		for (auto& NodeToSetup : PrerequisiteNodes)
		{
			if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(NodeToSetup))
			{
				if (!ObjectiveNode->bSetupCompleted) bPrerequisiteMet = false;
			}
			for (auto& Event : NodeToSetup->Events)
			{
				if (!Event->bSetupCompleted) bPrerequisiteMet = false;
			}
		}
	}


	bSetupCompleted = bPrerequisiteMet;


	if (bSetupCompleted)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TimerHandle_CheckPrerequisites);
		}
		PrerequisiteNodes.Empty();
		OnQuestSetupFinished.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("Quest Loaded - %s"), *QuestName.ToString());
	}

}

void UQuest::ClearGraph()
{
	for (int i = 0; i < AllNodes.Num(); ++i)
	{
		UQuestBuilderNode* Node = AllNodes[i];
		if (Node)
		{
			Node->ParentNodes.Empty();
			Node->ChildrenNodes.Empty();
			Node->Edges.Empty();
		}
	}

	VisitedNodeIDs.Empty();
	AllNodes.Empty();
	RootNodes.Empty();
	NodeMap.Empty();
}

void UQuest::EvaluateUniqueID()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> QuestGraphDataArray;
	AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestGraphDataArray);

	TArray<UQuest*> Quests;
	TArray<FName> QuestIDs;

	for (const FAssetData& AssetData : QuestGraphDataArray)
	{
		UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
		if (QuestGraphAsset)
		{
			for (auto& Quest : QuestGraphAsset->QuestList)
			{
				Quests.Add(Quest);
			}
		}
	}

	for (auto& Quest : Quests)
	{
		if (Quest != this)
		{
			QuestIDs.Add(Quest->ID);
		}
	}

	int32 Suffix = 1;
	FName NewID = ID;

	if (!QuestIDs.Contains(NewID))
	{
		return;
	}

	// Check if the new ID already exists in the array
	while (QuestIDs.Contains(NewID))
	{
		// If it does, add a numeric suffix and try again
		NewID = FName(*FString::Printf(TEXT("%s%d"), *ID.ToString(), Suffix));
		Suffix++;
	}

	ID = NewID;

}


void UQuest::MakeQuestSettingShareable(FString ShareName, bool bInitialize)
{
	SharedCategoryIdx = INDEX_NONE;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> QuestgraphDataArray;
	AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestgraphDataArray);

	TArray<int32> Remap;

	if (bInitialize)
	{
		FOrionRPGModule& OrionRPGModule = FModuleManager::GetModuleChecked<FOrionRPGModule>(TEXT("OrionRPG"));
		for (auto& QuestPreset : OrionRPGModule.QuestPresets)
		{
			if (QuestPreset == this || QuestPreset->SharedCategoryIdx != INDEX_NONE)
			{
				QuestPreset->SharedCategoryIdx = Remap.AddUnique(QuestPreset->SharedCategoryIdx) + 1; // Remaps existing index to lowest index available
			}
		}
	}
	else
	{
		for (const FAssetData& AssetData : QuestgraphDataArray)
		{
			UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
			if (QuestGraphAsset)
			{
				for (auto& OtherQuest : QuestGraphAsset->QuestList)
				{
					if (OtherQuest->SharedCategoryIdx != INDEX_NONE || OtherQuest == this)
					{
						OtherQuest->SharedCategoryIdx = Remap.AddUnique(OtherQuest->SharedCategoryIdx) + 1; // Remaps existing index to lowest index available
					}
				}
			}
		}
	}




	bSharedCategory = true;
	SharedCategoryName = ShareName;
	SharedCategoryGuid = FGuid::NewGuid();
}


void UQuest::UnshareQuestSetting()
{
	bSharedCategory = false;
	SharedCategoryIdx = INDEX_NONE;
	SharedCategoryName.Empty();
	SharedCategoryGuid.Invalidate();
}

void UQuest::UseSharedQuestSetting(const UQuest* Quest)
{
	if (Quest == this || Quest == nullptr)
	{
		return;
	}


	Modify();

	bSharedCategory = Quest->bSharedCategory;
	SharedCategoryName = Quest->SharedCategoryName;
	SharedCategoryGuid = Quest->SharedCategoryGuid;
	CopyQuestSettings(Quest);
}

void UQuest::CopyQuestSettings(const UQuest* SrcQuest)
{
	QuestCategory = SrcQuest->QuestCategory;
	bCanQuestBeAborted = SrcQuest->bCanQuestBeAborted;
	bCanRetakeQuest = SrcQuest->bCanRetakeQuest;
	SharedCategoryIdx = SrcQuest->SharedCategoryIdx;
	SharedCategoryName = SrcQuest->SharedCategoryName;
	SharedCategoryGuid = SrcQuest->SharedCategoryGuid;
}

void UQuest::PropagateQuestSettings()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> QuestgraphDataArray;
	AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestgraphDataArray);

	for (const FAssetData& AssetData : QuestgraphDataArray)
	{
		UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
		if (QuestGraphAsset)
		{
			for (int32 idx = 0; idx < QuestGraphAsset->QuestList.Num(); idx++)
			{
				if (UQuest* Quest = QuestGraphAsset->QuestList[idx])
				{
					if (Quest->SharedCategoryIdx != INDEX_NONE && Quest->SharedCategoryGuid == SharedCategoryGuid)
					{
						Quest->Modify();
						Quest->CopyQuestSettings(this);
					}
				}
			}
		}
	}
	FOrionRPGModule& OrionRPGModule = FModuleManager::GetModuleChecked<FOrionRPGModule>(TEXT("OrionRPG"));
	for (auto& QuestPreset : OrionRPGModule.QuestPresets)
	{
		if (QuestPreset && QuestPreset->SharedCategoryIdx != INDEX_NONE && QuestPreset->SharedCategoryGuid == SharedCategoryGuid)
		{
			QuestPreset->CopyQuestSettings(this);
		}
	}
}


void IQuestSharedDataHelper::MakeSureGuidExists(UQuest* Quest)
{
	if (!Quest || !Quest->QuestGraph)
		return;

	UQuestBuilderGraph* CurrentGraph = Quest->QuestGraph;
	for (int32 idx = 0; idx < CurrentGraph->QuestList.Num(); idx++)
	{
		if (UQuest* OtherQuest = CurrentGraph->QuestList[idx])
		{
			if (OtherQuest != Quest &&
				CheckIfQuestShouldShareData(Quest, OtherQuest))
			{
				AccessShareDataName(Quest) = AccessShareDataName(OtherQuest);
			}
		}
	}

	if (!AccessShareDataGuid(Quest).IsValid())
	{
		AccessShareDataGuid(Quest) = FGuid::NewGuid();
	}
}

bool FQuestSharedSettingHelper::CheckIfQuestShouldShareData(const UQuest* QuestA, const UQuest* QuestB)
{
	return QuestA->bSharedCategory && QuestB->bSharedCategory && QuestA->SharedCategoryGuid == QuestB->SharedCategoryGuid;
}

bool FQuestSharedSettingHelper::CheckIfHasDataToShare(const UQuest* Quest)
{
	return Quest->SharedCategoryIdx != INDEX_NONE;
}

void FQuestSharedSettingHelper::ShareData(UQuest* QuestWhoWantsToShare, const UQuest* ShareFrom)
{
	QuestWhoWantsToShare->UseSharedQuestSetting(ShareFrom);
}

FString& FQuestSharedSettingHelper::AccessShareDataName(UQuest* Quest)
{
	return Quest->SharedCategoryName;
}

FGuid& FQuestSharedSettingHelper::AccessShareDataGuid(UQuest* Quest)
{
	return Quest->SharedCategoryGuid;
}

void UQuest::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

#if WITH_EDITOR

void UQuest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UQuest, QuestCategory) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UQuest, bCanRetakeQuest) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UQuest, bCanQuestBeAborted))
	{
		PropagateQuestSettings();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		//If we changed the ID, make sure it doesn't conflict with any other IDs in the quest
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UQuest, ID))
		{
			EvaluateUniqueID();
		}
	}
}

void UQuest::PostLoad()
{
	Super::PostLoad();

	// make sure we have guid for shared quest setting 
	if (bSharedCategory && !SharedCategoryGuid.IsValid())
	{
		FQuestSharedSettingHelper().MakeSureGuidExists(this);
	}
}



#endif

#undef LOCTEXT_NAMESPACE
