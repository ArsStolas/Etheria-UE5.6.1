// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_PlayerChoice.generated.h"


UCLASS(BlueprintType)
class ORIONRPG_API UDialogBuilderNode_PlayerChoice : public UDialogBuilderNode
{
	GENERATED_BODY()            
public:
    UDialogBuilderNode_PlayerChoice();
    virtual void BeginNode() override;


    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Detail")
    FText ChoiceText;
#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual void SetNodeTitle(const FText& NewTitle);
    virtual FText GetNodeDescription() const override;
    
#endif
};
