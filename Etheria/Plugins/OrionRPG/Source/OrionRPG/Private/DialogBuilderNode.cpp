// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderNode.h"
#include "DialogBuilderEdge.h"
#include "Event/OrionEvent.h"
#include "Decorator/OrionDecorator.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogComponent.h"

#define LOCTEXT_NAMESPACE "DialogBuilderNode"

UDialogBuilderNode::UDialogBuilderNode()
{
	bVisited = false;
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UDialogBuilderGraph::StaticClass();

	FLinearColor Default(0.05f, 0.05f, 0.05f);
	BackgroundColor = Default;
#endif
}

UDialogBuilderNode::~UDialogBuilderNode()
{
}

void UDialogBuilderNode::Initialize(bool bLaunchEventOnLoad)
{
	StartEventQueue.Empty();

	if (bLaunchEventOnLoad)
	{
		for (auto& StartEvent : Events)
		{
			if (StartEvent && StartEvent->bLaunchEventOnLoad && (StartEvent->EventLaunchType == EEventLaunchType::E_Both || StartEvent->EventLaunchType == EEventLaunchType::E_Start))
			{
				StartEventQueue.Add(StartEvent);
			}
		}
	}
	else
	{
		for (auto& StartEvent : Events)
		{
			if (StartEvent && (StartEvent->EventLaunchType == EEventLaunchType::E_Both || StartEvent->EventLaunchType == EEventLaunchType::E_Start))
			{
				StartEventQueue.Add(StartEvent);
			}
		}
	}

	if (!OnAllEndEventFinished.IsBound())
	{
		TWeakObjectPtr<UDialogBuilderNode> WeakThis = this;
		OnAllEndEventFinished.AddLambda(
			[this, WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					if (NextNode)
					{
						DialogGraph->BeginNode(NextNode);
					}
				}
			}
		);
	}

	
	if (!OnAllStartEventFinished.IsBound())
	{
		TWeakObjectPtr<UDialogBuilderNode> WeakThis = this;
		OnAllStartEventFinished.AddLambda(
			[this, WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					BeginNode();
				}
			}
		);
	}
	
	LaunchStartEventQueue();
}


void UDialogBuilderNode::Deinitialize()
{
	EndEventQueue.Empty();
	for (auto& EndEvent : Events)
	{
		if (EndEvent && (EndEvent->EventLaunchType == EEventLaunchType::E_Both || EndEvent->EventLaunchType == EEventLaunchType::E_End))
		{
			EndEventQueue.Add(EndEvent);
		}
	}
	LaunchEndEventQueue();
	OnAllStartEventFinished.RemoveAll(this);
	OnAllEndEventFinished.RemoveAll(this);
}

void UDialogBuilderNode::Reset()
{
	//empty in base 
}

void UDialogBuilderNode::BeginNode()
{
	//empty in base 
}

void UDialogBuilderNode::EvaluateNextNode()
{
	NextNode = nullptr;
	

	DetermineNextNode();


	if (NextNode)
	{
		
		Deinitialize();
	}
	else
	{
		Deinitialize();
		GetOwningDialogGraph()->EndDialog();
		return;
	}


	//Going to next node will begin when all End Events are finished
}

void UDialogBuilderNode::DetermineNextNode()
{
	//Will execute children from top to bottom, and will stop executing until first child succeeds.
	//if none of the children succeeds, stop evaluate until the next dialog update
	for (auto& ChildNode : ChildrenNodes)
	{
		if (ChildNode->DecoratorConditionMet())
		{
			NextNode = ChildNode;
			break;
		}
	}
}

bool UDialogBuilderNode::DecoratorConditionMet()
{
	int SumConditionMet = 0;
	//Evaluate Decorator
	for (auto& Condition : Decorators)
	{
		bool bConditionMet = Condition && Condition->InvertCondition ? !Condition->PerformConditionCheck(OwningController, GetOwningPawn()) : Condition->PerformConditionCheck(OwningController, GetOwningPawn());
		if (bConditionMet)
		{
			SumConditionMet++;
		}
	}
	return SumConditionMet >= Decorators.Num();
}


void UDialogBuilderNode::InitializeFromAsset(UDialogBuilderGraph& OwningDialogGraph)
{
	DialogGraph = &OwningDialogGraph;
}


UDialogBuilderGraph* UDialogBuilderNode::GetOwningDialogGraph() const
{
	return DialogGraph;
}

APlayerController* UDialogBuilderNode::GetOwningController() const
{
	return OwningController;
}

APawn* UDialogBuilderNode::GetOwningPawn() const
{
	if (DialogComponent)
		return DialogComponent->GetOwningPawn();
	return nullptr;
}

UDialogComponent* UDialogBuilderNode::GetDialogComponent() const
{
	return DialogComponent;
}

UDialogBuilderEdge* UDialogBuilderNode::GetEdge(UDialogBuilderNode* ChildNode)
{
	return Edges.Contains(ChildNode) ? Edges.FindChecked(ChildNode) : nullptr;
}



FText UDialogBuilderNode::GetDescription_Implementation() const
{
	return FText();
}

void UDialogBuilderNode::BeginEvents(EEventLaunchType EventLaunchType)
{
	//End of a Node, Launch Event with the EventLaunchType param and Both Type
	for (auto& Event : Events)
	{
		if (Event)
		{
			if (Event->EventLaunchType == EEventLaunchType::E_Both || Event->EventLaunchType == EventLaunchType)
			{
				Event->BeginEvent(GetOwningController(), GetOwningPawn());
			}
		}
	}
}


FString UDialogBuilderNode::GetShortTypeName(const UObject* Ob)
{
	if ((Ob == nullptr) || (Ob->GetClass() == nullptr))
	{
		return TEXT("None");
	}

	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return Ob->GetClass()->GetName().LeftChop(2);
	}

	FString TypeDesc = Ob->GetClass()->GetName();
	const int32 ShortNameIdx = TypeDesc.Find(TEXT("_"), ESearchCase::CaseSensitive);
	if (ShortNameIdx != INDEX_NONE)
	{
		TypeDesc.MidInline(ShortNameIdx + 1, MAX_int32);
	}

	return TypeDesc;
}

void UDialogBuilderNode::LaunchStartEventQueue()
{
	if (StartEventQueue.IsEmpty())
	{
		OnAllStartEventFinished.Broadcast();
		return;
	}
	if (StartEventQueue.IsValidIndex(0))
	{
		TWeakObjectPtr<UDialogBuilderNode> WeakThis = this;
		StartEventQueue[0]->OnEventFinished.AddLambda(
			[this, WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					if (StartEventQueue.IsValidIndex(0))
					{
						StartEventQueue.RemoveAt(0);
						LaunchStartEventQueue();
					}
				}
			}
		);
		StartEventQueue[0]->BeginEvent(GetOwningController(), GetOwningPawn());
	}
	else
	{
		StartEventQueue.Empty();
		LaunchStartEventQueue();
	}
}

void UDialogBuilderNode::LaunchEndEventQueue()
{
	if (EndEventQueue.IsEmpty())
	{
		OnAllEndEventFinished.Broadcast();
		return;
	}
	if (EndEventQueue.IsValidIndex(0))
	{
		TWeakObjectPtr<UDialogBuilderNode> WeakThis = this;
		EndEventQueue[0]->OnEventFinished.AddLambda(
			[this, WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					if (EndEventQueue.IsValidIndex(0))
					{
						EndEventQueue.RemoveAt(0);
						LaunchEndEventQueue();
					}
				}
			}
		);
		EndEventQueue[0]->BeginEvent(GetOwningController(), GetOwningPawn());
	}
	else
	{
		EndEventQueue.Empty();
		LaunchEndEventQueue();
	}
}

void UDialogBuilderNode::EvaluateUniqueID()
{
	if (UDialogBuilderGraph* OwningDialogGraph = GetOwningDialogGraph())
	{
		TArray<UDialogBuilderNode*> DialogNodes = OwningDialogGraph->AllNodes;
		TArray<FName> NodeIDs;

		for (auto& Node : DialogNodes)
		{
			if (Node != this)
			{
				NodeIDs.Add(Node->ID);
			}
		}

		int32 FinalSuffix = 1;
		FName NewID = ID;
		FString BaseName = NewID.ToString();
		int32 UnderscoreIndex;

		if (BaseName.FindLastChar(TEXT('_'), UnderscoreIndex))
		{
			FString Suffix = BaseName.Mid(UnderscoreIndex + 1);
			if (Suffix.IsNumeric())
			{
				BaseName = BaseName.Left(UnderscoreIndex);
			}
		}

		if (!NodeIDs.Contains(NewID))
		{
			return;
		}

		// Check if the new ID already exists in the array
		while (NodeIDs.Contains(NewID))
		{
			// If it does, add a numeric suffix and try again
			NewID = FName(*FString::Printf(TEXT("%s_%d"), *BaseName, FinalSuffix));
			FinalSuffix++;
		}

		ID = NewID;
	}
}

TArray<FName> UDialogBuilderNode::GetNodeIDOptions()
{
	TArray<FName> NodeIDOptions;
	for (auto& Node : DialogGraph->AllNodes)
	{
		if (UDialogBuilderNode_Root* RootNode = Cast<UDialogBuilderNode_Root>(Node))
			continue;
		NodeIDOptions.Add(Node->ID);
	}
	return NodeIDOptions;
}


#if WITH_EDITOR
bool UDialogBuilderNode::IsNameEditable() const
{
	return true;
}

FLinearColor UDialogBuilderNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

FText UDialogBuilderNode::GetNodeTitle() const
{
	return FText::FromString(UDialogBuilderNode::GetShortTypeName(this));
}

FText UDialogBuilderNode::GetNodeDescription() const
{
	return FText::FromString(TEXT("This is NodeBase"));
}

FText UDialogBuilderNode::GetEventsText() const
{
	FString EventListText;

	for (const UOrionEvent* Event : Events)
	{
		if (Event)
		{
			if(!EventListText.IsEmpty()) EventListText += TEXT("\n");
			EventListText += Event->GetNodeDisplayText();
		}

	}

	return FText::FromString(EventListText);
}

FText UDialogBuilderNode::GetConditionsText() const
{
	FString ConditionListText;

	for (const UOrionDecorator* Condition : Decorators)
	{
		if (Condition)
		{
			if(!ConditionListText.IsEmpty()) ConditionListText += TEXT("\n");

			ConditionListText += Condition->GetNodeDisplayText();
		}

	}

	return FText::FromString(ConditionListText);
}

void UDialogBuilderNode::SetNodeTitle(const FText& NewTitle)
{
}

bool UDialogBuilderNode::CanCreateConnection(UDialogBuilderNode* Other, FText& ErrorMessage)
{
	return true;
}

bool UDialogBuilderNode::CanCreateConnectionTo(UDialogBuilderNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
	if (ChildrenLimitType == EDialogNodeLimits::Limited && NumberOfChildrenNodes >= ChildrenLimit)
	{
		ErrorMessage = FText::FromString("Children limit exceeded");
		return false;
	}

	return CanCreateConnection(Other, ErrorMessage);
	
}

bool UDialogBuilderNode::CanCreateConnectionFrom(UDialogBuilderNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage)
{
	if (ParentLimitType == EDialogNodeLimits::Limited && NumberOfParentNodes >= ParentLimit)
	{
		ErrorMessage = FText::FromString("Parent limit exceeded");
		return false;
	}

	return true;
}

void UDialogBuilderNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		//If we changed the ID, make sure it doesn't conflict with any other IDs in the dialog
		if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UDialogBuilderNode, ID))
		{
			EvaluateUniqueID();
		}
	}
}

void UDialogBuilderNode::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}


#endif

bool UDialogBuilderNode::IsLeafNode() const
{
	return ChildrenNodes.Num() == 0;
}


#undef LOCTEXT_NAMESPACE


