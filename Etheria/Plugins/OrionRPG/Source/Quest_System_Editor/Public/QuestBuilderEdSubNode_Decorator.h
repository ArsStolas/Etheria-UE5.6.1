// Copyright 2025 Ivan Chandra. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderEdSubNode.h"
#include "EdGraph/EdGraphNode.h"
#include "QuestBuilderEdSubNode_Decorator.generated.h"

/**
 * 
 */
UCLASS()
class QUEST_SYSTEM_EDITOR_API UQuestBuilderEdSubNode_Decorator : public UQuestBuilderEdSubNode
{
	GENERATED_BODY()

public:
	UQuestBuilderEdSubNode_Decorator();
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const override;
	virtual FText GetDescription() const override;
};
