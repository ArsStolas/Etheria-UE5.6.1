// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuestBuilderEdge.generated.h"


class UQuestBuilderGraph;
class UQuestBuilderNode;

UCLASS(Blueprintable)
class ORIONRPG_API UQuestBuilderEdge : public UObject
{
	GENERATED_BODY()
public:
	UQuestBuilderEdge();
	virtual ~UQuestBuilderEdge();

	UPROPERTY(VisibleAnywhere, Category = "QuestBuilderNode")
		TObjectPtr<UQuestBuilderGraph> Graph;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderEdge")
		TObjectPtr<UQuestBuilderNode> StartNode;

	UPROPERTY(BlueprintReadOnly, Category = "QuestBuilderEdge")
		TObjectPtr<UQuestBuilderNode> EndNode;

	UFUNCTION(BlueprintPure, Category = "QuestBuilderEdge")
		UQuestBuilderGraph* GetGraph() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, Category = "QuestBuilderNode_Editor")
		bool bShouldDrawTitle = false;

	UPROPERTY(EditDefaultsOnly, Category = "QuestBuilderNode_Editor")
		FText NodeTitle;

	UPROPERTY(EditDefaultsOnly, Category = "QuestBuilderEdge")
		FLinearColor EdgeColour = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
#endif

#if WITH_EDITOR
	virtual FText GetNodeTitle() const { return NodeTitle; }
	FLinearColor GetEdgeColour() { return EdgeColour; }

	virtual void SetNodeTitle(const FText& NewTitle);
#endif
};
