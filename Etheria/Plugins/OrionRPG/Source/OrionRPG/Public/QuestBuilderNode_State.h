// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode.h"
#include "Quest.h"
#include "QuestBuilderNode_State.generated.h"


UCLASS(/*HideCategories = ("Detail")*/)
class ORIONRPG_API UQuestBuilderNode_State : public UQuestBuilderNode
{
	GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, Category = "State")
        EQuestState QuestState;

    virtual void Initialize(bool bLaunchEventOnLoad = false) override;
	virtual void BeginNode() override;

    UFUNCTION(BlueprintCallable, Category = "State")
        void BeginState();

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual void SetNodeTitle(const FText& NewTitle);
    virtual FText GetNodeDescription() const override;

    virtual FLinearColor GetBackgroundColor() const override;
#endif
};
