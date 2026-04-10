// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/TimerHandle.h"
#include "UObject/NoExportTypes.h"
#include "Quest.generated.h"

class UOrionDecorator;
class UQuestBuilderNode;
class UQuestBuilderNode_Objective;
class UQuestBuilderEdge;
class UQuestBuilderNode_Root;

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	/**Quest is Active*/
	E_Active		UMETA(DisplayName = "ACTIVE"),
	/**Quest is Completed*/
	E_Complete		UMETA(DisplayName = "COMPLETE"),
	/**Quest is Fail*/
	E_Fail			UMETA(DisplayName = "FAIL"),
	/**Quest is Unlocked, Waiting to be Activated*/
	E_Unlocked		UMETA(DisplayName = "UNLOCKED"),
	/**Quest is Locked, Waiting to be Unlocked*/
	E_Locked		UMETA(DisplayName = "LOCKED"),
};

DECLARE_MULTICAST_DELEGATE(FQuestSetupFinishedSignature);

UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UQuest : public UObject
{
	GENERATED_BODY()
public:
	UQuest();

	UPROPERTY(VisibleDefaultsOnly, Category = "QuestAssets")
	TObjectPtr<UQuestBuilderGraph> QuestGraph;

	/** Quest ID */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FName ID;

	/**Category For this Quest*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest")
	FText QuestCategory;

	UPROPERTY()
	FString SharedCategoryName;

	/** The Category data of this node may be shared */
	UPROPERTY()
	bool bSharedCategory;

	UPROPERTY()
	FGuid SharedCategoryGuid;

	UPROPERTY()
	int32 SharedCategoryIdx;
	
	/** Gameplay Tag for this quest, each tag should be unique*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest", meta = (Categories = "Quest"))
	FGameplayTag QuestTag;

	/** Your Quest Name*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest", meta = (MultiLine = true))
		FText QuestName;

	/** Description for your quest*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest", meta = (MultiLine = true))
		FText Description; 

	/**
	* Change the Quest State to this value the first time this quest registered
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest",  meta=(ValidEnumValues="E_Active, E_Unlocked, E_Locked"))
	EQuestState DesiredQuestState = EQuestState::E_Active;
	
	/**Can Abort Quest*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest", meta = (DisplayName = "CanAbortQuest"))
		bool bCanQuestBeAborted = false;

	/**Can Retake Quest*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Quest", meta = (DisplayName = "CanRetakeQuest"))
		bool bCanRetakeQuest = false;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TObjectPtr<UQuestBuilderNode_Objective> CurrentNavigatedObjective;

	/** List of All Visited Node IDs in Sequence*/
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TArray<FName> VisitedNodeIDs;

	/**Your quest state, will be changed based on condition*/
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		EQuestState QuestState = EQuestState::E_Locked;
	
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TArray<TObjectPtr<UQuestBuilderNode>> CurrentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
		TArray< TObjectPtr<UQuestBuilderNode>> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
		TArray< TObjectPtr<UQuestBuilderNode>> AllNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
		TMap<FName, TObjectPtr<UQuestBuilderNode>> NodeMap;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TObjectPtr<APlayerController> OwningController;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TObjectPtr < UQuestComponent> QuestComponent;

	/**Nodes that need to be setup before finished loading map*/
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TArray<TObjectPtr<UQuestBuilderNode>> PrerequisiteNodes;
	
public:
	bool bSetupCompleted;
	/**Called when quest setup finished*/
	FQuestSetupFinishedSignature OnQuestSetupFinished;
	struct FTimerHandle TimerHandle_CheckPrerequisites;

public:

	virtual bool Initialize(class UQuestComponent* InitializingComp);
	virtual void Deinitialize();
	virtual UWorld* GetWorld() const override;
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateQuest();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<UQuestBuilderNode*> GetVisitedNodes();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<UQuestBuilderNode_Objective*> GetCurrentObjectives();
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SortVisitedNodes();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	UQuestBuilderNode_Objective* GetNavigatedObjective();
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
		void BeginNode(UQuestBuilderNode* InNode, bool bLaunchEventOnLoad = false);
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
		void BeginMultipleNodes(TArray<UQuestBuilderNode*> InNodes, bool bLaunchEventOnLoad = false);
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
		int GetLevelNum() const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
		void GetNodesByLevel(int Level, TArray<UQuestBuilderNode*>& Nodes);

	UFUNCTION(BlueprintCallable, Category = "Quest")
		APlayerController* GetOwningController();

	UFUNCTION(BlueprintCallable, Category = "Quest")
		APawn* GetOwningPawn();

	UFUNCTION(BlueprintCallable, Category = "Quest")
		UQuestComponent* GetQuestComponent();

	UFUNCTION(BlueprintCallable, Category = "Quest")
		UQuestBuilderGraph* GetOwningQuestGraph() const;
	void ClearGraph();

	virtual void EvaluateUniqueID();


	void RestartQuest(UQuestBuilderNode* InNode = nullptr, const bool bNotifyQuestAdded = false);

	void ActivateQuest(const bool bNotifyQuestAdded = true, const bool bLaunchEventOnLoad = false);

public:
	virtual void Serialize(FArchive& Ar) override;

	//Quest Sharing
	void MakeQuestSettingShareable(FString ShareName, bool bInitialize = false);
	void UnshareQuestSetting();
	void UseSharedQuestSetting(const UQuest* Quest);

	void CopyQuestSettings(const UQuest* SrcQuest);
	void PropagateQuestSettings();

	void GetNodePrerequisites(TArray<UQuestBuilderNode*>& Nodes);
	void SetupPrerequisites();
	void CheckQuestPrerequisites();


#if WITH_EDITOR
public:
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	//~ End UObject Interface
#endif
};

class ORIONRPG_API IQuestSharedDataHelper
{
public:
	void MakeSureGuidExists(UQuest* Quest);

protected:
	virtual bool CheckIfQuestShouldShareData(const UQuest* QuestA, const UQuest* QuestB) = 0;
	virtual bool CheckIfHasDataToShare(const UQuest* Quest) = 0;
	virtual void ShareData(UQuest* QuestWhoWantsToShare, const UQuest* ShareFrom) = 0;
	virtual FString& AccessShareDataName(UQuest* Quest) = 0;
	virtual FGuid& AccessShareDataGuid(UQuest* Quest) = 0;
};

//////////////////////////////////////////////////////////////////////////
// FQuestSharedSettingHelper

class ORIONRPG_API FQuestSharedSettingHelper : public IQuestSharedDataHelper
{
protected:
	virtual bool CheckIfQuestShouldShareData(const UQuest* QuestA, const UQuest* QuestB) override;
	virtual bool CheckIfHasDataToShare(const UQuest* Quest) override;
	virtual void ShareData(UQuest* QuestWhoWantsToShare, const UQuest* ShareFrom) override;
	virtual FString& AccessShareDataName(UQuest* Quest) override;
	virtual FGuid& AccessShareDataGuid(UQuest* Quest) override;
};