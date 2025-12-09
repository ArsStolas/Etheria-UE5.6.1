/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestComponent" - Source
 */

#include "Components/Quests/QuestComponent.h"
#include "Components/Quests/System/QuestDefinition.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UQuestComponent::UQuestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UQuestComponent::BeginPlay()
{
    Super::BeginPlay();

    // Register all available quest definitions for fast lookup.
    for (UQuestDefinition* Def : availableQuests)
    {
        if (Def && !Def->questId.IsNone())
        {
            RegisterQuestDefinition(Def);
            if (Def->bAutoAccept)
            {
                StartQuest(Def);
            }
        }
    }
}

#pragma region API_BLUEPRINT

bool UQuestComponent::GetQuestState(FName QuestId, FQuestRuntimeState& OutState) const
{
    if (const FQuestRuntimeState* Found = FindActiveQuestState(QuestId))
    {
        OutState = *Found;
        return true;
    }
    return false;
}

bool UQuestComponent::StartQuest(UQuestDefinition* QuestDef)
{
    if (!QuestDef || QuestDef->questId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Quest] Invalid QuestDefinition on %s"), *GetOwner()->GetName());
        return false;
    }

    const FName QuestId = QuestDef->questId;

    // Ensure we know about this definition.
    RegisterQuestDefinition(QuestDef);

    if (const FQuestRuntimeState* ExistingState = FindActiveQuestState(QuestId))
    {
        if (ExistingState->state == EQuestState::Active || ExistingState->state == EQuestState::Completed)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[Quest] Quest %s already active or completed on %s"),
                *QuestId.ToString(), *GetOwner()->GetName());
            return false;
        }
    }

    if (bRememberCompletedQuests && completedQuestIds.Contains(QuestId))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Quest] Quest %s already completed on %s"),
            *QuestId.ToString(), *GetOwner()->GetName());
        return false;
    }

    if (!ArePrerequisitesMet(QuestDef))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Quest] Prerequisites missing for quest %s on %s"),
            *QuestId.ToString(), *GetOwner()->GetName());
        return false;
    }

    FQuestRuntimeState NewState;
    InitializeRuntimeStateFromDefinition(QuestDef, NewState);
    NewState.state = EQuestState::Active;

    activeQuests.Add(QuestId, NewState);

    // Delegates for UI / systems.
    OnQuestStateChanged.Broadcast(QuestId, EQuestState::Active);
    OnQuestStarted.Broadcast(QuestId, QuestDef->category);
    OnQuestListChanged.Broadcast(QuestId);

    UE_LOG(LogTemp, Log, TEXT("[Quest] StartQuest %s on %s"), *QuestId.ToString(), *GetOwner()->GetName());
    return true;
}

bool UQuestComponent::AbandonQuest(FName QuestId)
{
    FQuestRuntimeState* State = FindActiveQuestState(QuestId);
    if (!State)
    {
        return false;
    }

    activeQuests.Remove(QuestId);

    OnQuestStateChanged.Broadcast(QuestId, EQuestState::NotStarted);
    OnQuestAbandoned.Broadcast(QuestId);
    OnQuestListChanged.Broadcast(QuestId);

    UE_LOG(LogTemp, Log, TEXT("[Quest] AbandonQuest %s on %s"), *QuestId.ToString(), *GetOwner()->GetName());
    return true;
}

bool UQuestComponent::IsQuestActive(FName QuestId) const
{
    const FQuestRuntimeState* State = activeQuests.Find(QuestId);
    return State && State->state == EQuestState::Active;
}

bool UQuestComponent::IsQuestCompleted(FName QuestId) const
{
    return completedQuestIds.Contains(QuestId);
}

#pragma endregion API_BLUEPRINT

#pragma region GAMEPLAY_NOTIFICATIONS

void UQuestComponent::NotifyItemCollected(FName ItemId, int32 DeltaCount)
{
    if (DeltaCount <= 0 || ItemId.IsNone())
    {
        return;
    }

    // On capture les clés pour ne pas itérer directement sur le TMap
    TArray<FName> QuestIds;
    activeQuests.GenerateKeyArray(QuestIds);

    for (const FName QuestId : QuestIds)
    {
        FQuestRuntimeState* QuestStatePtr = activeQuests.Find(QuestId);
        if (!QuestStatePtr)
        {
            continue;
        }

        FQuestRuntimeState& QuestState = *QuestStatePtr;
        if (QuestState.state != EQuestState::Active)
        {
            continue;
        }

        UQuestDefinition* QuestDef = FindQuestDefinition(QuestId);
        if (!QuestDef)
        {
            continue;
        }

        const int32 ObjectiveCount = FMath::Min(
            QuestDef->objectives.Num(),
            QuestState.objectives.Num()
        );

        for (int32 Index = 0; Index < ObjectiveCount; ++Index)
        {
            const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
            FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

            if (ObjDef.type != EQuestObjectiveType::CollectItem)
            {
                continue;
            }

            if (!IsObjectiveCurrentlyRelevant(QuestState, Index, ObjDef, QuestDef))
            {
                continue;
            }

            if (ObjDef.targetId != NAME_None && ObjDef.targetId != ItemId)
            {
                continue;
            }

            ObjRuntime.currentCount = FMath::Clamp(
                ObjRuntime.currentCount + DeltaCount,
                0,
                ObjRuntime.requiredCount
            );

            HandleObjectiveProgress(QuestState, ObjDef, ObjRuntime);
        }
    }
    CleanupCompletedQuests();
}

void UQuestComponent::NotifyActorKilled(AActor* Victim, AActor* Killer)
{
    if (!Victim || Killer != GetOwner())
    {
        return;
    }

    const UClass* VictimClass = Victim->GetClass();

    TArray<FName> QuestIds;
    activeQuests.GenerateKeyArray(QuestIds);

    for (const FName QuestId : QuestIds)
    {
        FQuestRuntimeState* QuestStatePtr = activeQuests.Find(QuestId);
        if (!QuestStatePtr)
        {
            continue;
        }

        FQuestRuntimeState& QuestState = *QuestStatePtr;
        if (QuestState.state != EQuestState::Active)
        {
            continue;
        }

        UQuestDefinition* QuestDef = FindQuestDefinition(QuestId);
        if (!QuestDef)
        {
            continue;
        }

        const int32 ObjectiveCount = FMath::Min(
            QuestDef->objectives.Num(),
            QuestState.objectives.Num()
        );

        for (int32 Index = 0; Index < ObjectiveCount; ++Index)
        {
            const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
            FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

            if (ObjDef.type != EQuestObjectiveType::KillActor)
            {
                continue;
            }

            if (!IsObjectiveCurrentlyRelevant(QuestState, Index, ObjDef, QuestDef))
            {
                continue;
            }

            bool bTagMatches = ObjDef.targetId.IsNone();
            if (!bTagMatches)
            {
                bTagMatches = Victim->ActorHasTag(ObjDef.targetId);
            }

            const bool bClassMatches =
                !ObjDef.targetActorClass ||
                VictimClass->IsChildOf(ObjDef.targetActorClass);

            if (!bTagMatches || !bClassMatches)
            {
                continue;
            }

            ObjRuntime.currentCount = FMath::Clamp(
                ObjRuntime.currentCount + 1,
                0,
                ObjRuntime.requiredCount
            );

            HandleObjectiveProgress(QuestState, ObjDef, ObjRuntime);
        }
    }
    CleanupCompletedQuests();
}

void UQuestComponent::NotifyLocationReached(FName LocationId, AActor* LocationActor)
{
    if (LocationId.IsNone())
    {
        return;
    }

    TArray<FName> QuestIds;
    activeQuests.GenerateKeyArray(QuestIds);

    for (const FName QuestId : QuestIds)
    {
        FQuestRuntimeState* QuestStatePtr = activeQuests.Find(QuestId);
        if (!QuestStatePtr)
        {
            continue;
        }

        FQuestRuntimeState& QuestState = *QuestStatePtr;
        if (QuestState.state != EQuestState::Active)
        {
            continue;
        }

        UQuestDefinition* QuestDef = FindQuestDefinition(QuestId);
        if (!QuestDef)
        {
            continue;
        }

        const int32 ObjectiveCount = FMath::Min(
            QuestDef->objectives.Num(),
            QuestState.objectives.Num()
        );

        for (int32 Index = 0; Index < ObjectiveCount; ++Index)
        {
            const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
            FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

            if (ObjDef.type != EQuestObjectiveType::ReachLocation)
            {
                continue;
            }

            if (!IsObjectiveCurrentlyRelevant(QuestState, Index, ObjDef, QuestDef))
            {
                continue;
            }

            if (ObjDef.targetId != NAME_None && ObjDef.targetId != LocationId)
            {
                continue;
            }

            ObjRuntime.currentCount = ObjRuntime.requiredCount;
            HandleObjectiveProgress(QuestState, ObjDef, ObjRuntime);
        }
    }
    CleanupCompletedQuests();
}

void UQuestComponent::NotifyCustomEvent(FName EventId, int32 Amount)
{
    if (EventId.IsNone() || Amount <= 0)
    {
        return;
    }

    TArray<FName> QuestIds;
    activeQuests.GenerateKeyArray(QuestIds);

    for (const FName QuestId : QuestIds)
    {
        FQuestRuntimeState* QuestStatePtr = activeQuests.Find(QuestId);
        if (!QuestStatePtr)
        {
            continue;
        }

        FQuestRuntimeState& QuestState = *QuestStatePtr;
        if (QuestState.state != EQuestState::Active)
        {
            continue;
        }

        UQuestDefinition* QuestDef = FindQuestDefinition(QuestId);
        if (!QuestDef)
        {
            continue;
        }

        const int32 ObjectiveCount = FMath::Min(
            QuestDef->objectives.Num(),
            QuestState.objectives.Num()
        );

        for (int32 Index = 0; Index < ObjectiveCount; ++Index)
        {
            const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
            FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

            if (ObjDef.type != EQuestObjectiveType::Custom)
            {
                continue;
            }

            if (!IsObjectiveCurrentlyRelevant(QuestState, Index, ObjDef, QuestDef))
            {
                continue;
            }

            if (ObjDef.targetId != NAME_None && ObjDef.targetId != EventId)
            {
                continue;
            }

            ObjRuntime.currentCount = FMath::Clamp(
                ObjRuntime.currentCount + Amount,
                0,
                ObjRuntime.requiredCount
            );

            HandleObjectiveProgress(QuestState, ObjDef, ObjRuntime);
        }
    }
    CleanupCompletedQuests();
}

#pragma endregion GAMEPLAY_NOTIFICATIONS

#pragma region SAVE_LOAD

void UQuestComponent::BuildSaveSnapshot(FString& OutSerializedJson) const
{
    // Placeholder: integrate with your SaveGame system.
    // You can serialize activeQuests and completedQuestIds to JSON or a custom struct.
    OutSerializedJson = TEXT("");
}

void UQuestComponent::ApplySaveSnapshot(const FString& SerializedJson)
{
    // Placeholder: integrate with your SaveGame system.
    // Here to restore activeQuests and completedQuestIds from a save SerializedJson.
}

#pragma endregion SAVE_LOAD

#pragma region INTERNAL

FQuestRuntimeState* UQuestComponent::FindActiveQuestState(FName QuestId)
{
    return activeQuests.Find(QuestId);
}

const FQuestRuntimeState* UQuestComponent::FindActiveQuestState(FName QuestId) const
{
    return activeQuests.Find(QuestId);
}

void UQuestComponent::InitializeRuntimeStateFromDefinition(const UQuestDefinition* QuestDef, FQuestRuntimeState& OutState)
{
    if (!QuestDef)
    {
        return;
    }

    OutState.questId = QuestDef->questId;
    OutState.category = QuestDef->category;
    OutState.bSequentialObjectives = QuestDef->bSequentialObjectives;
    OutState.currentObjectiveIndex = 0;
    OutState.state = EQuestState::NotStarted;

    OutState.objectives.Reset();

    for (const FQuestObjectiveDef& ObjDef : QuestDef->objectives)
    {
        FQuestObjectiveRuntimeState Runtime;
        Runtime.objectiveId = ObjDef.objectiveId;
        Runtime.currentCount = 0;
        Runtime.requiredCount = FMath::Max(ObjDef.requiredCount, 1);
        Runtime.bCompleted = false;
        Runtime.bIsOptional = ObjDef.bIsOptional;

        OutState.objectives.Add(Runtime);
    }
}

bool UQuestComponent::ArePrerequisitesMet(const UQuestDefinition* QuestDef) const
{
    if (!QuestDef)
    {
        return false;
    }

    for (const FName& PreId : QuestDef->prerequisiteQuestIds)
    {
        if (!completedQuestIds.Contains(PreId))
        {
            return false;
        }
    }

    return true;
}

void UQuestComponent::HandleObjectiveProgress(FQuestRuntimeState& QuestState, const FQuestObjectiveDef& ObjectiveDef, FQuestObjectiveRuntimeState& ObjRuntime)
{
    // Clamp to required count and mark completed if needed.
    ObjRuntime.currentCount = FMath::Clamp(ObjRuntime.currentCount, 0, ObjRuntime.requiredCount);

    if (ObjRuntime.currentCount >= ObjRuntime.requiredCount && !ObjRuntime.bCompleted)
    {
        ObjRuntime.bCompleted = true;

        // Log when an objective is fully completed.
        UE_LOG(LogTemp, Log, TEXT("[Quest] Objective %s completed in quest %s on %s (%d / %d)"),
            *ObjRuntime.objectiveId.ToString(),
            *QuestState.questId.ToString(),
            *GetOwner()->GetName(),
            ObjRuntime.currentCount,
            ObjRuntime.requiredCount
        );
        
        OnQuestObjectiveCompleted.Broadcast(QuestState.questId, ObjRuntime.objectiveId);
    }

    OnQuestObjectiveProgress.Broadcast(
        QuestState.questId,
        ObjRuntime.objectiveId,
        ObjRuntime.currentCount,
        ObjRuntime.requiredCount
    );

    UQuestDefinition** DefPtr = questDefinitionsById.Find(QuestState.questId);
    UQuestDefinition* QuestDef = DefPtr ? *DefPtr : nullptr;
    EvaluateQuestCompletion(QuestState, QuestDef);
}

bool UQuestComponent::IsObjectiveCurrentlyRelevant(const FQuestRuntimeState& QuestState, int32 ObjectiveIndex, const FQuestObjectiveDef& ObjectiveDef, const UQuestDefinition* QuestDef) const
{
    if (!QuestState.objectives.IsValidIndex(ObjectiveIndex) || !QuestDef)
    {
        return false;
    }

    const FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[ObjectiveIndex];
    if (ObjRuntime.bCompleted)
    {
        return false;
    }

    if (!QuestState.bSequentialObjectives)
    {
        // Parallel quests: any non-completed objective can be progressed.
        return true;
    }

    // Sequential quests with grouping support.
    if (ObjectiveDef.groupMode == EQuestObjectiveGroupMode::None || ObjectiveDef.groupId.IsNone())
    {
        // Standalone objective: only current index is active.
        return ObjectiveIndex == QuestState.currentObjectiveIndex;
    }

    // Grouped objective: all non-completed objectives in the current group can progress.
    const FName GroupId = ObjectiveDef.groupId;
    TArray<int32> GroupIndices;
    GroupIndices.Reserve(QuestDef->objectives.Num());

    for (int32 Index = 0; Index < QuestDef->objectives.Num(); ++Index)
    {
        if (QuestDef->objectives[Index].groupId == GroupId)
        {
            GroupIndices.Add(Index);
        }
    }

    if (GroupIndices.Num() == 0)
    {
        return false;
    }

    // Group is considered the "current step" if currentObjectiveIndex is one of its members.
    const bool bGroupIsCurrentStep = GroupIndices.Contains(QuestState.currentObjectiveIndex);
    if (!bGroupIsCurrentStep)
    {
        return false;
    }

    // If the group is already satisfied, no objective in it is relevant anymore.
    bool bGroupSatisfied = false;

    if (ObjectiveDef.groupMode == EQuestObjectiveGroupMode::Any)
    {
        for (int32 Idx : GroupIndices)
        {
            if (QuestState.objectives.IsValidIndex(Idx) && QuestState.objectives[Idx].bCompleted)
            {
                bGroupSatisfied = true;
                break;
            }
        }
    }
    else if (ObjectiveDef.groupMode == EQuestObjectiveGroupMode::All)
    {
        bGroupSatisfied = true;
        for (int32 Idx : GroupIndices)
        {
            if (!QuestState.objectives.IsValidIndex(Idx))
            {
                continue;
            }

            const FQuestObjectiveRuntimeState& GroupObjRuntime = QuestState.objectives[Idx];
            const FQuestObjectiveDef& GroupObjDef = QuestDef->objectives[Idx];

            if (!GroupObjRuntime.bCompleted && !GroupObjDef.bIsOptional)
            {
                bGroupSatisfied = false;
                break;
            }
        }
    }

    if (bGroupSatisfied)
    {
        return false;
    }

    return true;
}

void UQuestComponent::EvaluateQuestCompletion(FQuestRuntimeState& QuestState, const UQuestDefinition* QuestDef)
{
    if (QuestState.state != EQuestState::Active)
    {
        return;
    }

    if (!QuestDef)
    {
        return;
    }

    const int32 ObjectiveCount = FMath::Min(QuestDef->objectives.Num(), QuestState.objectives.Num());

    // Build group runtime info.
    struct FObjectiveGroupRuntime
    {
        EQuestObjectiveGroupMode Mode = EQuestObjectiveGroupMode::None;
        bool bRequired = false;
        bool bSatisfied = false;
        TArray<int32> Indices;
    };

    TMap<FName, FObjectiveGroupRuntime> GroupInfos;

    for (int32 Index = 0; Index < ObjectiveCount; ++Index)
    {
        const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
        const FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

        if (ObjDef.groupMode == EQuestObjectiveGroupMode::None || ObjDef.groupId.IsNone())
        {
            continue;
        }

        FObjectiveGroupRuntime& Group = GroupInfos.FindOrAdd(ObjDef.groupId);
        Group.Mode = ObjDef.groupMode;
        Group.Indices.Add(Index);

        if (!ObjDef.bIsOptional)
        {
            Group.bRequired = true;
        }

        if (ObjRuntime.bCompleted)
        {
            if (Group.Mode == EQuestObjectiveGroupMode::Any)
            {
                Group.bSatisfied = true;
            }
        }
    }

    // For "All" mode, compute satisfaction now that we know all indices.
    for (auto& Pair : GroupInfos)
    {
        FObjectiveGroupRuntime& Group = Pair.Value;
        if (Group.Mode == EQuestObjectiveGroupMode::All)
        {
            bool bAllCompleted = true;

            for (int32 Idx : Group.Indices)
            {
                if (!QuestState.objectives.IsValidIndex(Idx))
                {
                    continue;
                }

                const FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Idx];
                const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Idx];

                if (!ObjRuntime.bCompleted && !ObjDef.bIsOptional)
                {
                    bAllCompleted = false;
                    break;
                }
            }

            Group.bSatisfied = bAllCompleted;
        }
        else if (Group.Mode == EQuestObjectiveGroupMode::Any)
        {
            // If no objective in an Any group was completed earlier, bSatisfied stays false.
        }
    }

    // Now evaluate if all required objectives/groups are satisfied.
    bool bAllRequiredCompleted = true;

    // 1) Standalone objectives (no group).
    for (int32 Index = 0; Index < ObjectiveCount; ++Index)
    {
        const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
        const FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

        if (ObjDef.groupMode != EQuestObjectiveGroupMode::None && !ObjDef.groupId.IsNone())
        {
            // Grouped objectives are evaluated as part of their group.
            continue;
        }

        if (!ObjDef.bIsOptional && !ObjRuntime.bCompleted)
        {
            bAllRequiredCompleted = false;
            break;
        }
    }

    // 2) Grouped objectives.
    if (bAllRequiredCompleted)
    {
        for (auto& Pair : GroupInfos)
        {
            const FObjectiveGroupRuntime& Group = Pair.Value;

            if (!Group.bRequired)
            {
                // Entire group is optional if all objectives in it are optional.
                continue;
            }

            if (!Group.bSatisfied)
            {
                bAllRequiredCompleted = false;
                break;
            }
        }
    }

    if (!bAllRequiredCompleted)
    {
        // If sequential, advance currentObjectiveIndex to next non-satisfied required objective or group.
        if (QuestState.bSequentialObjectives)
        {
            int32 NextIndex = QuestState.currentObjectiveIndex;

            for (int32 Index = 0; Index < ObjectiveCount; ++Index)
            {
                const FQuestObjectiveDef& ObjDef = QuestDef->objectives[Index];
                const FQuestObjectiveRuntimeState& ObjRuntime = QuestState.objectives[Index];

                if (ObjDef.groupMode == EQuestObjectiveGroupMode::None || ObjDef.groupId.IsNone())
                {
                    if (!ObjDef.bIsOptional && !ObjRuntime.bCompleted)
                    {
                        NextIndex = Index;
                        break;
                    }
                }
                else
                {
                    const FObjectiveGroupRuntime* GroupPtr = GroupInfos.Find(ObjDef.groupId);
                    if (!GroupPtr)
                    {
                        continue;
                    }

                    if (!GroupPtr->bRequired || GroupPtr->bSatisfied)
                    {
                        continue;
                    }

                    NextIndex = Index;
                    break;
                }
            }

            QuestState.currentObjectiveIndex = NextIndex;
        }

        return;
    }

    // All required pieces are satisfied -> complete quest.
    QuestState.state = EQuestState::Completed;
    completedQuestIds.Add(QuestState.questId);

    OnQuestStateChanged.Broadcast(QuestState.questId, EQuestState::Completed);
    OnQuestListChanged.Broadcast(QuestState.questId);

    UE_LOG(LogTemp, Log, TEXT("[Quest] Quest %s completed on %s"),
        *QuestState.questId.ToString(),
        *GetOwner()->GetName());

    // Auto-start follow-up quests, if any (typically used for main quest chains).
    if (QuestDef)
    {
        for (const FName& NextId : QuestDef->nextQuestIds)
        {
            if (NextId.IsNone())
            {
                continue;
            }

            UQuestDefinition* NextDef = FindQuestDefinition(NextId);
            if (!NextDef)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Quest] Follow-up quest %s not found for quest %s on %s"),
                    *NextId.ToString(),
                    *QuestState.questId.ToString(),
                    *GetOwner()->GetName());
                continue;
            }

            // Will internally check prerequisites, already completed, already active, etc.
            StartQuest(NextDef);
        }
    }
}

void UQuestComponent::RegisterQuestDefinition(UQuestDefinition* QuestDef)
{
    if (!QuestDef || QuestDef->questId.IsNone())
    {
        return;
    }

    questDefinitionsById.FindOrAdd(QuestDef->questId) = QuestDef;
}

UQuestDefinition* UQuestComponent::FindQuestDefinition(FName QuestId) const
{
    if (UQuestDefinition* const* Ptr = questDefinitionsById.Find(QuestId))
    {
        return *Ptr;
    }
    return nullptr;
}

void UQuestComponent::CleanupCompletedQuests()
{
    // On copie les clés pour pouvoir Remove pendant la boucle
    TArray<FName> QuestIds;
    activeQuests.GenerateKeyArray(QuestIds);

    for (const FName QuestId : QuestIds)
    {
        FQuestRuntimeState* State = activeQuests.Find(QuestId);
        if (!State)
        {
            continue;
        }

        // On ne garde dans activeQuests que les vraies quêtes actives
        if (State->state != EQuestState::Active)
        {
            activeQuests.Remove(QuestId);
        }
    }
}

#pragma endregion INTERNAL
