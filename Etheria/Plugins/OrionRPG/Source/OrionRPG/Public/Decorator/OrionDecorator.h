// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "OrionDecorator.generated.h"

class UQuestBuilderGraph;
class UQuestBuilderNode;

UCLASS(Abstract, Blueprintable, EditInlineNew, HideDropdown)
class ORIONRPG_API UOrionDecorator : public UObject
{
	GENERATED_BODY()
public:
	UOrionDecorator();
	
	/** node name override*/
	UPROPERTY(Category = "Node", EditAnywhere, meta = (EditCondition = "bIsNode == true", HideEditConditionToggle, EditConditionHides))
	FString NodeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrionDecorator")
		bool InvertCondition = false;

	/**Display quest Tag in property panel?*/
	UPROPERTY(BlueprintReadWrite, Category = "OrionDecorator")
		bool bUseQuestTag = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "OrionDecorator")
	bool bIsNode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrionDecorator", meta = (EditCondition = "bUseQuestTag == true", HideEditConditionToggle, EditConditionHides, Categories = "Quest"))
		FGameplayTag QuestTag;

	/** Display Node ID in property panel?*/
	UPROPERTY(BlueprintReadWrite, Category = "OrionDecorator")
		bool bUseNodeTag = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrionDecorator", meta = (EditCondition = "bUseNodeTag == true", HideEditConditionToggle, EditConditionHides, Categories = "Quest"))
		FGameplayTag NodeTag;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "OrionDecorator")
		bool PerformConditionCheck(APlayerController* OwnerController, APawn* ControlledPawn) const;

		virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "OrionDecorator")
	FString GetNodeDisplayText() const;

	virtual FString GetNodeDisplayText_Implementation() const;

	UFUNCTION(BlueprintCallable, Category = "OrionDecorator")
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
