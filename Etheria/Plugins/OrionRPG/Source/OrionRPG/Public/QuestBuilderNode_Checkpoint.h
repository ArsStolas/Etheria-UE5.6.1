// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode.h"
#include "Quest.h"
#include "QuestBuilderNode_Checkpoint.generated.h"


UCLASS()
class ORIONRPG_API UQuestBuilderNode_Checkpoint : public UQuestBuilderNode
{
	GENERATED_BODY()
public:

    virtual void Initialize(bool bLaunchEventOnLoad = false) override;
	virtual void BeginNode() override;

    UFUNCTION(BlueprintCallable, Category = "CheckpointNode")
        void BeginCheckpoint();

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual void SetNodeTitle(const FText& NewTitle);
    virtual FText GetNodeDescription() const override;

    virtual FLinearColor GetBackgroundColor() const override;
#endif
};
