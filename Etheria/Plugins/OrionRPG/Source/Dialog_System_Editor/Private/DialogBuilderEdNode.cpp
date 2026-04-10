// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode.h"
#include "DialogBuilderEdSubNode_Decorator.h"
#include "DialogBuilderEdSubNode_Event.h"
#include "DialogBuilderSetting.h"
#include "EdGraphSchema_DialogBuilder.h"
#include "SGraphEditorActionMenuDialog.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderNode_PlayerLine.h"
#include "DialogBuilderEditorUtils.h"
#include "GraphDiffControl.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DialogBuilderGraph.h"

#define LOCTEXT_NAMESPACE "DialogBuilderEditor"

UDialogBuilderEdNode::UDialogBuilderEdNode()
{
	bIsReadOnly = false;
	bCanRenameNode = false;
	CopySubNodeIndex = 0;
}

UDialogBuilderEdNode::~UDialogBuilderEdNode()
{
}


UDialogBuilderEdGraph* UDialogBuilderEdNode::GetDialogBuilderEdGraph()
{
	return Cast<UDialogBuilderEdGraph>(GetGraph());
}



void UDialogBuilderEdNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

FText UDialogBuilderEdNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeInstance == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}
	else
	{
		const UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);
		return DialogNode->GetNodeTitle();
	}
}

FText UDialogBuilderEdNode::GetTooltipText() const
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

void UDialogBuilderEdNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	if (UDialogBuilderEdNode* OtherGraphNode = Cast<UDialogBuilderEdNode>(OtherNode))
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
	UDialogBuilderEdNode* OtherDialogEdNode = Cast<UDialogBuilderEdNode>(OtherNode);
	if (OtherDialogEdNode)
	{
		auto DiffSubNodes = [&Results](const FText& NodeTypeDisplayName, const TArray<UDialogBuilderEdNode*>& LhsSubNodes, const TArray<UDialogBuilderEdNode*>& RhsSubNodes)
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
		DiffSubNodes(LOCTEXT("DecoratorDiffDisplayName", "Decorator"), Decorators, OtherDialogEdNode->Decorators);
		DiffSubNodes(LOCTEXT("EventDiffDisplayName", "Event"), Events, OtherDialogEdNode->Events);
	}
}

bool UDialogBuilderEdNode::CanDuplicateNode() const
{
	return bIsReadOnly ? false : Super::CanDuplicateNode();
}

void UDialogBuilderEdNode::DestroyNode()
{
	if (ParentNode)
	{
		ParentNode->RemoveSubNode(this);
	}

	UEdGraphNode::DestroyNode();
}

void UDialogBuilderEdNode::PostPlacedNewNode()
{
	// NodeInstance can be already spawned by paste operation, don't override it

	UClass* NodeClass = ClassData.GetClass(true);
	if (NodeClass)
	{
		UEdGraph* MyGraph = GetGraph();
		UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(GetGraph());
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
				if (UDialogBuilderNode_PlayerLine* PlayerLineNode = Cast<UDialogBuilderNode_PlayerLine>(NodeInstance))
				{
					FindUniqueNodeName("PlayerLine");
				}
				else if (UDialogBuilderNode_DialogLine* DialogLineNode = Cast<UDialogBuilderNode_DialogLine>(NodeInstance))
				{
					FindUniqueNodeName("DialogLine");
					// Randomize color
					float R = FMath::RandRange(0.0f, 0.2f);
					float G = FMath::RandRange(0.0f, 0.2f);
					float B = FMath::RandRange(0.0f, 0.2f);
					float A = 1.0f;
					DialogLineNode->ParticipantInfo.NodeColor = FLinearColor(R, G, B, A);
				}
				if (UDialogBuilderNode_RerouteNode* RerouteNode = Cast<UDialogBuilderNode_RerouteNode>(NodeInstance))
				{
					FindUniqueNodeName("Reroute");
				}
				if (UDialogBuilderNode_PlayerChoice* PlayerOptionNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance))
				{
					FindUniqueNodeName("PlayerChoice");
					PlayerOptionNode->ChoiceText = FText::FromString("Choice");

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

			InitializeInstance();
		}
	}
	if (GetDialogBuilderEdGraph())
	{
		GetDialogBuilderEdGraph()->UpdateAsset();
	}
}

void UDialogBuilderEdNode::PostPasteNode()
{
	Super::PostPasteNode();
	
}

void UDialogBuilderEdNode::InitializeInstance()
{
	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);
	UDialogBuilderGraph* DialogAsset = DialogNode ? Cast<UDialogBuilderGraph>(DialogNode->GetOuter()) : nullptr;
	if (DialogNode && DialogAsset)
	{
		DialogNode->InitializeFromAsset(*DialogAsset);
	}
}

bool UDialogBuilderEdNode::CanUserDeleteNode() const
{
	return bIsReadOnly ? false : Super::CanUserDeleteNode();
}

void UDialogBuilderEdNode::PrepareForCopying()
{
	if (NodeInstance)
	{
		// Temporarily take ownership of the node instance, so that it is not deleted when cutting
		NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}

void UDialogBuilderEdNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr) {
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}


void UDialogBuilderEdNode::UpdateSubnodeDependencies()
{
	if (NodeInstance)
	{
		UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);

		DialogNode->Events.Reset();
		for (int32 iAux = 0; iAux < Events.Num(); iAux++)
		{
			if (UOrionEvent* EventInstance = Cast<UOrionEvent>(Events[iAux]->NodeInstance))
			{
				DialogNode->Events.Add(EventInstance);
			}
		}


		DialogNode->Decorators.Reset();
		for (int32 iAux = 0; iAux < Decorators.Num(); iAux++)
		{
			if (UOrionDecorator* DecoratorInstance = Cast<UOrionDecorator>(Decorators[iAux]->NodeInstance))
			{
				DialogNode->Decorators.Add(DecoratorInstance);
			}
		}
	}
}


void UDialogBuilderEdNode::AddSubNode(UDialogBuilderEdNode* SubNode, UEdGraph* ParentGraph)
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
	
}

void UDialogBuilderEdNode::RemoveSubNode(UDialogBuilderEdNode* SubNode)
{
	Modify();
	SubNodes.RemoveSingle(SubNode);

	OnSubNodeRemoved(SubNode);
	GetGraph()->Modify();
}

void UDialogBuilderEdNode::RemoveAllSubNodes()
{
	SubNodes.Reset();
	GetGraph()->Modify();
}

void UDialogBuilderEdNode::OnSubNodeRemoved(UDialogBuilderEdNode* SubNode)
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

void UDialogBuilderEdNode::OnSubNodeAdded(UDialogBuilderEdNode* NodeTemplate)
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(NodeTemplate);

	if (Cast<UDialogBuilderEdSubNode_Decorator>(NodeTemplate))
	{
		Decorators.Add(NodeTemplate);
	}
	else if(Cast<UDialogBuilderEdSubNode_Event>(NodeTemplate))
	{
		Events.Add(NodeTemplate);
	}
}

int32 UDialogBuilderEdNode::FindSubNodeDropIndex(UDialogBuilderEdNode* SubNode) const
{
	const int32 SubIdx = SubNodes.IndexOfByKey(SubNode) + 1;
	const int32 DecoratorIdx = Decorators.IndexOfByKey(SubNode) + 1;
	const int32 ServiceIdx = Events.IndexOfByKey(SubNode) + 1;

	const int32 CombinedIdx = (SubIdx & 0xff) | ((DecoratorIdx & 0xff) << 8) | ((ServiceIdx & 0xff) << 16);
	return CombinedIdx;
}

void UDialogBuilderEdNode::InsertSubNodeAt(UDialogBuilderEdNode* SubNode, int32 DropIndex)
{
	const int32 SubIdx = (DropIndex & 0xff) - 1;
	const int32 DecoratorIdx = ((DropIndex >> 8) & 0xff) - 1;
	const int32 EventIdx = ((DropIndex >> 16) & 0xff) - 1;

	if (SubIdx >= 0)
	{
		SubNodes.Insert(SubNode, SubIdx);
	}
	else
	{
		SubNodes.Add(SubNode);
	}

	UDialogBuilderEdNode* TypedNode = Cast<UDialogBuilderEdNode>(SubNode);
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

bool UDialogBuilderEdNode::IsSubNode() const
{
	return bIsSubNode || (ParentNode != nullptr);
}

bool UDialogBuilderEdNode::RefreshNodeClass()
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

void UDialogBuilderEdNode::UpdateNodeClassData()
{
	if (NodeInstance)
	{
		UpdateNodeClassDataFrom(NodeInstance->GetClass(), ClassData);
		UpdateErrorMessage();
	}
}

void UDialogBuilderEdNode::UpdateErrorMessage()
{
	ErrorMessage = ClassData.GetDeprecatedMessage();
}

bool UDialogBuilderEdNode::HasErrors() const
{
	return ErrorMessage.Len() > 0 || NodeInstance == nullptr;
}

void UDialogBuilderEdNode::UpdateNodeClassDataFrom(UClass* InstanceClass, FGraphNodeClassData& UpdatedData)
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

void UDialogBuilderEdNode::PostCopyNode()
{
	ResetNodeOwner();
}

void UDialogBuilderEdNode::ResetNodeOwner()
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

FName UDialogBuilderEdNode::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Icon");
}

void UDialogBuilderEdNode::FindUniqueNodeName(const FString& InBaseName)
{
	// Start with the base name
	FString BaseName = InBaseName;
	FString UniqueName = InBaseName;// Iterate through the dialog graph pages until we find a unique name

	int32 UnderscoreIndex;
	if (BaseName.FindLastChar(TEXT('_'), UnderscoreIndex))
	{
		FString Suffix = BaseName.Mid(UnderscoreIndex + 1);
		if (Suffix.IsNumeric())
		{
			BaseName = BaseName.Left(UnderscoreIndex);
		}
	}

	UDialogBuilderNode* DialogNode = NodeInstance ? Cast<UDialogBuilderNode>(NodeInstance) : nullptr;
	UDialogBuilderEdGraph* DialogEdGraph = GetDialogBuilderEdGraph();

	if (DialogNode && DialogEdGraph)
	{
		for (int i = 0;; i++)
		{
			bool bIsUnique = true;


			for (auto& Node : DialogEdGraph->Nodes)
			{
				UDialogBuilderEdNode* NodeEdNode = Cast<UDialogBuilderEdNode>(Node);
				if (NodeEdNode && NodeEdNode->NodeInstance)
				{
					UDialogBuilderNode* OtherDialogNode = Cast<UDialogBuilderNode>(NodeEdNode->NodeInstance);
					if (OtherDialogNode && OtherDialogNode != DialogNode)
					{
						if (FName(UniqueName) == OtherDialogNode->ID)
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
				DialogNode->ID = FName(UniqueName);
				return;
			}

			// If the current name is not unique, append a number to it and try again
			UniqueName = FString::Printf(TEXT("%s_%d"), *BaseName, i + 1);
		}
	}
}

FText UDialogBuilderEdNode::GetDescription() const
{
	
	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (DialogNode)
	{
		return DialogNode->GetNodeDescription();
	}
	
	

	FString StoredClassName = ClassData.GetClassName();
	StoredClassName.RemoveFromEnd(TEXT("_C"));

	return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));

}

FText UDialogBuilderEdNode::GetEventsText() const
{
	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (DialogNode)
	{
		return DialogNode->GetEventsText();
	}
	return FText::GetEmpty();
}

FText UDialogBuilderEdNode::GetConditionsText() const
{
	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (DialogNode)
	{
		return DialogNode->GetConditionsText();
	}
	return FText::GetEmpty();
}

FLinearColor UDialogBuilderEdNode::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->RootNodeColor;
}

UEdGraphPin* UDialogBuilderEdNode::GetInputPin() const
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

UEdGraphPin* UDialogBuilderEdNode::GetOutputPin() const
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


bool UDialogBuilderEdNode::UsesBlueprint() const
{
	if (NodeInstance)
	{
		return NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
	}
	return false;
}

void UDialogBuilderEdNode::CreateAddDecoratorSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	TSharedRef<SGraphEditorActionMenuDialog> Widget =
		SNew(SGraphEditorActionMenuDialog)
		.GraphObj(Graph)
		.GraphNode((UDialogBuilderEdNode*)this)
		.SubNodeFlags(EDialogSubNode::Decorator)
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection("Section");
	Section.AddEntry(FToolMenuEntry::InitWidget("DecoratorWidget", Widget, FText(), true));
}

void UDialogBuilderEdNode::CreateAddEventSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	TSharedRef<SGraphEditorActionMenuDialog> Widget =
		SNew(SGraphEditorActionMenuDialog)
		.GraphObj(Graph)
		.GraphNode((UDialogBuilderEdNode*)this)
		.SubNodeFlags(EDialogSubNode::Event)
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection("Section");
	Section.AddEntry(FToolMenuEntry::InitWidget("EventWidget", Widget, FText(), true));
}


void UDialogBuilderEdNode::AddContextMenuActionsDecorators(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		"AddDecorator",
		LOCTEXT("AddDecorator", "Add Decorator..."),
		LOCTEXT("AddDecoratorTooltip", "Adds new decorator as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UDialogBuilderEdNode::CreateAddDecoratorSubMenu, (UEdGraph*)Context->Graph));

}

void UDialogBuilderEdNode::AddContextMenuActionsEvents(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSubMenu(
		"AddEvent",
		LOCTEXT("AddEvent", "Add Event..."),
		LOCTEXT("AddEventTooltip", "Adds new event as a subnode"),
		FNewToolMenuDelegate::CreateUObject(this, &UDialogBuilderEdNode::CreateAddEventSubMenu, (UEdGraph*)Context->Graph));

}


#if WITH_EDITOR
void UDialogBuilderEdNode::PostEditImport()
{
	ResetNodeOwner();

	if (NodeInstance)
	{
		InitializeInstance();
	}
}
void UDialogBuilderEdNode::PostEditUndo()
{
	Super::PostEditUndo();

	ResetNodeOwner();

	if (ParentNode)
	{
		ParentNode->SubNodes.AddUnique(this);
	}

	UDialogBuilderEdNode* MyParentNode = Cast<UDialogBuilderEdNode>(ParentNode);
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
void UDialogBuilderEdNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	
}
#endif

#undef LOCTEXT_NAMESPACE