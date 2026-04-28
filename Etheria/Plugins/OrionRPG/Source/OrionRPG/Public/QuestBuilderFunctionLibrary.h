// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Quest.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "QuestBuilderFunctionLibrary.generated.h"

class UQuestComponent;

UCLASS()
class ORIONRPG_API UQuestBuilderFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	* Grab the Quest component from the local pawn or player controller, whichever it exists on.
	*
	* @return The Quest component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static class UQuestComponent* GetQuestComponent(const UObject* WorldContextObject);

	/**
	* Find the Quest component from the supplied target object.
	*
	* @return The Quest component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (DefaultToSelf = "Target"))
	static class UQuestComponent* GetQuestComponentFromTarget(AActor* Target);

	/**
	* Find the Quest from the given Quest ID
	*
	* @return The Quest.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static class UQuest* FindQuest(const UObject* WorldContextObject, FGameplayTag QuestTag);

	/**
	* Check if the current objective valid from QuestID and NodeID.
	*
	* @return true if the quest id and node id match.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static bool CanProgressQuestObjective(const UObject* WorldContextObject, FGameplayTag QuestTag, FGameplayTag NodeTag);

	/**
	* Check if the quest at which state from the given Quest ID
	*
	* @return bool.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static bool IsQuestAtState(const UObject* WorldContextObject, FGameplayTag QuestTag, EQuestState QuestState);

	/**
	* Get the currently navigated quest
	*
	* @return The Quest.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static class UQuest* GetCurrentNavigatedQuest(const UObject* WorldContextObject);
	
	/**
	* Evaluate if the current objective ID is being navigated.
	*
	* @return bool.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static bool IsCurrentlyNavigatedObjective(const UObject* WorldContextObject, FName QuestID, FName NodeID);
	
	/**
	* Evaluate if the node objective has completed.
	*
	* @return bool.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static bool IsObjectiveCompleted(const UObject* WorldContextObject, UQuestBuilderNode* QuestNode);


	/**
	* Navigate Quest
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void NavigateQuest(const UObject* WorldContextObject, UQuest* InQuest, bool bOverrideNavigatedQuest);

	/**
	* Auto Navigate Quest
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void AutoNavigateQuest(const UObject* WorldContextObject);

	/**
	* Begin Quest Graph
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void BeginQuestGraph(const UObject* WorldContextObject, class UQuestBuilderGraph* QuestAsset);

	/**
	* Activate Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void ActivateQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);

	/**
	* Lock Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void LockQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);

	/**
	* Unlock Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void UnlockQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);


	/**
	* Complete Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void CompleteQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);


	/**
	* Fail Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void FailQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag);


	/**
	* Restart Quest from Tag
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static void RestartQuestFromTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "Quest"))FGameplayTag QuestTag, UPARAM(meta = (Categories = "Quest"))FGameplayTag NodeTag, const bool bNotifyQuestAdded);

	/**
	* Check if there is Any Active Quest
	*
	*/
	UFUNCTION(BlueprintPure, Category = "OrionLibrary|Quest", meta = (WorldContext = "WorldContextObject"))
	static bool IsAnyQuestActive(const UObject* WorldContextObject);

};


