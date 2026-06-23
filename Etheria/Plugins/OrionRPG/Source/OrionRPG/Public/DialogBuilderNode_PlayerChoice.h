// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderNode_PlayerChoice.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnPlayerChoiceDataChanged);

USTRUCT()
struct FDialogChoiceBranch
{
	GENERATED_BODY()

	UPROPERTY()
	FText ChoiceText;

	UPROPERTY()
	TArray<TObjectPtr<UDialogBuilderNode>> ChildrenNodes;
};

UCLASS(BlueprintType)
class ORIONRPG_API UDialogBuilderNode_PlayerChoice : public UDialogBuilderNode_DialogSequence
{
	GENERATED_BODY()
public:
	UDialogBuilderNode_PlayerChoice();
	virtual void BeginNode() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice Selection", meta = (DisplayPriority = -1))
	EDialogSelectionType SelectionType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Choice Selection", meta = (EditCondition = "SelectionType == EDialogSelectionType::E_CameraShot", HideEditConditionToggle, EditConditionHides, DisplayPriority = -1))
	TObjectPtr<UDialogCameraShot> SelectingChoiceShotOverride;


	/**
	* Entering selection mode will have it's time limit setting.
	* Selection will have no time limit by default
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice Selection", meta = (DisplayPriority = -1))
	ESelectionTimeLimit SelectionTimeLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice Selection", meta = (EditCondition = "SelectionTimeLimit == ESelectionTimeLimit::E_HasTimeLimit", HideEditConditionToggle, EditConditionHides, DisplayPriority = -1))
	float TimeLimit;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Detail")
	TArray<FText> ChoiceList;

	UPROPERTY()
	TArray<FDialogChoiceBranch> ChoiceBranches;


	FOnPlayerChoiceDataChanged OnPlayerChoiceDataChanged;

public:
	UFUNCTION(BlueprintCallable, Category=Dialog)
	void SelectChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintPure, Category = Dialog)
	bool IsChoiceVisible(int32 ChoiceIndex);




	virtual void DetermineNextNode() override;

private:
	int32 SelectedChoiceIndex = -1;

#if WITH_EDITOR
public:
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle);
	virtual FText GetNodeDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
};
