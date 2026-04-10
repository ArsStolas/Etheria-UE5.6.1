// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OrionSaveGame.h"
#include "Components/ActorComponent.h"
#include "QuestComponent.generated.h"

struct FQuestCategory;
struct FOrionQuestSaveData;
class UOrionSaveGame;
class UQuestBuilderNode_Objective;
class UQuest;
class USaveGame;
class APlayerController;

//Quest Delegate

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuestUpdated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStopNavigateQuest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestObjectiveBegin, UQuestBuilderNode_Objective*, Objective);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestObjectiveUpdated, UQuest*, Quest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartNavigateQuest, UQuest*, Quest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestFailed, UQuest*, Quest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestCompleted, UQuest*, Quest);
	
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestAdded, UQuest*, Quest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestUnlocked, UQuest*, Quest);


UCLASS(meta=(BlueprintSpawnableComponent), Blueprintable, HideDropdown, ClassGroup = "OrionRPG", DisplayName = "Orion Quest Component")
class ORIONRPG_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UQuestComponent();

	//We cache the OwningController, we won't cache pawn as this might change
	UPROPERTY(BlueprintReadOnly, Category = "QuestComponent")
		TObjectPtr<APlayerController> OwningController;

	/** Map of Quest ID and attached Quest That is Running*/
	UPROPERTY(BlueprintReadWrite, Category = "QuestComponent")
		TMap<FName, TObjectPtr<UQuest>> QuestMap;
	
	/**Map of Quest from asset registry. this quest are from asset, DONT manipulate the property unless you know that you're doing*/
	UPROPERTY(BlueprintReadOnly, Category = "QuestComponent")
	TMap<FName, TObjectPtr<UQuest>> QuestRegistryMap;

	/** List of quest category*/
	UPROPERTY(BlueprintReadWrite, Category = "QuestComponent")
		TArray<FQuestCategory> QuestCategories;

	UPROPERTY(BlueprintReadWrite, Category = "QuestComponent")
		TObjectPtr<UQuest> CurrentNavigatedQuest;

	UPROPERTY()
		FOrionQuestSaveData CurrentQuestSaveData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  Category = "QuestComponent")
		bool bAutoNavigateQuest;

	/**
	* Default quest to be added when the game start.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QuestComponent")
		TArray<TObjectPtr<UQuestBuilderGraph>> DefaultQuestGraphs;
public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
	FQuestUpdated OnQuestUpdated;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestObjectiveBegin OnQuestObjectiveBegin;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestObjectiveUpdated OnQuestObjectiveUpdated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FStartNavigateQuest OnStartNavigateQuest;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestManager")
		FStopNavigateQuest OnStopNavigateQuest;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestFailed OnQuestFailed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestCompleted OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestAdded OnQuestAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "QuestComponent")
		FQuestUnlocked OnQuestUnlocked;

private:
	

	TMap<FName, TObjectPtr<UQuest>> GetQuestRegistryMap();

public:
	UFUNCTION()
	virtual void QuestUpdated();
	
	UFUNCTION()
		virtual void QuestObjectiveBegin(class UQuestBuilderNode_Objective* InObjective);

	UFUNCTION()
		virtual void QuestObjectiveUpdated(UQuest* Quest);

	UFUNCTION()
		virtual void StartNavigatingQuest(UQuest* Quest);

	UFUNCTION()
		virtual void QuestFailed(UQuest* Quest);

	UFUNCTION()
		virtual void QuestCompleted(UQuest* Quest);

	UFUNCTION()
		virtual void QuestAdded(UQuest* Quest);

	UFUNCTION()
		virtual void QuestUnlocked(UQuest* Quest);
	
public:
	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		bool BeginQuestGraph(UQuestBuilderGraph* QuestGraphAsset);
	
	UFUNCTION()
		virtual class UQuest* MakeQuestInstance(UQuest* QuestTemplate);
	
	UFUNCTION()
	virtual class UQuest* TryMakeQuestInstanceFromTag(FGameplayTag QuestTag);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void AutoNavigateQuest();

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void UpdateAllQuest();
	
	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void ActivateQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag, const bool bNotifyQuestAdded = true);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void UnlockQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void LockQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);
		
	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void RestartQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag, UPARAM(meta = (Categories = "Quest"))FGameplayTag NodeTag, const bool bNotifyQuestAdded = false);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void FailQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void CompleteQuestFromTag(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);

	/**
	* Start navigate the selected quest. (Objective input is optional)
	* if the objective is none, auto navigate the first objective found.
	*/
	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void StartNavigateQuest(UQuest* InQuest, class UQuestBuilderNode_Objective* InObjective = nullptr, bool bOverrideCurrentNavigatedQuest = false);

	UFUNCTION(BlueprintCallable, Category = "QuestManager")
		void StopNavigateQuest();

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void ActivateQuest(UQuest* InQuest, const bool bNotifyQuestAdded = true, const bool bLaunchEventOnLoad = false);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void UnlockQuest(UQuest* InQuest);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void LockQuest(UQuest* InQuest);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void RestartQuest(UQuest* InQuest, UQuestBuilderNode* InNode = nullptr, const bool bNotifyQuestAdded = false);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void FailQuest(UQuest* InQuest);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void CompleteQuest(UQuest* InQuest);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		bool IsQuestAtState(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag, enum EQuestState QuestState);

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		UQuest* FindQuest(UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag) const;

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void ResetAllQuest();

	UFUNCTION(BlueprintCallable, Category = "QuestComponent")
		void EvaluateQuestState(UQuest* QuestParam);

	UFUNCTION()
	FOrionQuestSaveData GetSaveGameData();

	UFUNCTION()
	void InitializeFromSaveGame(FOrionQuestSaveData& QuestSaveData);

	UFUNCTION(BlueprintCallable, Category = "Saving")
	virtual bool DeleteSave(const FString& SaveName = "QuestBuilderSaveData", const int32 Slot = 0);

public:
	UFUNCTION(BlueprintPure, Category = "QuestComponent")
	virtual APawn* GetOwningPawn() const;

	UFUNCTION(BlueprintPure, Category = "QuestComponent")
	virtual APlayerController* GetOwningController() const;

	void InitializeDelegate();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// ================================================================================================ //
	// CONSOLE COMMANDS
	// ================================================================================================ //
public:
	UFUNCTION(Exec)
	void execDumpQuestLog();

	UFUNCTION(Exec)
	void execCompleteQuestObjectives(const TArray<FString>& QuestTag);

	UFUNCTION(Exec)
	void execActivateQuest(const TArray<FString>& QuestTag);

	UFUNCTION(Exec)
	void execCompleteQuest(const TArray<FString>& QuestTag);

	UFUNCTION(Exec)
	void execFailQuest(const TArray<FString>& QuestTag);
};
