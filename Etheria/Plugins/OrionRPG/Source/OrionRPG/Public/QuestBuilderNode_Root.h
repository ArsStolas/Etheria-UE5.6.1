// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode.h"
#include "UObject/NoExportTypes.h"
#include "QuestBuilderNode_Root.generated.h"



UCLASS(HideCategories = ("Events", BranchingConditions, "Detail"))
class ORIONRPG_API UQuestBuilderNode_Root : public UQuestBuilderNode
{
	GENERATED_BODY()

public:
	virtual void BeginNode() override;

#if WITH_EDITOR
	virtual FText GetNodeDescription() const override;
#endif
	
};
