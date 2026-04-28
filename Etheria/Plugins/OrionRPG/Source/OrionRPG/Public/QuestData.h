// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NativeGameplayTags.h"
#include "Quest.h"
#include "UObject/NoExportTypes.h"
#include "QuestData.generated.h"

//Gameplay Tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Quest);

//Enum
UENUM(BlueprintType)
enum class EObjectiveState : uint8
{
	E_Active		UMETA(DisplayName = "ACTIVE"),
	E_Completed		UMETA(DisplayName = "COMPLETED"),
	E_Failed		UMETA(DisplayName = "FAILED"),
};


/** Node Data To Be Saved */
USTRUCT()
struct FNodeData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FName NodeID;

	UPROPERTY(SaveGame)
	bool bWaitForBranching = false;

	UPROPERTY(SaveGame)
	int CurrentProgress = 0;

	UPROPERTY(SaveGame)
	EObjectiveState ObjectiveState = EObjectiveState::E_Active;
};



/** Quest Data To Be Saved */
USTRUCT()
struct FQuestData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	TArray<FName> CurrentNodeIDs;

	UPROPERTY(SaveGame)
	EQuestState QuestState = EQuestState::E_Locked;

	/** Map of Node ID and it's Node Data Struct */
	UPROPERTY(SaveGame)
	TMap<FName, FNodeData> NodeMapData;

	/** List of All Visited Node IDs in Sequence*/
	UPROPERTY(SaveGame)
	TArray<FName> VisitedNodeIDs;
};

//Store Data of Quest Category
USTRUCT(BlueprintType)
struct FQuestCategory
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuestCategory)
	FText Category;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = FQuestCategory)
	TArray<UQuest*> QuestList;


};

//Data Handle for quest object
USTRUCT(BlueprintType)
struct FQuestDataHandle
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuestDataHandle)
	FGameplayTag QuestTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuestDataHandle)
	FGameplayTag NodeTag;


};


