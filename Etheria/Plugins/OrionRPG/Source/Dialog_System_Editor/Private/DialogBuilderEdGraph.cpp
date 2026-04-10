// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdGraph.h"
#include "NativeGameplayTags.h"
#include "DialogData.h"
#include "DialogBuilderEdNode_Edge.h"
#include "DialogBuilderEdNode_Root.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderEdNode_PlayerChoice.h"
#include "DialogBuilderEdNode_DialogLine.h"
#include "DialogBuilderEdge.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_PlayerLine.h"
#include "DialogBuilderEditorUtils.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"


UDialogBuilderEdGraph::UDialogBuilderEdGraph()
{
	GetOutermost()->PackageMarkedDirtyEvent.AddUObject(this, &UDialogBuilderEdGraph::OnPackageMarkedDirty);
	LastRebuildTime = 0.f;
	LastSortTime = 0.f;
	bLockUpdates = false;
}

UDialogBuilderEdGraph::~UDialogBuilderEdGraph()
{
}



void UDialogBuilderEdGraph::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)  
{  
   
}


void UDialogBuilderEdGraph::UpdateAsset(bool bForce)
{
	if (bLockUpdates ||
		!OwningDialog)
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


	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	DialogGraph->ID = FDialogBuilderEditorUtils::FindUniqueDialogGraphName("DialogAsset_");
	Clear();

	UpdateAllSubnodeDependencies();


	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(Nodes[i]))
		{
			if (DialogEdNode->NodeInstance == nullptr)
				continue;

			// cache root
			if (UDialogBuilderEdNode_Root* RootNode = Cast<UDialogBuilderEdNode_Root>(Nodes[i]))
			{
				if (RootNode->NodeInstance)
				{
					OwningDialog->RootNodes.Add(Cast<UDialogBuilderNode>(RootNode->NodeInstance));
				}
			}
			if (UDialogBuilderEdNode_DialogLine* DialogLineEdNode = Cast<UDialogBuilderEdNode_DialogLine>(Nodes[i]))
			{
				GetOutermost()->PackageMarkedDirtyEvent.AddUObject(DialogLineEdNode, &UDialogBuilderEdNode_DialogLine::OnPackageMarkedDirty);
			}
			
			UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
			if (DialogNode)
			{
				//cache dialog and node dependencies
				NodeMap.Add(DialogNode, DialogEdNode);
				OwningDialog->AllNodes.Add(DialogNode);
				OwningDialog->NodeMap.Emplace(DialogNode->ID, DialogNode);

				DialogNode->DialogGraph = OwningDialog;
				DialogNode->Rename(nullptr, DialogGraph, REN_DontCreateRedirectors | REN_DoNotDirty);

				if (UDialogBuilderNode_PlayerLine* PlayerLineNode = Cast<UDialogBuilderNode_PlayerLine>(DialogNode))
				{
					PlayerLineNode->ParticipantInfo.ParticipantTag = TAG_Dialog_Participant_Player;
					PlayerLineNode->ParticipantInfo.DefaultShot = OwningDialog->DefaultPlayerShot;
					PlayerLineNode->ParticipantInfo.ParticipantImage = OwningDialog->DefaultPlayerImage;
					PlayerLineNode->ParticipantInfo.ParticipantName = OwningDialog->PlayerName;
					OwningDialog->ParticipantInfoMap.Emplace(PlayerLineNode->ParticipantInfo.ParticipantTag, PlayerLineNode->ParticipantInfo);
				}
				if(UDialogBuilderNode_DialogLine* DialogLineNode = Cast<UDialogBuilderNode_DialogLine>(DialogNode))
				{
					OwningDialog->ParticipantInfoMap.Emplace(DialogLineNode->ParticipantInfo.ParticipantTag, DialogLineNode->ParticipantInfo);
				}
				

				//link child and parent nodes
				for (int PinIdx = 0; PinIdx < DialogEdNode->Pins.Num(); ++PinIdx)
				{
					UEdGraphPin* Pin = DialogEdNode->Pins[PinIdx];

					if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
						continue;

					for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
					{
						UDialogBuilderNode* ChildNode = nullptr;
						if (UDialogBuilderEdNode* EdNode_Child = Cast<UDialogBuilderEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
						{
							ChildNode = EdNode_Child ? Cast<UDialogBuilderNode>(EdNode_Child->NodeInstance) : nullptr;
						}
						else if (UDialogBuilderEdNode_Edge* EdNode_Edge = Cast<UDialogBuilderEdNode_Edge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
						{
							UDialogBuilderEdNode* Child = EdNode_Edge->GetEndNode();
							if (Child != nullptr)
							{
								ChildNode = Child ? Cast<UDialogBuilderNode>(Child->NodeInstance) : nullptr;
							}
						}

						if (ChildNode != nullptr)
						{
							DialogNode->ChildrenNodes.Add(ChildNode);

							ChildNode->ParentNodes.Add(DialogNode);
						}
					}
				}
			}
			
		}
		else if (UDialogBuilderEdNode_Edge* EdgeNode = Cast<UDialogBuilderEdNode_Edge>(Nodes[i]))
		{
			UDialogBuilderEdNode* StartNode = EdgeNode->GetStartNode();
			UDialogBuilderEdNode* EndNode = EdgeNode->GetEndNode();
			UDialogBuilderEdge* Edge = EdgeNode->DialogSystemGraphEdge;

			if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
			{
				continue;
			}

			EdgeMap.Add(Edge, EdgeNode);

			Edge->Graph = DialogGraph;
			Edge->Rename(nullptr, DialogGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
			Edge->StartNode = StartNode ? Cast<UDialogBuilderNode>(StartNode->NodeInstance) : nullptr; 
			Edge->EndNode = EndNode ? Cast<UDialogBuilderNode>(EndNode->NodeInstance) : nullptr;
			Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
		}
	}

	SortAllChildrenNodes(true);
}

void UDialogBuilderEdGraph::UpdateAllSubnodeDependencies()
{
	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(Nodes[i]))
		{
			if (DialogEdNode->NodeInstance == nullptr)
				continue;
			UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;

			if (DialogNode)
			{
				// parent chain
				DialogEdNode->ParentNode = NULL;

				DialogNode->Events.Reset();
				for (int32 iAux = 0; iAux < DialogEdNode->Events.Num(); iAux++)
				{
					DialogEdNode->Events[iAux]->ParentNode = DialogEdNode;
					if (UOrionEvent* EventInstance = Cast<UOrionEvent>(DialogEdNode->Events[iAux]->NodeInstance))
					{
						DialogNode->Events.Add(EventInstance);
						EventInstance->Rename(nullptr, DialogGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
					}
				}


				DialogNode->Decorators.Reset();
				for (int32 iAux = 0; iAux < DialogEdNode->Decorators.Num(); iAux++)
				{
					DialogEdNode->Decorators[iAux]->ParentNode = DialogEdNode;
					if (UOrionDecorator* DecoratorInstance = Cast<UOrionDecorator>(DialogEdNode->Decorators[iAux]->NodeInstance))
					{
						DialogNode->Decorators.Add(DecoratorInstance);
						DecoratorInstance->Rename(nullptr, DialogGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
					}
				}

			}
			
		}
	}
}

bool UDialogBuilderEdGraph::IsLocked() const
{
	return bLockUpdates;
}

void UDialogBuilderEdGraph::LockUpdates()
{
	bLockUpdates = true;
}

void UDialogBuilderEdGraph::UnlockUpdates()
{
	bLockUpdates = false;
	UpdateAsset();
}



void UDialogBuilderEdGraph::OnSubNodeDropped()
{
	NotifyGraphChanged();
}

bool UDialogBuilderEdGraph::UpdateUnknownNodeClasses()
{
	bool bUpdated = false;
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UDialogBuilderEdNode* MyNode = Cast<UDialogBuilderEdNode>(Nodes[NodeIdx]);
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

void UpdateDialogGraphNodeErrorMessage(UDialogBuilderEdNode& Node)
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

void UDialogBuilderEdGraph::UpdateDeprecatedClasses()
{
	// This function sets error messages and logs errors about nodes.

	for (int32 Idx = 0, IdxNum = Nodes.Num(); Idx < IdxNum; ++Idx)
	{
		UDialogBuilderEdNode* Node = Cast<UDialogBuilderEdNode>(Nodes[Idx]);
		if (Node != nullptr)
		{
			UpdateDialogGraphNodeErrorMessage(*Node);

			for (int32 SubIdx = 0, SubIdxNum = Node->SubNodes.Num(); SubIdx < SubIdxNum; ++SubIdx)
			{
				if (Node->SubNodes[SubIdx] != nullptr)
				{
					UpdateDialogGraphNodeErrorMessage(*Node->SubNodes[SubIdx]);
				}
			}
		}
	}
}

void UDialogBuilderEdGraph::UpdateClassData()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UDialogBuilderEdNode* Node = Cast<UDialogBuilderEdNode>(Nodes[Idx]);
		if (Node)
		{
			Node->UpdateNodeClassData();

			for (int32 SubIdx = 0; SubIdx < Node->SubNodes.Num(); SubIdx++)
			{
				if (UDialogBuilderEdNode* SubNode = Node->SubNodes[SubIdx])
				{
					SubNode->UpdateNodeClassData();
				}
			}
		}
	}
}

void UDialogBuilderEdGraph::RemoveUnknownSubNodes()
{
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UDialogBuilderEdNode* Node = Cast<UDialogBuilderEdNode>(Nodes[Index]);
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

void UDialogBuilderEdGraph::SortAllChildrenNodes(bool bForce)
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
					//sort children nodes based on the PosY of DialogEdNode
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


void UDialogBuilderEdGraph::RebuildChildOrder(UEdGraphNode* ParentNode)
{
	UDialogBuilderEdNode* DialogParentEdNode = Cast<UDialogBuilderEdNode>(ParentNode);
	UDialogBuilderNode* DialogParentNode = DialogParentEdNode ? Cast<UDialogBuilderNode>(DialogParentEdNode->NodeInstance) : nullptr;
	if (DialogParentNode)
	{
		DialogParentNode->ChildrenNodes.Sort([&](const UDialogBuilderNode& B, const UDialogBuilderNode& T)
		{
			UDialogBuilderEdNode* EdNode_BNode = NodeMap.FindRef(&B);
			UDialogBuilderEdNode* EdNode_TNode = NodeMap.FindRef(&T);
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

UDialogBuilderGraph* UDialogBuilderEdGraph::GetDialogBuilderGraph() const
{
	return CastChecked<UDialogBuilderGraph>(GetOuter());
}

bool UDialogBuilderEdGraph::Modify(bool bAlwaysMarkDirty)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetDialogBuilderGraph()->Modify();

	UpdateAllSubnodeDependencies();

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}

	return Rtn;
}

#if WITH_EDITOR
void UDialogBuilderEdGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
	UpdateAsset();
}

void UDialogBuilderEdGraph::CollectAllNodeInstances(TSet<UObject*>& NodeInstances)
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UDialogBuilderEdNode* MyNode = Cast<UDialogBuilderEdNode>(Nodes[Idx]);
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
void UDialogBuilderEdGraph::UpdateVersion_UnifiedSubNodes()
{
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UDialogBuilderEdNode* MyNode = Cast<UDialogBuilderEdNode>(Nodes[NodeIdx]);
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

void UDialogBuilderEdGraph::Clear()
{
	if (OwningDialog)
	{
		OwningDialog->ClearGraph();
	}
	NodeMap.Reset();
	EdgeMap.Reset();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(Nodes[i]))
		{
			UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
			if (DialogNode)
			{
				DialogNode->ParentNodes.Reset();
				DialogNode->ChildrenNodes.Reset();
				DialogNode->Edges.Reset();
			}
		}
	}
}

void UDialogBuilderEdGraph::SortNodes(UDialogBuilderNode* RootNode)
{
	int Level = 0;
	TArray<UDialogBuilderNode*> CurrLevelNodes = { RootNode };
	TArray<UDialogBuilderNode*> NextLevelNodes;
	TSet<UDialogBuilderNode*> Visited;

	while (CurrLevelNodes.Num() != 0)
	{
		int32 LevelWidth = 0;
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UDialogBuilderNode* Node = CurrLevelNodes[i];
			Visited.Add(Node);

			auto Comp = [&](const UDialogBuilderNode& L, const UDialogBuilderNode& R)
			{
				UDialogBuilderEdNode* EdNode_LNode = NodeMap[&L];
				UDialogBuilderEdNode* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				UDialogBuilderNode* ChildNode = Node->ChildrenNodes[j];
				if (!Visited.Contains(ChildNode))
					NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}
