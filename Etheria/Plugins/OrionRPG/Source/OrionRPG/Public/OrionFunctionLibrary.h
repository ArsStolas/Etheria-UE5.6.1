// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "OrionFunctionLibrary.generated.h"


UCLASS()
class ORIONRPG_API UOrionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	* Save actor data to current save slot
	*
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Save", meta = (WorldContext = "WorldContextObject"))
	static class UOrionSaveGame* GetCurrentSaveGameObject(const UObject* WorldContextObject);


	/**
	* Save actor data to current save slot
	*
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Save", meta = (WorldContext = "WorldContextObject"))
	static class UOrionSaveGame* GetSaveGameObject(const UObject* WorldContextObject, int slot = 0);

	/**
	* Save actor data to current save slot
	* 
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveActorData(const UObject* WorldContextObject, AActor* ActorToSave);

	/**
	* Remove specific actor data to current save slot
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Save", meta = (WorldContext = "WorldContextObject"))
	static void RemoveSavedActorData(const UObject* WorldContextObject, AActor* InActor);

	/**
	* Save actor data to specific save slot
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveActorDataToSlot(const UObject* WorldContextObject, int slot = 0, AActor* ActorToSave = nullptr);

};


