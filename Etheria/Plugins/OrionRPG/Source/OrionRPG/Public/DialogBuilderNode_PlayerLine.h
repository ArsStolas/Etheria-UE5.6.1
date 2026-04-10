// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogData.h"
#include "DialogBuilderNode_PlayerLine.generated.h"


UCLASS(BlueprintType)
class ORIONRPG_API UDialogBuilderNode_PlayerLine : public UDialogBuilderNode_DialogLine
{
    GENERATED_BODY()
public:
    UDialogBuilderNode_PlayerLine();
    virtual void BeginNode() override;

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual void SetNodeTitle(const FText& NewTitle);
    virtual FText GetNodeDescription() const override;

#endif
};
