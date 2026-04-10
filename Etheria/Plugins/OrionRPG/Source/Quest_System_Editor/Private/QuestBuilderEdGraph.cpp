// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdGraph.h"
#include "QuestBuilderEdNode_Edge.h"
#include "QuestBuilderEdNode_Root.h"
#include "QuestBuilderEdNode.h"
#include "QuestBuilderEditorUtils.h"
#include "QuestBuilderEdNode.h"
#include "QuestBuilderEdge.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"


UQuestBuilderEdGraph::UQuestBuilderEdGraph()
{
	GetOutermost()->PackageMarkedDirtyEvent.AddUObject(this, &UQuestBuilderEdGraph::OnPackageMarkedDirty);
	LastRebuildTime = 0.f;
	LastSortTime = 0.f;
	bLockUpdates = false;
}

UQuestBuilderEdGraph::~UQuestBuilderEdGraph()
{
}



void UQuestBuilderEdGraph::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)  
{  
   
}


void UQuestBuilderEdGraph::UpdateAsset(bool bForce)
{
	if (bLockUpdates)
	{
		return;
	}

	const double CurrentTime = FPlatformTime::Seconds();

	if (!bForce)
	{
		// Prevent the function from being called twice in the same timeframe (e.g., 1 second)
		if (CurrentTime - LastRebuildTime < .5f)
		{
			return;
		}
	}

	LastRebuildTime = CurrentTime;

	UE_LOG(LogTemp, Log, TEXT("UpdateAsset started."));


	UQuestBuilderGraph* QuestGraph = GetQuestBuilderGraph();

	Clear();
	Quest->ID = FQuestBuilderEditorUtils::FindUniqueQuestName("QUEST_");
	
	UpdateAllSubnodeDependencies();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(Nodes[i]))
		{
			if (QuestEdNode->NodeInstance == nullptr)
				continue;

			// cache root
			if (UQuestBuilderEdNode_Root* RootNode = Cast<UQuestBuilderEdNode_Root>(Nodes[i]))
			{
				if (RootNode->NodeInstance)
				{
					Quest->RootNodes.Add(Cast<UQuestBuilderNode>(RootNode->NodeInstance));
				}
			}
			UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
			if (QuestNode)
			{
				//cache quest and node dependencies
				NodeMap.Add(QuestNode, QuestEdNode);
				Quest->AllNodes.Add(QuestNode);

				QuestNode->QuestGraph = QuestGraph;
				QuestNode->Quest = Quest;
				QuestNode->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);

				//link child and parent nodes
				for (int PinIdx = 0; PinIdx < QuestEdNode->Pins.Num(); ++PinIdx)
				{
					UEdGraphPin* Pin = QuestEdNode->Pins[PinIdx];

					if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
						continue;

					for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
					{
						UQuestBuilderNode* ChildNode = nullptr;
						if (UQuestBuilderEdNode* EdNode_Child = Cast<UQuestBuilderEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
						{
							ChildNode = EdNode_Child ? Cast<UQuestBuilderNode>(EdNode_Child->NodeInstance) : nullptr;
						}
						else if (UQuestBuilderEdNode_Edge* EdNode_Edge = Cast<UQuestBuilderEdNode_Edge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
						{
							UQuestBuilderEdNode* Child = EdNode_Edge->GetEndNode();
							if (Child != nullptr)
							{
								ChildNode = Child ? Cast<UQuestBuilderNode>(Child->NodeInstance) : nullptr;
							}
						}

						if (ChildNode != nullptr)
						{
							QuestNode->ChildrenNodes.Add(ChildNode);

							ChildNode->ParentNodes.Add(QuestNode);
						}
					}
				}
			}
			
		}
		else if (UQuestBuilderEdNode_Edge* EdgeNode = Cast<UQuestBuilderEdNode_Edge>(Nodes[i]))
		{
			UQuestBuilderEdNode* StartNode = EdgeNode->GetStartNode();
			UQuestBuilderEdNode* EndNode = EdgeNode->GetEndNode();
			UQuestBuilderEdge* Edge = EdgeNode->QuestSystemGraphEdge;

			if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
			{
				continue;
			}

			EdgeMap.Add(Edge, EdgeNode);

			Edge->Graph = QuestGraph;
			Edge->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);
			Edge->StartNode = StartNode ? Cast<UQuestBuilderNode>(StartNode->NodeInstance) : nullptr; 
			Edge->EndNode = EndNode ? Cast<UQuestBuilderNode>(EndNode->NodeInstance) : nullptr;
			Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
		}
	}

	SortAllChildrenNodes(true);
}

void UQuestBuilderEdGraph::UpdateAllSubnodeDependencies()
{
	UQuestBuilderGraph* QuestGraph = GetQuestBuilderGraph();
	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(Nodes[i]))
		{
			if (QuestEdNode->NodeInstance == nullptr)
				continue;
			UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;

			if (QuestNode)
			{
				// parent chain
				QuestEdNode->ParentNode = NULL;

				QuestNode->Events.Reset();
				for (int32 iAux = 0; iAux < QuestEdNode->Events.Num(); iAux++)
				{
					QuestEdNode->Events[iAux]->ParentNode = QuestEdNode;
					if (UOrionEvent* EventInstance = Cast<UOrionEvent>(QuestEdNode->Events[iAux]->NodeInstance))
					{
						QuestNode->Events.Add(EventInstance);
						EventInstance->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);
					}
				}


				QuestNode->Decorators.Reset();
				for (int32 iAux = 0; iAux < QuestEdNode->Decorators.Num(); iAux++)
				{
					QuestEdNode->Decorators[iAux]->ParentNode = QuestEdNode;
					if (UOrionDecorator* DecoratorInstance = Cast<UOrionDecorator>(QuestEdNode->Decorators[iAux]->NodeInstance))
					{
						QuestNode->Decorators.Add(DecoratorInstance);
						DecoratorInstance->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);
					}
				}

			}
			
		}
	}
}

bool UQuestBuilderEdGraph::IsLocked() const
{
	return bLockUpdates;
}

void UQuestBuilderEdGraph::LockUpdates()
{
	bLockUpdates = true;
}

void UQuestBuilderEdGraph::UnlockUpdates()
{
	bLockUpdates = false;
	UpdateAsset();
}



void UQuestBuilderEdGraph::OnSubNodeDropped()
{
	NotifyGraphChanged();
}

bool UQuestBuilderEdGraph::UpdateUnknownNodeClasses()
{
	bool bUpdated = false;
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UQuestBuilderEdNode* MyNode = Cast<UQuestBuilderEdNode>(Nodes[NodeIdx]);
		if (MyNode)
		{
			const bool bUpdatedNode = MyNode->RefreshNodeClass();
			bUpdated = bUpdated || bUpdatedNode;

			for (int32 SubNodeIdx = 0; SubNodeIdx < MyNode->SubNodes.Num(); SubNodeIdx++)
			{
				if (MyNode->SubNodes[SubNodeIdx])
				{
					const bool bUpdatedSubNode = MyNode->SubNodes[SubNodeIdx]->RefreshNodeClass();
					bUpdated = bUpdated || bUpdatedSubNode;
				}
			}
		}
	}

	return bUpdated;
}

void UpdateQuestGraphNodeErrorMessage(UQuestBuilderEdNode& Node)
{
	// Broke out setting error message in to own function so it can be reused when iterating nodes collection.
	if (Node.NodeInstance)
	{
		Node.ErrorMessage = FGraphNodeClassHelper::GetDeprecationMessage(Node.NodeInstance->GetClass());

		// Only check for node-specific errors if the node is not deprecated
		if (Node.ErrorMessage.IsEmpty())
		{
			Node.UpdateErrorMessage();

			// For node-specific validation we don't want to spam the log with errors
			return;
		}
	}
	else
	{
		// Null instance. Do we have any meaningful class data?
		FString StoredClassName = Node.ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		if (!StoredClassName.IsEmpty())
		{
			// There is class data here but the instance was not be created.
			static const FString IsMissingClassMessage(" class missing. Referenced by ");
			Node.ErrorMessage = StoredClassName + IsMissingClassMessage + Node.GetFullName();
		}
	}

	if (Node.HasErrors())
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Node.ErrorMessage);
	}
}

void UQuestBuilderEdGraph::UpdateDeprecatedClasses()
{
	// This function sets error messages and logs errors about nodes.

	for (int32 Idx = 0, IdxNum = Nodes.Num(); Idx < IdxNum; ++Idx)
	{
		UQuestBuilderEdNode* Node = Cast<UQuestBuilderEdNode>(Nodes[Idx]);
		if (Node != nullptr)
		{
			UpdateQuestGraphNodeErrorMessage(*Node);

			for (int32 SubIdx = 0, SubIdxNum = Node->SubNodes.Num(); SubIdx < SubIdxNum; ++SubIdx)
			{
				if (Node->SubNodes[SubIdx] != nullptr)
				{
					UpdateQuestGraphNodeErrorMessage(*Node->SubNodes[SubIdx]);
				}
			}
		}
	}
}

void UQuestBuilderEdGraph::UpdateClassData()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UQuestBuilderEdNode* Node = Cast<UQuestBuilderEdNode>(Nodes[Idx]);
		if (Node)
		{
			Node->UpdateNodeClassData();

			for (int32 SubIdx = 0; SubIdx < Node->SubNodes.Num(); SubIdx++)
			{
				if (UQuestBuilderEdNode* SubNode = Node->SubNodes[SubIdx])
				{
					SubNode->UpdateNodeClassData();
				}
			}
		}
	}
}

void UQuestBuilderEdGraph::RemoveUnknownSubNodes()
{
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UQuestBuilderEdNode* Node = Cast<UQuestBuilderEdNode>(Nodes[Index]);
		if (Node)
		{
			for (int32 SubIdx = Node->SubNodes.Num() - 1; SubIdx >= 0; SubIdx--)
			{
				const bool bIsDecorator = Node->Decorators.Contains(Node->SubNodes[SubIdx]);
				const bool bIsService = Node->Events.Contains(Node->SubNodes[SubIdx]);

				if (!bIsDecorator && !bIsService)
				{
					Node->SubNodes.RemoveAt(SubIdx);
				}
			}
		}
	}
}

void UQuestBuilderEdGraph::SortAllChildrenNodes(bool bForce)
{
	const double CurrentTime = FPlatformTime::Seconds();

	if (!bForce)
	{
		// Prevent the function from being called twice in the same timeframe (e.g., 1 second)
		if (CurrentTime - LastSortTime < .5f)
		{
			return;
		}
	}

	LastSortTime = CurrentTime;
	FTimerHandle TimerHandle;
	if (GEditor)
	{
		GEditor->GetTimerManager()->SetTimer(
			TimerHandle,
			FTimerDelegate::CreateLambda([this]()
				{
					//sort children nodes based on the PosY of QuestEdNode
					for (auto& EdNode : Nodes)
					{
						RebuildChildOrder(EdNode);

					}
				}),
			.5f, // Delay in seconds
			false // Do not loop
		);
	}
}


void UQuestBuilderEdGraph::RebuildChildOrder(UEdGraphNode* ParentNode)
{
	UQuestBuilderEdNode* QuestParentEdNode = Cast<UQuestBuilderEdNode>(ParentNode);
	UQuestBuilderNode* QuestParentNode = QuestParentEdNode ? Cast<UQuestBuilderNode>(QuestParentEdNode->NodeInstance) : nullptr;
	if (QuestParentNode)
	{
		QuestParentNode->ChildrenNodes.Sort([&](const UQuestBuilderNode& B, const UQuestBuilderNode& T)
			{
				UQuestBuilderEdNode* EdNode_BNode = NodeMap.FindRef(&B);
				UQuestBuilderEdNode* EdNode_TNode = NodeMap.FindRef(&T);
				if (EdNode_BNode && EdNode_TNode)
				{
					return EdNode_TNode->NodePosY > EdNode_BNode->NodePosY;
				}
				else
				{
					return false;
				}
			});
	}
}

UQuestBuilderGraph* UQuestBuilderEdGraph::GetQuestBuilderGraph() const
{
	return CastChecked<UQuestBuilderGraph>(GetOuter());
}

bool UQuestBuilderEdGraph::Modify(bool bAlwaysMarkDirty)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetQuestBuilderGraph()->Modify();

	UpdateAllSubnodeDependencies();

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}

	return Rtn;
}

#if WITH_EDITOR
void UQuestBuilderEdGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
	UpdateAsset();
}

void UQuestBuilderEdGraph::CollectAllNodeInstances(TSet<UObject*>& NodeInstances)
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UQuestBuilderEdNode* MyNode = Cast<UQuestBuilderEdNode>(Nodes[Idx]);
		if (MyNode)
		{
			NodeInstances.Add(MyNode->NodeInstance);

			for (int32 SubIdx = 0; SubIdx < MyNode->SubNodes.Num(); SubIdx++)
			{
				if (MyNode->SubNodes[SubIdx])
				{
					NodeInstances.Add(MyNode->SubNodes[SubIdx]->NodeInstance);
				}
			}
		}
	}
}
void UQuestBuilderEdGraph::UpdateVersion_UnifiedSubNodes()
{
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UQuestBuilderEdNode* MyNode = Cast<UQuestBuilderEdNode>(Nodes[NodeIdx]);
		if (MyNode == nullptr)
		{
			continue;
		}

		MyNode->SubNodes.Reset(MyNode->Decorators.Num() + MyNode->Events.Num());

		for (int32 SubIdx = 0; SubIdx < MyNode->Decorators.Num(); SubIdx++)
		{
			MyNode->SubNodes.Add(MyNode->Decorators[SubIdx]);
		}

		for (int32 SubIdx = 0; SubIdx < MyNode->Events.Num(); SubIdx++)
		{
			MyNode->SubNodes.Add(MyNode->Events[SubIdx]);
		}
	}
}
#endif // WITH_EDITOR

void UQuestBuilderEdGraph::Clear()
{
	Quest->ClearGraph();
	Quest->NodeMap.Reset();
	NodeMap.Reset();
	EdgeMap.Reset();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(Nodes[i]))
		{
			UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
			if (QuestNode)
			{
				QuestNode->ParentNodes.Reset();
				QuestNode->ChildrenNodes.Reset();
				QuestNode->Edges.Reset();
			}
		}
	}
}

void UQuestBuilderEdGraph::SortNodes(UQuestBuilderNode* RootNode)
{
	int Level = 0;
	TArray<UQuestBuilderNode*> CurrLevelNodes = { RootNode };
	TArray<UQuestBuilderNode*> NextLevelNodes;
	TSet<UQuestBuilderNode*> Visited;

	while (CurrLevelNodes.Num() != 0)
	{
		int32 LevelWidth = 0;
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UQuestBuilderNode* Node = CurrLevelNodes[i];
			Visited.Add(Node);

			auto Comp = [&](const UQuestBuilderNode& L, const UQuestBuilderNode& R)
			{
				UQuestBuilderEdNode* EdNode_LNode = NodeMap[&L];
				UQuestBuilderEdNode* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				UQuestBuilderNode* ChildNode = Node->ChildrenNodes[j];
				if (!Visited.Contains(ChildNode))
					NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}
