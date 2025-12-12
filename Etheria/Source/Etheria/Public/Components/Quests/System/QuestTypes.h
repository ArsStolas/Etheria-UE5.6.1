/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestTypes" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EQuestState : uint8
{
    NotStarted   UMETA(DisplayName = "Not Started"),
    Active       UMETA(DisplayName = "Active"),
    Completed    UMETA(DisplayName = "Completed"),
    Failed       UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EQuestCategory : uint8
{
    MainQuest    UMETA(DisplayName = "Main Quest"),
    SideQuest    UMETA(DisplayName = "Side Quest")
};

UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
    ReachLocation   UMETA(DisplayName = "Reach Location"),
    CollectItem     UMETA(DisplayName = "Collect Item"),
    KillActor       UMETA(DisplayName = "Kill Actor"),
    Interact        UMETA(DisplayName = "Interact"),
    Custom          UMETA(DisplayName = "Custom")
};

/**
 * How objectives inside a group are evaluated.
 * None  -> objective is standalone.
 * Any   -> any completed objective in the group satisfies the group.
 * All   -> all required objectives in the group must be completed.
 */
UENUM(BlueprintType)
enum class EQuestObjectiveGroupMode : uint8
{
    None    UMETA(DisplayName = "None"),
    Any     UMETA(DisplayName = "Any In Group"),
    All     UMETA(DisplayName = "All In Group")
};

/**
 * Basic definition of a single quest objective.
 * This lives in the QuestDefinition DataAsset so designers can configure it.
 */
USTRUCT(BlueprintType)
struct ETHERIA_API FQuestObjectiveDef
{
    GENERATED_BODY()

public:
    /** Unique id for this objective inside the quest (for debug / blueprint hooks). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    FName objectiveId = NAME_None;

    /** Type of objective: kill, collect, reach location, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    EQuestObjectiveType type = EQuestObjectiveType::CollectItem;

    /**
     * Main "target" identifier.
     * - CollectItem: itemId
     * - KillActor: enemy tag / enemy type id
     * - ReachLocation: locationId (used by QuestLocationTrigger)
     * - Interact: interactableId
     * - Custom: event id
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    FName targetId = NAME_None;

    /** Optional specific class to match for KillActor or Interact objectives. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    TSubclassOf<AActor> targetActorClass = nullptr;

    /** If > 1, objective requires multiple occurrences (ex: collect 5 berries, kill 10 enemies). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective", meta = (ClampMin = "1"))
    int32 requiredCount = 1;

    /** If true and objectives are sequential, this one is optional. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    bool bIsOptional = false;

    /** If true, objective can be done in parallel with others (non-sequential quests). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    bool bCanBeCompletedInParallel = true;

    /**
     * Optional group identifier.
     * All objectives sharing the same group id are evaluated together.
     * Example: "PathChoice" group with two ReachLocation objectives (A or B).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective|Grouping")
    FName groupId = NAME_None;

    /**
     * How this objective's group is evaluated.
     * - None: objective is standalone.
     * - Any: any completed objective in the group is enough (branch / choice).
     * - All: all non-optional objectives in the group must be completed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective|Grouping")
    EQuestObjectiveGroupMode groupMode = EQuestObjectiveGroupMode::None;

    /** Optional description override for this specific objective. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    FText description;
};

/**
 * Runtime state for a single objective.
 * This is stored inside the QuestComponent, not in the data asset.
 */
USTRUCT(BlueprintType)
struct ETHERIA_API FQuestObjectiveRuntimeState
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    FName objectiveId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    int32 currentCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    int32 requiredCount = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    bool bCompleted = false;

    /** Mirrors definition optional flag to evaluate completion rules. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    bool bIsOptional = false;
};

/**
 * Runtime state for an entire quest instance on an owner.
 */
USTRUCT(BlueprintType)
struct ETHERIA_API FQuestRuntimeState
{
    GENERATED_BODY()

public:
    /** Quest unique identifier coming from the QuestDefinition. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    FName questId = NAME_None;

    /** Current state of the quest: active, completed, failed, etc. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    EQuestState state = EQuestState::NotStarted;

    /** Category (main / side) for fast access. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    EQuestCategory category = EQuestCategory::SideQuest;

    /** If true, objectives must be completed in order (1, then 2, then 3...). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    bool bSequentialObjectives = true;

    /** Index of the current objective for sequential quests. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    int32 currentObjectiveIndex = 0;

    /** All objective runtime states (one entry per objective in the definition). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
    TArray<FQuestObjectiveRuntimeState> objectives;
};
