// Copyright 2025 Ivan Chandra. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEdSubNode.h"
#include "EdGraph/EdGraphNode.h"
#include "DialogBuilderEdSubNode_Decorator.generated.h"

/**
 * 
 */
UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdSubNode_Decorator : public UDialogBuilderEdSubNode
{
	GENERATED_BODY()

public:
	UDialogBuilderEdSubNode_Decorator();
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const override;
	virtual FText GetDescription() const override;
};
