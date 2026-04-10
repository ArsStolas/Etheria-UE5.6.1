// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogBuilderEdge.generated.h"


class UDialogBuilderGraph;
class UDialogBuilderNode;

UCLASS(Blueprintable)
class ORIONRPG_API UDialogBuilderEdge : public UObject
{
	GENERATED_BODY()
public:
	UDialogBuilderEdge();
	virtual ~UDialogBuilderEdge();

	UPROPERTY(VisibleAnywhere, Category = "DialogBuilderNode")
		TObjectPtr<UDialogBuilderGraph> Graph;

	UPROPERTY(BlueprintReadOnly, Category = "DialogBuilderEdge")
		TObjectPtr<UDialogBuilderNode> StartNode;

	UPROPERTY(BlueprintReadOnly, Category = "DialogBuilderEdge")
		TObjectPtr<UDialogBuilderNode> EndNode;

	UFUNCTION(BlueprintPure, Category = "DialogBuilderEdge")
		UDialogBuilderGraph* GetGraph() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, Category = "DialogBuilderNode_Editor")
		bool bShouldDrawTitle = false;

	UPROPERTY(EditDefaultsOnly, Category = "DialogBuilderNode_Editor")
		FText NodeTitle;

	UPROPERTY(EditDefaultsOnly, Category = "DialogBuilderEdge")
		FLinearColor EdgeColour = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
#endif

#if WITH_EDITOR
	virtual FText GetNodeTitle() const { return NodeTitle; }
	FLinearColor GetEdgeColour() { return EdgeColour; }

	virtual void SetNodeTitle(const FText& NewTitle);
#endif
};
