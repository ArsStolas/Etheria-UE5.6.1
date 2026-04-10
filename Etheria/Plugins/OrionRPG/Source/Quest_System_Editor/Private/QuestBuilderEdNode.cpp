// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdNode.h"
#include "QuestBuilderEdSubNode_Decorator.h"
#include "QuestBuilderEdSubNode_Event.h"
#include "QuestBuilderSetting.h"
#include "EdGraphSchema_QuestBuilder.h"
#include "SGraphEditorActionMenuQuest.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Root.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderNode_Checkpoint.h"
#include "QuestBuilderNode_State.h"
#include "QuestBuilderEditorUtils.h"
#include "GraphDiffControl.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "QuestBuilderGraph.h"
#include "UObject/TopLevelAssetPath.h"
#include "AIGraphTypes.h"
#include "Quest.h"

#define LOCTEXT_NAMESPACE "QuestBuilderEditor"

UQuestBuilderEdNode::UQuestBuilderEdNode()
{
	bIsReadOnly = false;
	bCanRenameNode = false;
	CopySubNodeIndex = 0;
}

UQuestBuilderEdNode::~UQuestBuilderEdNode()
{
}


UQuestBuilderEdGraph* UQuestBuilderEdNode::GetQuestBuilderEdGraph()
{
	return Cast<UQuestBuilderEdGraph>(GetGraph());
}



void UQuestBuilderEdNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

FText UQuestBuilderEdNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeInstance == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}
	else
	{
		const UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
		return QuestNode->GetNodeTitle();
	}
}

FText UQuestBuilderEdNode::GetTooltipText() const
{
	FText TooltipDesc;

	if (TooltipDesc.IsEmpty())
	{
		if (!NodeInstance)
		{
			FString StoredClassName = ClassData.GetClassName();
			StoredClassName.RemoveFromEnd(TEXT("_C"));

			TooltipDesc = FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
		}
		else
		{
			if (ErrorMessage.Len() > 0)
			{
				TooltipDesc = FText::FromString(ErrorMessage);
			}
			else
			{
				if (NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && !NodeInstance->GetClass()->GetPackage()->HasAnyPackageFlags(PKG_Cooked))
				{
					FAssetData AssetData(NodeInstance->GetClass()->ClassGeneratedBy);
					FString Description = AssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription));
					if (!Description.IsEmpty())
					{
						Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
						TooltipDesc = FText::FromString(MoveTemp(Description));
					}
				}
				else
				{
					TooltipDesc = NodeInstance->GetClass()->GetToolTipText();
				}
			}
		}
	}
	if (NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && !NodeInstance->GetClass()->GetPackage()->HasAnyPackageFlags(PKG_Cooked))
	{
		TooltipDesc = FText::FromString(TooltipDesc.ToString() + "\n**Double click node to open blueprint");
	}

	return TooltipDesc;
}

void UQuestBuilderEdNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	if (UQuestBuilderEdNode* OtherGraphNode = Cast<UQuestBuilderEdNode>(OtherNode))
	{
		if (NodeInstance && OtherGraphNode->NodeInstance)
		{
			FDiffSingleResult Diff;
			Diff.Diff = EDiffType::NODE_PROPERTY;
			Diff.Node1 = this;
			Diff.Node2 = OtherNode;
			Diff.ToolTip = LOCTEXT("DIF_NodeInstancePropertyToolTip", "A property of the node instance has changed");
			Diff.Category = EDiffType::MODIFICATION;

			DiffProperties(NodeInstance->GetClass(), OtherGraphNode->NodeInstance->GetClass(), NodeInstance, OtherGraphNode->NodeInstance, Results, Diff);
		}
	}
	UQuestBuilderEdNode* OtherQuestEdNode = Cast<UQuestBuilderEdNode>(OtherNode);
	if (OtherQuestEdNode)
	{
		auto DiffSubNodes = [&Results](const FText& NodeTypeDisplayName, const TArray<UQuestBuilderEdNode*>& LhsSubNodes, const TArray<UQuestBuilderEdNode*>& RhsSubNodes)
		{
			TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
			TSet<const UEdGraphNode*> MatchedRhsNodes;
			FGraphDiffControl::FNodeDiffContext AdditiveDiffContext;
			AdditiveDiffContext.NodeTypeDisplayName = NodeTypeDisplayName;
			AdditiveDiffContext.bIsRootNode = false;

			// march through the all the nodes in the rhs and look for matches 
			for (UEdGraphNode* RhsSubNode : RhsSubNodes)
			{
				FGraphDiffControl::FNodeMatch NodeMatch;
				NodeMatch.NewNode = RhsSubNode;

				// Do two passes, exact and soft
				for (UEdGraphNode* LhsSubNode : LhsSubNodes)
				{
					if (FGraphDiffControl::IsNodeMatch(LhsSubNode, RhsSubNode, true, &NodeMatches))
					{
						NodeMatch.OldNode = LhsSubNode;
						break;
					}
				}

				if (NodeMatch.NewNode == nullptr)
				{
					for (UEdGraphNode* LhsSubNode : LhsSubNodes)
					{
						if (FGraphDiffControl::IsNodeMatch(LhsSubNode, RhsSubNode, false, &NodeMatches))
						{
							NodeMatch.OldNode = LhsSubNode;
							break;
						}
					}
				}

				// if we found a corresponding node in the lhs graph, track it (so we can prevent future matches with the same nodes)
				if (NodeMatch.IsValid())
				{
					NodeMatches.Add(NodeMatch);
					MatchedRhsNodes.Add(NodeMatch.OldNode);
				}

				NodeMatch.Diff(AdditiveDiffContext, Results);
			}
			FGraphDiffControl::FNodeDiffContext SubtractiveDiffContext = AdditiveDiffContext;
			SubtractiveDiffContext.DiffMode = FGraphDiffControl::EDiffMode::Subtractive;
			SubtractiveDiffContext.DiffFlags = FGraphDiffControl::EDiffFlags::NodeExistance;

			// go through the lhs nodes to catch ones that may have been missing from the rhs graph
			for (UEdGraphNode* LhsSubNode : LhsSubNodes)
			{
				// if this node has already been matched, move on
				if (!LhsSubNode || MatchedRhsNodes.Find(LhsSubNode))
				{
					continue;
				}

				// There can't be a matching node in RhsGraph because it would have been found above
				FGraphDiffControl::FNodeMatch NodeMatch;
				NodeMatch.NewNode = LhsSubNode;

				NodeMatch.Diff(SubtractiveDiffContext, Results);
			}
		};
		DiffSubNodes(LOCTEXT("DecoratorDiffDisplayName", "Decorator"), Decorators, OtherQuestEdNode->Decorators);
		DiffSubNodes(LOCTEXT("EventDiffDisplayName", "Event"), Events, OtherQuestEdNode->Events);
	}
}

bool UQuestBuilderEdNode::CanDuplicateNode() const
{
	return bIsReadOnly ? false : Super::CanDuplicateNode();
}

void UQuestBuilderEdNode::DestroyNode()
{
	if (ParentNode)
	{
		ParentNode->RemoveSubNode(this);
	}

	UEdGraphNode::DestroyNode();
}

void UQuestBuilderEdNode::PostPlacedNewNode()
{
	// NodeInstance can be already spawned by paste operation, don't override it

	UClass* NodeClass = ClassData.GetClass(true);
	if (NodeClass)
	{
		UEdGraph* MyGraph = GetGraph();
		UQuestBuilderEdGraph* QuestEdGraph = Cast<UQuestBuilderEdGraph>(GetGraph());
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;
		if (GraphOwner)
		{
			if (NodeInstance == nullptr)
			{
				NodeInstance = NewObject<UObject>(GraphOwner, NodeClass);
				NodeInstance->SetFlags(RF_Transactional);
			}
			if (NodeInstance)
			{
				if (UQuestBuilderNode_Objective* ObjectiveQuestNode = Cast<UQuestBuilderNode_Objective>(NodeInstance))
				{
					FindUniqueNodeName("ObjectiveNode");
				}
				if (UQuestBuilderNode_State* StateQuestNode = Cast<UQuestBuilderNode_State>(NodeInstance))
				{
					FindUniqueNodeName("StateNode");
				}
				if (UQuestBuilderNode_Checkpoint* CheckpointQuestNode = Cast<UQuestBuilderNode_Checkpoint>(NodeInstance))
				{
					FindUniqueNodeName("CheckpointNode");
				}
				if (UOrionDecorator* OrionDecorator = Cast<UOrionDecorator>(NodeInstance))
				{
					OrionDecorator->bIsNode = true;
				}
				if (UOrionEvent* OrionEvent = Cast<UOrionEvent>(NodeInstance))
				{
					OrionEvent->bIsNode = true;
				}
			}
			UQuestBuilderNode* QuestNode = NodeInstance ?  Cast<UQuestBuilderNode>(NodeInstance) : nullptr;
			if (QuestNode)
			{
                QuestNode->Description = FText::FromString(TEXT("Description..."));
			}

			InitializeInstance();
		}
	}
}

void UQuestBuilderEdNode::InitializeInstance()
{
	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
	UQuestBuilderGraph* QuestAsset = QuestNode ? Cast<UQuestBuilderGraph>(QuestNode->GetOuter()) : nullptr;
	UQuest* Quest = GetQuestBuilderEdGraph() ? GetQuestBuilderEdGraph()->Quest : nullptr;
	if (Quest && QuestNode && QuestAsset)
	{
		QuestNode->InitializeFromAsset(*QuestAsset, *Quest);

	}
}

bool UQuestBuilderEdNode::CanUserDeleteNode() const
{
	return bIsReadOnly ? false : Super::CanUserDeleteNode();
}

void UQuestBuilderEdNode::PrepareForCopying()
{
	if (NodeInstance)
	{
		// Temporarily take ownership of the node instance, so that it is not deleted when cutting
		NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}

void UQuestBuilderEdNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr) {
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}


void UQuestBuilderEdNode::UpdateSubnodeDependencies()
{
	if (NodeInstance)
	{
		UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
		UQuestBuilderGraph* QuestAsset = QuestNode ? Cast<UQuestBuilderGraph>(QuestNode->GetOuter()) : nullptr;
		UQuest* Quest = GetQuestBuilderEdGraph() ? GetQuestBuilderEdGraph()->Quest : nullptr;

		QuestNode->Events.Reset();
		for (int32 iAux = 0; iAux < Events.Num(); iAux++)
		{
			if (UOrionEvent* EventInstance = Cast<UOrionEvent>(Events[iAux]->NodeInstance))
			{
				QuestNode->Events.Add(EventInstance);
				EventInstance->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}


		QuestNode->Decorators.Reset();
		for (int32 iAux = 0; iAux < Decorators.Num(); iAux++)
		{
			if (UOrionDecorator* DecoratorInstance = Cast<UOrionDecorator>(Decorators[iAux]->NodeInstance))
			{
				QuestNode->Decorators.Add(DecoratorInstance);
				DecoratorInstance->Rename(nullptr, Quest, REN_DontCreateRedirectors | REN_DoNotDirty);
			}
		}
	}
}


void UQuestBuilderEdNode::AddSubNode(UQuestBuilderEdNode* SubNode, UEdGraph* ParentGraph)
{
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	

	SubNode->SetFlags(RF_Transactional);

	// set outer to be the graph so it doesn't go away
	SubNode->Rename(nullptr, ParentGraph, REN_NonTransactional);
	SubNode->ParentNode = this;

	SubNode->CreateNewGuid();
	SubNode->PostPlacedNewNode();
	SubNode->AllocateDefaultPins();
	SubNode->AutowireNewNode(nullptr);

	SubNode->NodePosX = 0;
	SubNode->NodePosY = 0;

	SubNodes.Add(SubNode);
	OnSubNodeAdded(SubNode);

	ParentGraph->NotifyGraphChanged();
	ParentGraph->Modify();
	Modify();
	if (GetQuestBuilderEdGraph())
	{
		GetQuestBuilderEdGraph()->UpdateAllSubnodeDependencies();
	}
}

void UQuestBuilderEdNode::RemoveSubNode(UQuestBuilderEdNode* SubNode)
{
	Modify();
	SubNodes.RemoveSingle(SubNode);

	OnSubNodeRemoved(SubNode);
	GetGraph()->Modify();
}

void UQuestBuilderEdNode::RemoveAllSubNodes()
{
	SubNodes.Reset();
	GetGraph()->Modify();
}

void UQuestBuilderEdNode::OnSubNodeRemoved(UQuestBuilderEdNode* SubNode)
{
	const int32 DecoratorIdx = Decorators.IndexOfByKey(SubNode);
	const int32 EventIdx = Events.IndexOfByKey(SubNode);

	if (DecoratorIdx >= 0)
	{
		Decorators.RemoveAt(DecoratorIdx);
	}

	if (EventIdx >= 0)
	{
		Events.RemoveAt(EventIdx);
	}

}

void UQuestBuilderEdNode::OnSubNodeAdded(UQuestBuilderEdNode* NodeTemplate)
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(NodeTemplate);

	if (Cast<UQuestBuilderEdSubNode_Decorator>(NodeTemplate))
	{
		Decorators.Add(NodeTemplate);
	}
	else if(Cast<UQuestBuilderEdSubNode_Event>(NodeTemplate))
	{
		Events.Add(NodeTemplate);
	}
}

int32 UQuestBuilderEdNode::FindSubNodeDropIndex(UQuestBuilderEdNode* SubNode) const
{
	const int32 SubIdx = SubNodes.IndexOfByKey(SubNode) + 1;
	const int32 DecoratorIdx = Decorators.IndexOfByKey(SubNode) + 1;
	const int32 ServiceIdx = Events.IndexOfByKey(SubNode) + 1;

	const int32 CombinedIdx = (SubIdx & 0xff) | ((DecoratorIdx & 0xff) << 8) | ((ServiceIdx & 0xff) << 16);
	return CombinedIdx;
}

void UQuestBuilderEdNode::InsertSubNodeAt(UQuestBuilderEdNode* SubNode, int32 DropIndex)
{
	const int32 SubIdx = (DropIndex & 0xff) - 1;
	const int32 DecoratorIdx = ((DropIndex >> 8) & 0xff) - 1;
	const int32 EventIdx = ((DropIndex >> 16) & 0xff) - 1;
	const int32 TaskIdx = ((DropIndex >> 24) & 0xff) - 1;

	if (SubIdx >= 0)
	{
		SubNodes.Insert(SubNode, SubIdx);
	}
	else
	{
		SubNodes.Add(SubNode);
	}

	UQuestBuilderEdNode* TypedNode = Cast<UQuestBuilderEdNode>(SubNode);
	const bool bIsDecorator = Cast<UOrionDecorator>(SubNode->NodeInstance) != nullptr;
	const bool bIsService = Cast<UOrionEvent>(SubNode->NodeInstance) != nullptr;

	if (TypedNode)
	{
		if (bIsDecorator)
		{
			if (DecoratorIdx >= 0)
			{
				Decorators.Insert(TypedNode, DecoratorIdx);
			}
			else
			{
				Decorators.Add(TypedNode);
			}

		}

		if (bIsService)
		{
			if (EventIdx >= 0)
			{
				Events.Insert(TypedNode, EventIdx);
			}
			else
			{
				Events.Add(TypedNode);
			}
		}
	}
	GetGraph()->Modify();
}

bool UQuestBuilderEdNode::IsSubNode() const
{
	return bIsSubNode || (ParentNode != nullptr);
}

bool UQuestBuilderEdNode::RefreshNodeClass()
{
	bool bUpdated = false;
	if (NodeInstance == nullptr)
	{
		if (FGraphNodeClassHelper::IsClassKnown(ClassData))
		{
			PostPlacedNewNode();
			bUpdated = (NodeInstance != nullptr);
		}
		else
		{
			FGraphNodeClassHelper::AddUnknownClass(ClassData);
		}
	}

	return bUpdated;
}

void UQuestBuilderEdNode::UpdateNodeClassData()
{
	if (NodeInstance)
	{
		UpdateNodeClassDataFrom(NodeInstance->GetClass(), ClassData);
		UpdateErrorMessage();
	}
}

void UQuestBuilderEdNode::UpdateErrorMessage()
{
	ErrorMessage = ClassData.GetDeprecatedMessage();
}

bool UQuestBuilderEdNode::HasErrors() const
{
	return ErrorMessage.Len() > 0 || NodeInstance == nullptr;
}

void UQuestBuilderEdNode::UpdateNodeClassDataFrom(UClass* InstanceClass, FGraphNodeClassData& UpdatedData)
{
	if (InstanceClass)
	{
		if (UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy))
		{
			UpdatedData = FGraphNodeClassData(BPOwner->GetName(), BPOwner->GetOutermost()->GetName(), InstanceClass->GetName(), InstanceClass);
		}
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
		else if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(InstanceClass))
		{
			UpdatedData = FGraphNodeClassData(BPGC->GetClassPathName(), BPGC);
		}
#endif
		else
		{
			UpdatedData = FGraphNodeClassData(InstanceClass, FGraphNodeClassHelper::GetDeprecationMessage(InstanceClass));
		}
	}
}

void UQuestBuilderEdNode::PostCopyNode()
{
	ResetNodeOwner();
}

void UQuestBuilderEdNode::ResetNodeOwner()
{
	if (NodeInstance)
	{
		UEdGraph* MyGraph = GetGraph();
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;

		NodeInstance->Rename(NULL, GraphOwner, REN_DontCreateRedirectors | REN_DoNotDirty);
		NodeInstance->ClearFlags(RF_Transient);

		for (auto& SubNode : SubNodes)
		{
			SubNode->ResetNodeOwner();
		}
	}
}

FName UQuestBuilderEdNode::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Icon");
}

void UQuestBuilderEdNode::FindUniqueNodeName(const FString& InBaseName)
{
	// Start with the base name
	FString BaseName = InBaseName;
	FString UniqueName = InBaseName;// Iterate through the quest graph pages until we find a unique name

	int32 UnderscoreIndex;
	if (BaseName.FindLastChar(TEXT('_'), UnderscoreIndex))
	{
		FString Suffix = BaseName.Mid(UnderscoreIndex + 1);
		if (Suffix.IsNumeric())
		{
			BaseName = BaseName.Left(UnderscoreIndex);
		}
	}

	UQuestBuilderNode* QuestNode = NodeInstance ? Cast<UQuestBuilderNode>(NodeInstance) : nullptr;
	UQuestBuilderEdGraph* QuestEdGraph = GetQuestBuilderEdGraph();

	if (QuestNode && QuestEdGraph)
	{
		for (int i = 0;; i++)
		{
			bool bIsUnique = true;


			for (auto& Node : QuestEdGraph->Nodes)
			{
				UQuestBuilderEdNode* NodeEdNode = Cast<UQuestBuilderEdNode>(Node);
				if (NodeEdNode && NodeEdNode->NodeInstance)
				{
					UQuestBuilderNode* OtherQuestNode = Cast<UQuestBuilderNode>(NodeEdNode->NodeInstance);
					if (OtherQuestNode && OtherQuestNode != QuestNode)
					{
						if (FName(UniqueName) == OtherQuestNode->ID)
						{
							bIsUnique = false;
							break;
						}
					}
				}
			}


			// If the current name is unique, return it
			if (bIsUnique)
			{
				QuestNode->ID = FName(UniqueName);
				return;
			}

			// If the current name is not unique, append a number to it and try again
			UniqueName = FString::Printf(TEXT("%s_%d"), *BaseName, i + 1);
		}
	}
}

FText UQuestBuilderEdNode::GetDescription() const
{
	
	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
	if (QuestNode)
	{
		return QuestNode->GetNodeDescription();
	}
	
	

	FString StoredClassName = ClassData.GetClassName();
	StoredClassName.RemoveFromEnd(TEXT("_C"));

	return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));

}

FText UQuestBuilderEdNode::GetEventsText() const
{
	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
	if (QuestNode)
	{
		return QuestNode->GetEventsText();
	}
	return FText::GetEmpty();
}

FText UQuestBuilderEdNode::GetConditionsText() const
{
	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(NodeInstance);
	if (QuestNode)
	{
		return QuestNode->GetConditionsText();
	}
	return FText::GetEmpty();
}

FLinearColor UQuestBuilderEdNode::GetBackgroundColor() const
{
	return GetDefault<UQuestBuilderSetting>()->RootNodeColor;
}

UEdGraphPin* UQuestBuilderEdNode::GetInputPin() const
{
	//return Pins[0];
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Input)
		{
			return Pins[PinIndex];
		}
	}

	return nullptr;
}

UEdGraphPin* UQuestBuilderEdNode::GetOutputPin() const
{
	//return Pins[1];
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			return Pins[PinIndex];
		}
	}

	return nullptr;
}


bool UQuestBuilderEdNode::UsesBlueprint() const
{
	if (NodeInstance)
	{
		return NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
	}
	return false;
}

void UQuestBuilderEdNode::CreateAddDecoratorSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	TSharedRef<SGraphEditorActionMenuQuest> Widget =
		SNew(SGraphEditorActionMenuQuest)
		.GraphObj(Graph)
		.GraphNode((UQuestBuilderEdNode*)this)
		.SubNodeFlags(EQuestNode::Decorator)
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection("Section");
	Section.AddEntry(FToolMenuEntry::InitWidget("DecoratorWidget", Widget, FText(), true));
}

void UQuestBuilderEdNode::CreateAddEventSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	TSharedRef<SGraphEditorActionMenuQuest> Widget =
		SNew(SGraphEditorActionMenuQuest)
		.GraphObj(Graph)
		.GraphNode((UQuestBuilderEdNode*)this)
		.SubNodeFlags(EQuestNode::Event)
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection("Section");
	Section.AddEntry(FToolMenuEntry::InitWidget("EventWidget", Widget, FText(), true));
}

void UQuestBuilderEdNode::AddContextMenuActionsDecorators(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		"AddDecorator",
		LOCTEXT("AddDecorator", "Add Decorator..."),
		LOCTEXT("AddDecoratorTooltip", "Adds new decorator as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UQuestBuilderEdNode::CreateAddDecoratorSubMenu, (UEdGraph*)Context->Graph));

}

void UQuestBuilderEdNode::AddContextMenuActionsEvents(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		"AddEvent",
		LOCTEXT("AddEvent", "Add Event..."),
		LOCTEXT("AddEventTooltip", "Adds new event as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UQuestBuilderEdNode::CreateAddEventSubMenu, (UEdGraph*)Context->Graph));

}

#if WITH_EDITOR
void UQuestBuilderEdNode::PostEditImport()
{
	ResetNodeOwner();

	if (NodeInstance)
	{
		InitializeInstance();
	}
}
void UQuestBuilderEdNode::PostEditUndo()
{
	Super::PostEditUndo();

	ResetNodeOwner();

	if (ParentNode)
	{
		ParentNode->SubNodes.AddUnique(this);
	}

	UQuestBuilderEdNode* MyParentNode = Cast<UQuestBuilderEdNode>(ParentNode);
	if (MyParentNode)
	{
		const bool bIsDecorator = Cast<UOrionDecorator>(NodeInstance) != nullptr;
		const bool bIsEvent = Cast<UOrionEvent>(NodeInstance) != nullptr;

		if (bIsDecorator)
		{
			MyParentNode->Decorators.AddUnique(this);
		}
		else if (bIsEvent)
		{
			MyParentNode->Events.AddUnique(this);
		}
	}
}
void UQuestBuilderEdNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	
}
#endif

#undef LOCTEXT_NAMESPACE