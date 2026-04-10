// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogBuilderFunctionLibrary.generated.h"

class UDialogComponent;

UCLASS()
class ORIONRPG_API UDialogBuilderFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	* Grab the Dialog component from the local pawn or player controller, whichever it exists on.
	*
	* @return The Dialog component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static class UDialogComponent* GetDialogComponent(const UObject* WorldContextObject);

	/**
	* Find the Dialog component from the supplied target object.
	*
	* @return The Dialog component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Dialog", meta = (DefaultToSelf = "Target"))
	static class UDialogComponent* GetDialogComponentFromTarget(AActor* Target);

	/**
	* Get the currently active dialog.
	* 
	* @return The Dialog Graph reference.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static class UDialogBuilderGraph* GetCurrentActiveDialog(const UObject* WorldContextObject);

	/**
	* Get the current dialog node.
	*
	* @return The Dialog Node reference.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static class UDialogBuilderNode* GetCurrentDialogNode(const UObject* WorldContextObject);

	/**
	* Get the current dialog line node.
	*
	* @return The Dialog Node reference.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static class UDialogBuilderNode_DialogLine* GetCurrentLine(const UObject* WorldContextObject);


	/**
	* Advance the dialog line
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static void AdvanceDialogLine(const UObject* WorldContextObject);

	/**
	* Select Dialog Option
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static void SelectDialogChoice(const UObject* WorldContextObject, class UDialogBuilderNode_PlayerChoice* InOption);


	/**
	* Begin Dialog Graph
	*
	*/
	UFUNCTION(BlueprintCallable, Category = "OrionLibrary|Dialog", meta = (WorldContext = "WorldContextObject"))
	static void BeginDialog(const UObject* WorldContextObject, class UDialogBuilderGraph* DialogAsset);
};
