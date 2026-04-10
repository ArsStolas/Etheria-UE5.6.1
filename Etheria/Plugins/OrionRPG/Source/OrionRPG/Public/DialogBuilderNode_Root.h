// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "UObject/NoExportTypes.h"
#include "DialogBuilderNode_Root.generated.h"



UCLASS(HideCategories = ("Events", BranchingConditions, "Detail"))
class ORIONRPG_API UDialogBuilderNode_Root : public UDialogBuilderNode
{
	GENERATED_BODY()

public:
	virtual void BeginNode() override;

#if WITH_EDITOR
	virtual FText GetNodeDescription() const override;
#endif
	
};
