// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestData.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "OrionEvent.generated.h"

class APlayerController;
class APawn;
class UQuestBuilderGraph;
class UQuestBuilderNode;
class UQuest;


UENUM(BlueprintType)
enum class EEventLaunchType : uint8
{
	/** will trigger at the beggining of the node */
	E_Start			UMETA(DisplayName = "Start of node"),
	/** will trigger if next node conditions met || at the end of state node */
	E_End			UMETA(DisplayName = "End of node"),
	/** will trigger twice at start and end of node */
	E_Both			UMETA(DisplayName = "Both"),
};

DECLARE_MULTICAST_DELEGATE(FEventFinishedSignature);

UCLASS(Abstract, Blueprintable, EditInlineNew, HideDropdown, AutoExpandCategories = ("Default"))
class ORIONRPG_API UOrionEvent : public UObject
{
	GENERATED_BODY()
public:
	UOrionEvent();
	
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> OwningController;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APawn> OwningPawn;


	/** node name override*/
	UPROPERTY(Category = "Node", EditAnywhere, meta = (EditCondition = "bIsNode == true", HideEditConditionToggle, EditConditionHides))
	FString NodeName;

	/**Display quest Tag in property panel?*/
	UPROPERTY(BlueprintReadWrite, Category = "Event")
		bool bUseQuestTag = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "bUseQuestTag == true", HideEditConditionToggle, EditConditionHides, Categories = "Quest"))
		FGameplayTag QuestTag;

	/** Display Node ID in property panel?*/
	UPROPERTY(BlueprintReadWrite, Category = "Event")
		bool bUseNodeTag = false;

	UPROPERTY(BlueprintReadWrite, Category = "Event")
	bool bIsNode = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "bUseNodeTag == true", HideEditConditionToggle, EditConditionHides, Categories = "Quest"))
		FGameplayTag NodeTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Node", meta = (EditCondition = "bIsNode == true", HideEditConditionToggle, EditConditionHides))
		EEventLaunchType EventLaunchType = EEventLaunchType::E_Start;

	/**Launch this event for the first time quest loaded*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Node", DisplayName = "Launch Event On Load", meta = (EditCondition = "bIsNode == true", HideEditConditionToggle, EditConditionHides))
		bool bLaunchEventOnLoad = false;
		
public:
	/**Called when event finished*/
	FEventFinishedSignature OnEventFinished;

	bool bSetupCompleted;
	bool bPendingToStart;

public:
	UFUNCTION(BlueprintCallable, Category = "Event")
	void BeginSetup(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Begin Setup", Category = "Event")
	void K2_BeginSetup(APlayerController* OwnerController, APawn* ControlledPawn);

	void K2_BeginSetup_Implementation(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Begin Event", Category = "Event")
		void K2_BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(Category = "Event")
	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn);

	UFUNCTION(BlueprintCallable, Category = "Event")
	virtual void FinishSetup();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Event Finished", Category = "Event")
	void K2_EventFinished();

	UFUNCTION(BlueprintCallable, Category = "Event")
	virtual void EndEvent();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	FString GetNodeDisplayText() const;
	virtual FString GetNodeDisplayText_Implementation() const;


	UFUNCTION(BlueprintCallable, Category = "Event")
		FString GetShortTag(FGameplayTag Tag, int32 Level = 1) const;

	/** @return name of node */
	FString GetNodeName();
	FString GetShortTypeName(UObject* Ob);


	// Allows the Object to get a valid UWorld from it's outer.
	virtual UWorld* GetWorld() const override;

	
#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
