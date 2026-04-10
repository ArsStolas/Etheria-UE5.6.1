// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/NoExportTypes.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderEdNode_Edge.generated.h"


class UQuestBuilderNode;
class UQuestBuilderEdge;
class UQuestBuilderEdNode;
class UEdGraph;

UCLASS()
class QUEST_SYSTEM_EDITOR_API UQuestBuilderEdNode_Edge : public UEdGraphNode
{
	GENERATED_BODY()
public:
	UQuestBuilderEdNode_Edge();

	UPROPERTY()
		TObjectPtr<UEdGraph> Graph;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "QuestBuilderGraph")
		TObjectPtr<UQuestBuilderEdge> QuestSystemGraphEdge;

	void SetEdge(UQuestBuilderEdge* Edge);

	virtual void AllocateDefaultPins() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

	virtual void PrepareForCopying() override;

	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }

	void CreateConnections(UQuestBuilderEdNode* Start, UQuestBuilderEdNode* End);

	UQuestBuilderEdNode* GetStartNode();
	UQuestBuilderEdNode* GetEndNode();
};
