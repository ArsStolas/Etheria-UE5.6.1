/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestDefinition" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/Quests/System/QuestTypes.h"
#include "QuestDefinition.generated.h"

/**
 * Data-driven quest config.
 * Designers will create instances of this asset to define quests.
 */
UCLASS(BlueprintType)
class ETHERIA_API UQuestDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** Global unique quest identifier (used as key inside QuestComponent). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    FName questId = NAME_None;

    /** Title displayed in the quest log / HUD. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    FText title;

    /** Long description displayed in quest details. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = "true"))
    FText description;

    /** Quest main/side category. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    EQuestCategory category = EQuestCategory::SideQuest;

    /** If true, quest is automatically accepted on some condition (ex: entering area). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    bool bAutoAccept = false;

    /** If true, objectives must be done sequentially. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    bool bSequentialObjectives = true;

    /** Prerequisite quests that must be completed before this one can start. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    TArray<FName> prerequisiteQuestIds;

    /** 
     * Quests that should automatically start when this one is completed.
     * Typically used to chain main quests (Quest A -> Quest B).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Flow")
    TArray<FName> nextQuestIds;
    
    /** Objectives making up this quest, in order. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    TArray<FQuestObjectiveDef> objectives;

    // --- Rewards ---

    /** Simple XP reward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Rewards")
    int32 rewardXP = 0;

    /**
     * Reward items.
     * In your real project this will probably be some ItemId / ItemDefinition reference.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Rewards")
    TArray<FName> rewardItemIds;
};
