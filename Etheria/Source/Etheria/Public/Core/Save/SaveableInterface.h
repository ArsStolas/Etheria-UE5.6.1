/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "SaveableInterface - Header"
 * Notes: Implement this on any Actor or Component that needs to react to save/load.
 *        OnSaveRequested fires before SaveProgress writes to disk — write your data here using
 *        Set Save X nodes from the SaveSubsystem.
 *        OnLoadCompleted fires after LoadProgress reads from disk — read your data here using
 *        Get Save X nodes.
 *        GetSaveID gives a stable unique key so multiple instances don't collide.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveableInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, meta=(DisplayName="Saveable"))
class USaveable : public UInterface
{
	GENERATED_BODY()
};

class ETHERIA_API ISaveable
{
	GENERATED_BODY()

public:
	/** Called by the SaveSubsystem just before saving. Write your data here using Set Save X nodes. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Etheria|Save")
	void OnSaveRequested();
	virtual void OnSaveRequested_Implementation() {}

	/** Called by the SaveSubsystem just after loading. Read your data here using Get Save X nodes. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Etheria|Save")
	void OnLoadCompleted();
	virtual void OnLoadCompleted_Implementation() {}

	/** Stable unique identifier so multiple instances don't overwrite each other (e.g. "Campfire_Forest_01").
	 *  Default returns the actor's FName. Override in BP for human-readable IDs. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Etheria|Save")
	FName GetSaveID() const;
	virtual FName GetSaveID_Implementation() const { return NAME_None; }
};
