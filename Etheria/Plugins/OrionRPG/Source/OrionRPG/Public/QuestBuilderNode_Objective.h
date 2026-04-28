// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode.h"
#include "QuestData.h"
#include "Engine/TimerHandle.h"
#include "QuestBuilderNode_Objective.generated.h"

class UQuestObjective;


UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class ORIONRPG_API UQuestBuilderNode_Objective : public UQuestBuilderNode
{
	GENERATED_BODY()

public:
	UQuestBuilderNode_Objective();

	/**mark the objective as optional and skip this objective if the other non - optional objective has been completed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bOptional;

	/**Quest Indicator that will be shown in the world to indicate the location for the next objective*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Use Quest Indicator?", Category = "Objective")
	bool bUseQuestIndicator;

	/**This objective use amount required to complete objective?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (DisplayName = "UseAmount"))
	bool bUseAmount;

	/**Amount required for your objective*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (EditCondition = "bUseAmount == true", EditConditionHides))
	int RequiredAmount;

	/**Is Objective Tickable?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (DisplayName = "ObjectiveTickable"))
	bool bIsObjectiveTickable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (EditCondition = "bIsObjectiveTickable == true", EditConditionHides))
	float TickInterval;

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	EObjectiveState ObjectiveState;

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	int CurrentProgress = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	float DistanceToTarget;

	bool bObjectiveInitialized;
	bool bSetupCompleted;
	struct FTimerHandle TimerHandle_TickObjective;

public:
	UFUNCTION(BlueprintNativeEvent, DisplayName = "Begin Setup", Category = "Objective")
	void K2_BeginSetup(APlayerController* OwnerController, APawn* ControlledPawn);

	void K2_BeginSetup_Implementation(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Begin Objective", Category = "Objective")
	void K2_BeginObjective(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Tick Objective", Category = "Objective")
	void TickObjective();
	virtual void TickObjective_Implementation();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Objective Completed", Category = "Objective")
	void K2_ObjectiveCompleted(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Objective Failed", Category = "Objective")
	void K2_ObjectiveFailed(APlayerController* OwnerController, APawn* ControlledPawn);
	
	/**
	 * Implement to provide the quest with the location of your objective, Useful if you wanted to use quest indicator.
	 * Primitive component is optional, but will help in some case where the quest indicator need to keep tracking the actor location.
	 * If there is no input on the component, will use ObjectiveLocation Instead.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Objective", meta = (DisplayName = "Get Objective Location"))
	TArray<USceneComponent*> K2_GetObjectiveLocation(TArray<FVector>& ObjectiveLocation) const;

	UFUNCTION(Category = "Objective")
	virtual void BeginObjective();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	virtual void CompleteObjective();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	virtual void FailObjective();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	virtual void FinishSetup();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	virtual void DeactivateTick();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Objective")
	bool IsObjectiveCompleted();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Objective")
	bool IsObjectiveFailed();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Objective")
	bool IsObjectiveActive();

	/**Add amount and check if the required amount has fulfilled, then complete the objective*/
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void AddToProgress(int AmountToAdd = 1);

	virtual void Initialize(bool bLaunchEventOnLoad = false) override;
	virtual void Deinitialize() override;
	virtual void Reset() override;


	virtual void BeginNode() override;
	virtual void EvaluateNextNode() override;
	virtual bool HasBeenVisited() override;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle);
	virtual FText GetNodeDescription() const override;
#endif
};
