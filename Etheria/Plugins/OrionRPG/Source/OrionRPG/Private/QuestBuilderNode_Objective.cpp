// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderNode_Objective.h"
#include "Quest.h"
#include "QuestBuilderSetting.h"
#include "QuestComponent.h"
#include "Engine/World.h"	
#include "TimerManager.h"


#define LOCTEXT_NAMESPACE "QuestSystemGraphNode_Objective"


UQuestBuilderNode_Objective::UQuestBuilderNode_Objective()
{
    ObjectiveState = EObjectiveState::E_Active;
	bObjectiveInitialized = false;
    bSetupCompleted = false;
    bOptional = false;
    bHidden = false;
    bIsObjectiveTickable = false;
    bUseQuestIndicator = false;
    TickInterval = 0.1f;
    RequiredAmount = 1;
}

void UQuestBuilderNode_Objective::Initialize(bool bLaunchEventOnLoad)
{
    if (IsObjectiveCompleted())
    {
        EvaluateNextNode();
    }
    else
    {
        Super::Initialize(bLaunchEventOnLoad);
    }
}

void UQuestBuilderNode_Objective::K2_BeginSetup_Implementation(APlayerController* OwnerController, APawn* ControlledPawn)
{
    FinishSetup();
}

void UQuestBuilderNode_Objective::FinishSetup()
{
    bSetupCompleted = true;
}

void UQuestBuilderNode_Objective::BeginNode()
{
    Super::BeginNode();
	BeginObjective();
}

void UQuestBuilderNode_Objective::Deinitialize()
{
	Super::Deinitialize();
}

void UQuestBuilderNode_Objective::Reset()
{
    Super::Reset();
    ObjectiveState = EObjectiveState::E_Active;
    CurrentProgress = 0;
    bObjectiveInitialized = false;
	DeactivateTick();
}

bool UQuestBuilderNode_Objective::IsObjectiveCompleted()
{
	return ObjectiveState == EObjectiveState::E_Completed;
}

bool UQuestBuilderNode_Objective::IsObjectiveFailed()
{
    return ObjectiveState == EObjectiveState::E_Failed;
}

bool UQuestBuilderNode_Objective::IsObjectiveActive()
{
    return ObjectiveState == EObjectiveState::E_Active;
}

void UQuestBuilderNode_Objective::TickObjective_Implementation()
{
}

void UQuestBuilderNode_Objective::AddToProgress(int AmountToAdd)
{
    if (!bInitialized) return;
    if (bUseAmount)
    {
        CurrentProgress = FMath::Clamp(CurrentProgress + AmountToAdd, 0, RequiredAmount);
        if (CurrentProgress >= RequiredAmount)
        {
            CompleteObjective();
        }
        else
        {
            GetQuestComponent()->OnQuestObjectiveUpdated.Broadcast(GetOwningQuest());
        }
    }
    else
    {
        CompleteObjective();
    }
    
    
}

void UQuestBuilderNode_Objective::BeginObjective()
{
    if (!bInitialized) return;

    if (ObjectiveState == EObjectiveState::E_Completed || ObjectiveState == EObjectiveState::E_Failed)
        return;
    if(bObjectiveInitialized)
		return;

	bObjectiveInitialized = true;
    ObjectiveState = EObjectiveState::E_Active;
    K2_BeginObjective(GetOwningController(), GetOwningPawn());


    GetQuestComponent()->OnQuestObjectiveBegin.Broadcast(this);
    if (GetQuestComponent() && bIsObjectiveTickable)
    {
        if (TickInterval > 0.f)
        {
            UWorld* World = GetWorld();
            if (World)
            {
                World->GetTimerManager().SetTimer(TimerHandle_TickObjective, this, &UQuestBuilderNode_Objective::TickObjective, TickInterval, true);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BeginTask: Failed to get UWorld instance."));
            }
        }

        // Fire the first tick off after BeginTask since begin task will usually init things that TickTask may need  
        TickObjective();
    }
}

void UQuestBuilderNode_Objective::CompleteObjective()
{
    if (!bInitialized) return;

    if (Quest && Quest->QuestState != EQuestState::E_Active)
        return;
    if (ObjectiveState != EObjectiveState::E_Active)
        return;

    ObjectiveState = EObjectiveState::E_Completed;
    K2_ObjectiveCompleted(GetOwningController(), GetOwningPawn());
    GetQuestComponent()->OnQuestUpdated.Broadcast();


    Deinitialize();
    DeactivateTick();
}

void UQuestBuilderNode_Objective::FailObjective()
{
    if (!bInitialized) return;

    if (Quest->QuestState != EQuestState::E_Active)
        return;
    if (ObjectiveState != EObjectiveState::E_Active)
        return;

    ObjectiveState = EObjectiveState::E_Failed;
    K2_ObjectiveFailed(GetOwningController(), GetOwningPawn());
    GetQuestComponent()->OnQuestUpdated.Broadcast();

    DeactivateTick();
}

void UQuestBuilderNode_Objective::DeactivateTick()
{
    //clear tick timer handle
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimerHandle_TickObjective);
    }
}


void UQuestBuilderNode_Objective::EvaluateNextNode()
{
    if (IsObjectiveCompleted())
    {
        Super::EvaluateNextNode();
    }
}

bool UQuestBuilderNode_Objective::HasBeenVisited()
{
    return Super::HasBeenVisited() && (IsObjectiveCompleted() || IsObjectiveFailed());
}

#if WITH_EDITOR
FText UQuestBuilderNode_Objective::GetNodeTitle() const
{
    return NodeName.Len() ? FText::FromString(NodeName) :
        GetNodeDisplayName().ToString().Len() ? GetNodeDisplayName() :
        FText::FromString(GetShortTypeName(this));
}

void UQuestBuilderNode_Objective::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}

FText UQuestBuilderNode_Objective::GetNodeDescription() const
{
    return Description;
}
#endif

#undef LOCTEXT_NAMESPACE