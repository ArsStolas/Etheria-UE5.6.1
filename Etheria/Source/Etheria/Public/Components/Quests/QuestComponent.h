/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/Quests/System/QuestTypes.h"
#include "QuestComponent.generated.h"

class UQuestDefinition;
class AActor;

/** Quest id + state change delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestStateChanged, FName, QuestId, EQuestState, NewState);

/** Quest started delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestStarted, FName, QuestId, EQuestCategory, Category);

/** Quest abandoned delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestAbandoned, FName, QuestId);

/** Quest failed delegate (reserved for future use). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestFailed, FName, QuestId, FText, Reason);

/** Quest list changed (quest added, completed, abandoned). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestListChanged, FName, QuestId);

/** Objective progress delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnQuestObjectiveProgress, FName, QuestId, FName, ObjectiveId, int32, Current, int32, Required);

/** Objective completed delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveCompleted, FName, QuestId, FName, ObjectiveId);

/**
 * Component that tracks quests and objective progress for its owner (player or AI).
 * All external systems (inventory, combat, triggers) notify this component through simple API calls.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UQuestComponent : public UActorComponent
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    
public:
    UQuestComponent();

#pragma region API_BLUEPRINT

public:
    /**
     * @brief Returns the runtime state for a given quest id, if active.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool GetQuestState(FName QuestId, FQuestRuntimeState& OutState) const;

    /**
     * @brief Tries to start the given quest on this owner.
     * Checks prerequisites and prevents duplicates.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool StartQuest(UQuestDefinition* QuestDef);

    /**
     * @brief Abandons the quest (if active). Does not mark it as completed.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool AbandonQuest(FName QuestId);

    /**
     * @brief Returns true if this quest is currently active (state == Active).
     */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool IsQuestActive(FName QuestId) const;

    /**
     * @brief Returns true if this quest was completed in the past.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool IsQuestCompleted(FName QuestId) const;

#pragma endregion API_BLUEPRINT

#pragma region GAMEPLAY_NOTIFICATIONS

public:
    /**
     * @brief Notify the component that some items were collected.
     * Called for example by InventoryComponent delegates.
     *
     * NOTE: In Etheria, you will bind this to InventoryComponent->OnItemAdded.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest|Notify")
    void NotifyItemCollected(FName ItemId, int32 DeltaCount);

    /**
     * @brief Notify that an actor was killed by this owner.
     * Usually called from Combat/Health system when an enemy dies.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest|Notify")
    void NotifyActorKilled(AActor* Victim, AActor* Killer);

    /**
     * @brief Notify that a specific quest location has been reached.
     * Typically called by AQuestLocationTrigger when overlapped by the owner.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest|Notify")
    void NotifyLocationReached(FName LocationId, AActor* LocationActor);

    /**
     * @brief Generic custom hook that designers can use for script-driven objectives.
     */
    UFUNCTION(BlueprintCallable, Category = "Quest|Notify")
    void NotifyCustomEvent(FName EventId, int32 Amount);

#pragma endregion GAMEPLAY_NOTIFICATIONS

#pragma region SAVE_LOAD

public:
    /** Serialize current quest state into a simple string snapshot (placeholder). */
    UFUNCTION(BlueprintCallable, Category = "Quest|Save")
    void BuildSaveSnapshot(FString& OutSerializedJson) const;

    /** Restore quest state from serialized data (placeholder). */
    UFUNCTION(BlueprintCallable, Category = "Quest|Save")
    void ApplySaveSnapshot(const FString& SerializedJson);

#pragma endregion SAVE_LOAD

#pragma region DESIGNER_SETUP

public:
    /** Quests that this owner can potentially start (e.g. for auto-accept or quest log). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    TArray<TObjectPtr<UQuestDefinition>> availableQuests;

    /** If true, completed quest ids are kept even after abandoning states. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    bool bRememberCompletedQuests = true;

#pragma endregion DESIGNER_SETUP

#pragma region EVENTS

public:
    /** Broadcast when a quest changes state (Active / Completed / Failed / NotStarted). */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestStateChanged OnQuestStateChanged;

    /** Broadcast when a quest is started. */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestStarted OnQuestStarted;

    /** Broadcast when a quest is abandoned. */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestAbandoned OnQuestAbandoned;

    /** Broadcast when a quest fails. */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestFailed OnQuestFailed;

    /** Broadcast when quests are added / completed / abandoned (for quest log refresh). */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestListChanged OnQuestListChanged;

    /** Broadcast when an objective makes progress. */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestObjectiveProgress OnQuestObjectiveProgress;

    /** Broadcast when an objective is fully completed. */
    UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
    FOnQuestObjectiveCompleted OnQuestObjectiveCompleted;

#pragma endregion EVENTS

#pragma region INTERNAL

protected:
    /** All currently active quests keyed by quest id. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Internal")
    TMap<FName, FQuestRuntimeState> activeQuests;

    /** Completed quests (for prerequisites / main vs side progression). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Internal")
    TSet<FName> completedQuestIds;

    /** Mapping from quest id to its data asset definition. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Internal")
    TMap<FName, UQuestDefinition*> questDefinitionsById;

    /** Find active quest runtime state pointer. Returns nullptr if not active. */
    FQuestRuntimeState* FindActiveQuestState(FName QuestId);

    /** Const variant. */
    const FQuestRuntimeState* FindActiveQuestState(FName QuestId) const;

    /** Internal helper: builds a runtime state from the quest definition. */
    void InitializeRuntimeStateFromDefinition(const UQuestDefinition* QuestDef, FQuestRuntimeState& OutState);

    /** Check if prerequisites are met for this quest definition. */
    bool ArePrerequisitesMet(const UQuestDefinition* QuestDef) const;

    /** Try to advance objectives after some progress change. */
    void HandleObjectiveProgress(FQuestRuntimeState& QuestState, const FQuestObjectiveDef& ObjectiveDef, FQuestObjectiveRuntimeState& ObjRuntime);

    /** Returns true if this objective is currently "active" given quest sequential/parallel flags. */
    bool IsObjectiveCurrentlyRelevant(const FQuestRuntimeState& QuestState, int32 ObjectiveIndex, const FQuestObjectiveDef& ObjectiveDef, const UQuestDefinition* QuestDef) const;

    /** Check if all required objectives are completed and possibly complete the quest. */
    void EvaluateQuestCompletion(FQuestRuntimeState& QuestState, const UQuestDefinition* QuestDef);

    /** Helper to register a quest definition in the internal map. */
    void RegisterQuestDefinition(UQuestDefinition* QuestDef);

    /** Helper to find a quest definition by id, if it was registered. */
    UQuestDefinition* FindQuestDefinition(FName QuestId) const;

    /** Remove quests that are no longer Active from the activeQuests map. */
    void CleanupCompletedQuests();

#pragma endregion INTERNAL
};
