// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Event/OrionEvent.h"	
#include "QuestBuilderNode.generated.h"

class UQuestBuilderGraph;
class UQuest;
class UOrionDecorator;
class UOrionEvent;
class UQuestBuilderEdge;


UENUM(BlueprintType)
enum class EQuestNodeLimits : uint8
{
	Unlimited,
	Limited
};


DECLARE_MULTICAST_DELEGATE(FQuestNodeEventFinishedSignature);

UCLASS(Blueprintable, EditInlineNew)
class ORIONRPG_API UQuestBuilderNode : public UObject
{
	GENERATED_BODY()

public:
	UQuestBuilderNode();
	virtual ~UQuestBuilderNode();

	/**Asset ID*/
	UPROPERTY(BlueprintReadOnly, Category = "Description")
		FName ID;

	/**Hide this node information from widget*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Description")
		bool bHidden;

	/** Gameplay Tag for this node, each tag should be unique */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Description", DisplayName = "Node Tag (Optional)", meta = (Categories = "Quest"))
		FGameplayTag NodeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Description", meta = (MultiLine = true))
		FText Description;

	/** node name override*/
	UPROPERTY(Category = "Description", EditAnywhere)
		FString NodeName;
	
	UPROPERTY(Instanced, BlueprintReadOnly, Category = "Events")
		TArray<TObjectPtr<UOrionEvent>> Events;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = Decorators, meta = (DisplayName = "Decorators"))
		TArray<TObjectPtr<UOrionDecorator>> Decorators;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TObjectPtr<UQuestBuilderGraph> QuestGraph;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TObjectPtr<UQuest> Quest;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TArray<TObjectPtr<UQuestBuilderNode>> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TArray<TObjectPtr<UQuestBuilderNode>> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TMap<TObjectPtr<UQuestBuilderNode>, TObjectPtr<UQuestBuilderEdge>> Edges;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		virtual UQuestBuilderEdge* GetEdge(UQuestBuilderNode* ChildNode);

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TObjectPtr<APlayerController> OwningController;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		TObjectPtr < UQuestComponent> QuestComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		bool bWaitForBranching;

	UPROPERTY(BlueprintReadWrite, Category = "Quest")
		bool bInitialized;

	/**Called when all Start Node events finished*/
	FQuestNodeEventFinishedSignature OnAllStartEventFinished;
	
	/**Called when all End Node events finished*/
	FQuestNodeEventFinishedSignature OnAllEndEventFinished;

public:
	virtual void Initialize(bool bLaunchEventOnLoad = false);
	virtual void Deinitialize();

	virtual void Reset();
	virtual void BeginNode();
	virtual void EvaluateNextNode();

	/**Check if this node has been visited*/
	virtual bool HasBeenVisited();
	/**Check if the child node still has active objectives*/
	bool AllChildrenHasBeenVisited();

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
	bool DecoratorConditionMet();
	
	/** initialize any asset related data */
	virtual void InitializeFromAsset(UQuestBuilderGraph& OwningQuestGraph, UQuest& OwningQuest);

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
	bool IsLeafNode() const;
	
	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		APlayerController* GetOwningController() const;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		APawn* GetOwningPawn() const;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		UQuestComponent* GetQuestComponent() const;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		UQuestBuilderGraph* GetOwningQuestGraph() const;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
		UQuest* GetOwningQuest() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "QuestBuilderNode")
		FText GetNodeDisplayName() const;

	virtual FText GetNodeDisplayName_Implementation() const;

	virtual void LaunchStartEventQueue();
	virtual void LaunchEndEventQueue();

	/** returns short name of object's class (BTTaskNode_Wait -> Wait) */
	static FString GetShortTypeName(const UObject* Ob);

	// Allows the Object to get a valid UWorld from it's outer.
	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "QuestBuilderNode")
	FString GetShortTag(FGameplayTag Tag, int32 Level = 1) const;

protected:

	TArray<TObjectPtr<UQuestBuilderNode>> NextNodes;
	TArray<TObjectPtr<UOrionEvent>> StartEventQueue;
	TArray<TObjectPtr<UOrionEvent>> EndEventQueue;

public:
	//////////////////////////////////////////////////////////////////////////
#if WITH_EDITORONLY_DATA


	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		TSubclassOf<UQuestBuilderGraph> CompatibleGraphType;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		FLinearColor BackgroundColor;


	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		EQuestNodeLimits ParentLimitType;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode", meta = (ClampMin = "0", EditCondition = "ParentLimitType == EDialogNodeLimits::Limited", EditConditionHides))
		int32 ParentLimit;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode")
		EQuestNodeLimits ChildrenLimitType;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderNode", meta = (ClampMin = "0", EditCondition = "ChildrenLimitType == EDialogNodeLimits::Limited", EditConditionHides))
		int32 ChildrenLimit;


#endif

#if WITH_EDITOR
	virtual bool IsNameEditable() const;

	virtual FLinearColor GetBackgroundColor() const;

	virtual FText GetNodeTitle() const;

	virtual FText GetNodeDescription() const;

	virtual FText GetEventsText() const;

	virtual FText GetConditionsText() const;

	virtual void SetNodeTitle(const FText& NewTitle);

	virtual bool CanCreateConnection(UQuestBuilderNode* Other, FText& ErrorMessage);

	virtual bool CanCreateConnectionTo(UQuestBuilderNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage);
	virtual bool CanCreateConnectionFrom(UQuestBuilderNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
