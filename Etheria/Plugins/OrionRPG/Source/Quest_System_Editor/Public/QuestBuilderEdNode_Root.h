// Copyright 2025 Ivan Chandra. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderEdNode.h"
#include "EdGraph/EdGraphNode.h"
#include "QuestBuilderEdNode_Root.generated.h"

/**
 * 
 */
UCLASS()
class QUEST_SYSTEM_EDITOR_API UQuestBuilderEdNode_Root : public UQuestBuilderEdNode
{
	GENERATED_BODY()

public:
	UQuestBuilderEdNode_Root();
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual bool CanDuplicateNode() const override { return false; }
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetBackgroundColor() const override;
};
