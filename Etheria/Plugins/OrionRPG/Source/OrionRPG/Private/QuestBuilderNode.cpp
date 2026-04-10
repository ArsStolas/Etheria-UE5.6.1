// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderNode.h"
#include "QuestBuilderEdge.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "Event/OrionEvent.h"
#include "Decorator/OrionDecorator.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode_Objective.h"

#define LOCTEXT_NAMESPACE "QuestBuilderNode"

UQuestBuilderNode::UQuestBuilderNode()
{
	bWaitForBranching = false;
	bInitialized = false;
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UQuestBuilderGraph::StaticClass();

	FLinearColor Default(0.05f, 0.05f, 0.05f);
	BackgroundColor = Default;
#endif
}

UQuestBuilderNode::~UQuestBuilderNode()
{
}

void UQuestBuilderNode::Initialize(bool bLaunchEventOnLoad)
{
	if (bInitialized)
		return;

	OnAllStartEventFinished.RemoveAll(this);
	OnAllEndEventFinished.RemoveAll(this);
	bInitialized = true;
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
		TWeakObjectPtr<UQuestBuilderNode> WeakThis = this;
		//when all end event is finished, begin going to next node
		OnAllEndEventFinished.AddLambda(
			[this, WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					if (!NextNodes.IsEmpty())
					{
						bool bObjectiveUpdated = false;
						for (int i = 0; i < NextNodes.Num(); i++)
						{
							if (NextNodes.IsValidIndex(i))
							{
								if (NextNodes[i]->HasBeenVisited())
									continue;
								
								if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(NextNodes[i]))
								{
									bObjectiveUpdated = true;
								}
								Quest->BeginNode(NextNodes[i]);
							}
						}

						for(int i = 0; i< Quest->CurrentNodes.Num(); i++)
						{
							if (Quest->CurrentNodes.IsValidIndex(i))
							{
								UQuestBuilderNode* CurrNode = Quest->CurrentNodes[i];
								if ((CurrNode->AllChildrenHasBeenVisited() && CurrNode->HasBeenVisited()) ||
									(CurrNode->ParentNodes.IsEmpty() && CurrNode->ChildrenNodes.IsEmpty()))
								{
									Quest->CurrentNodes.RemoveSingle(CurrNode);
									CurrNode->bWaitForBranching = false;
								}

							}
						}
						NextNodes.Empty();
						if (bObjectiveUpdated)
						{
							GetQuestComponent()->OnQuestObjectiveUpdated.Broadcast(GetOwningQuest());
						}

					}
				}
			}
		);
	}

	if (!OnAllStartEventFinished.IsBound())
	{
		TWeakObjectPtr<UQuestBuilderNode> WeakThis = this;
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

void UQuestBuilderNode::Deinitialize()
{
	bInitialized = false;
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

void UQuestBuilderNode::BeginNode()
{
	//empty in base 
}

void UQuestBuilderNode::Reset()
{
	bInitialized = false;
}

void UQuestBuilderNode::EvaluateNextNode()
{
	if (Quest->QuestState != EQuestState::E_Active)
	 return;

	
	NextNodes.Empty();
	
	//This Node will executes their children from top to bottom in sequence
	//f none of the children succeeds, this node will stop branching until next quest update.
	for (auto& ChildNode : ChildrenNodes)
	{
		//evaluate if all the parent node is objective and has completed
		bool bCanBranch = true;
		for (auto& ParentNode : ChildNode->ParentNodes)
		{
			if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(ParentNode))
			{
				if (ObjectiveNode->bOptional)
					continue;
				if(!ObjectiveNode->IsObjectiveCompleted())
					bCanBranch = false;
			}
		}
		
		if (bCanBranch && ChildNode->DecoratorConditionMet())
		{
			NextNodes.AddUnique(ChildNode);
		}
	}

	if (NextNodes.IsEmpty())
	{
		if (GetOwningQuest()->CurrentNavigatedObjective && !GetOwningQuest()->CurrentNavigatedObjective->IsObjectiveActive())
		{
			GetOwningQuest()->CurrentNavigatedObjective = nullptr;
			GetOwningQuest()->GetNavigatedObjective();
		}
		else if (GetOwningQuest()->CurrentNavigatedObjective == nullptr)
		{
			GetOwningQuest()->GetNavigatedObjective();
		}

		if (!bWaitForBranching)
		{
			bWaitForBranching = true;
			GetQuestComponent()->OnQuestObjectiveUpdated.Broadcast(GetOwningQuest());
		}

	}
	else
	{
		//shift the next navigated objective when any of the objective has completed
		for (int32 i = NextNodes.Num() - 1; i >= 0; --i)
		{
			if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(NextNodes[i]))
			{
				if (GetOwningQuest())
				{
					GetOwningQuest()->CurrentNavigatedObjective = ObjectiveNode;
					break;
				}
			}
		}
	}

	if (NextNodes.Num() > 0 && bWaitForBranching) // this node end events has been launched, and only wait for branching to next node
	{
		OnAllEndEventFinished.Broadcast();
	}
}

bool UQuestBuilderNode::HasBeenVisited()
{
	if (GetOwningQuest())
	{
		return GetOwningQuest()->VisitedNodeIDs.Contains(ID);
	}
	return false;
}

bool UQuestBuilderNode::AllChildrenHasBeenVisited()
{
	/**Check if all of this node's children has been visited*/
	bool bAllVisited = true;
	for(auto& ChildNode : ChildrenNodes)
	{
		if (ChildNode && GetOwningQuest() && !GetOwningQuest()->VisitedNodeIDs.Contains(ChildNode->ID))
		{
			bAllVisited =  false;
			break;
		}
	}
	return bAllVisited;
}

bool UQuestBuilderNode::DecoratorConditionMet()
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

UQuestBuilderEdge* UQuestBuilderNode::GetEdge(UQuestBuilderNode* ChildNode)
{
	return Edges.Contains(ChildNode) ? Edges.FindChecked(ChildNode) : nullptr;
}


FText UQuestBuilderNode::GetNodeDisplayName_Implementation() const
{
	return FText();
}


void UQuestBuilderNode::LaunchStartEventQueue()
{
	if (StartEventQueue.IsEmpty())
	{
		OnAllStartEventFinished.Broadcast();
		return;
	}
	if (StartEventQueue.IsValidIndex(0))
	{
		TWeakObjectPtr<UQuestBuilderNode> WeakThis = this;
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

void UQuestBuilderNode::LaunchEndEventQueue()
{
	if (EndEventQueue.IsEmpty())
	{
		OnAllEndEventFinished.Broadcast();
		return;
	}
	if (EndEventQueue.IsValidIndex(0))
	{
		TWeakObjectPtr<UQuestBuilderNode> WeakThis = this;
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

FString UQuestBuilderNode::GetShortTypeName(const UObject* Ob)
{
	if ((Ob == nullptr) || (Ob->GetClass() == nullptr))
	{
		return TEXT("None");
	}

	FString TypeDesc = Ob->GetClass()->GetName();

	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		TypeDesc = Ob->GetClass()->GetName().LeftChop(2);
	}

	// Insert space before each capital letter (except the first character) and remove underscores
	// Do not add space if the previous character is uppercase (abbreviation)
	FString Result;
	for (int32 i = 0; i < TypeDesc.Len(); ++i)
	{
		if (TypeDesc[i] != TEXT('_'))
		{
			TCHAR Char = TypeDesc[i];
			if (i > 0 && FChar::IsUpper(Char) && !FChar::IsWhitespace(TypeDesc[i - 1]))
			{
				// Only add space if previous character is not uppercase (not abbreviation)
				if (!FChar::IsUpper(TypeDesc[i - 1]))
				{
					Result += TEXT(" ");
				}
			}
			Result += Char;
		}
	}

	return Result;
}

UWorld* UQuestBuilderNode::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}

	UObject* Outer = GetOuter();

	while (Outer)
	{
		UWorld* World = Outer->GetWorld();
		if (World)
		{
			return World;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

FString UQuestBuilderNode::GetShortTag(FGameplayTag Tag, int32 Level) const
{
	FString TagString = Tag.ToString();
	TArray<FString> Parts;
	TagString.ParseIntoArray(Parts, TEXT("."), true);

	if (Level <= 0 || Parts.Num() == 0)
	{
		return TagString;
	}

	int32 StartIndex = FMath::Max(Parts.Num() - Level, 0);
	FString Result;
	for (int32 i = StartIndex; i < Parts.Num(); ++i)
	{
		if (!Result.IsEmpty())
		{
			Result += TEXT(".");
		}
		Result += Parts[i];
	}
	return Result;
}




#if WITH_EDITOR
bool UQuestBuilderNode::IsNameEditable() const
{
	return true;
}

FLinearColor UQuestBuilderNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

FText UQuestBuilderNode::GetNodeTitle() const
{
	return NodeName.Len() ? FText::FromString(NodeName) : FText::FromString(GetShortTypeName(this));
}

FText UQuestBuilderNode::GetNodeDescription() const
{
	return FText::FromString(TEXT("This is NodeBase"));
}

FText UQuestBuilderNode::GetEventsText() const
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

FText UQuestBuilderNode::GetConditionsText() const
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

void UQuestBuilderNode::SetNodeTitle(const FText& NewTitle)
{
}

bool UQuestBuilderNode::CanCreateConnection(UQuestBuilderNode* Other, FText& ErrorMessage)
{
	return true;
}

bool UQuestBuilderNode::CanCreateConnectionTo(UQuestBuilderNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
	if (ChildrenLimitType == EQuestNodeLimits::Limited && NumberOfChildrenNodes >= ChildrenLimit)
	{
		ErrorMessage = FText::FromString("Children limit exceeded");
		return false;
	}

	return CanCreateConnection(Other, ErrorMessage);
	
}

bool UQuestBuilderNode::CanCreateConnectionFrom(UQuestBuilderNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage)
{
	if (ParentLimitType == EQuestNodeLimits::Limited && NumberOfParentNodes >= ParentLimit)
	{
		ErrorMessage = FText::FromString("Parent limit exceeded");
		return false;
	}

	return true;
}

void UQuestBuilderNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	
}


#endif

bool UQuestBuilderNode::IsLeafNode() const
{
	return ChildrenNodes.Num() == 0;
}


void UQuestBuilderNode::InitializeFromAsset(UQuestBuilderGraph& OwningQuestGraph, UQuest& OwningQuest)
{
	QuestGraph = &OwningQuestGraph;
	Quest = &OwningQuest;
	Quest->AllNodes.Add(this);
	Quest->NodeMap.Emplace(ID, this);
}


UQuestBuilderGraph* UQuestBuilderNode::GetOwningQuestGraph() const
{
	if (!QuestGraph)
		return nullptr;

	return QuestGraph;
}

UQuest* UQuestBuilderNode::GetOwningQuest() const
{
	return Quest;
}


APlayerController* UQuestBuilderNode::GetOwningController() const
{
	return OwningController;
}

APawn* UQuestBuilderNode::GetOwningPawn() const
{
	if (QuestComponent)
		return QuestComponent->GetOwningPawn();
	return nullptr;
}

UQuestComponent* UQuestBuilderNode::GetQuestComponent() const
{
	return QuestComponent;
}

#undef LOCTEXT_NAMESPACE


