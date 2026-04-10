// Copyright 2025 Ivan Chandra. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderEdSubNode.h"
#include "EdGraph/EdGraphNode.h"
#include "QuestBuilderEdSubNode_Event.generated.h"

/**
 * 
 */
UCLASS()
class QUEST_SYSTEM_EDITOR_API UQuestBuilderEdSubNode_Event : public UQuestBuilderEdSubNode
{
	GENERATED_BODY()

public:
	UQuestBuilderEdSubNode_Event();
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const override;
	virtual FText GetDescription() const override;
};
