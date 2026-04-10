// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode_Objective.h"
#include "CompleteAQuest.generated.h"

UCLASS()
class ORIONRPG_API UCompleteAQuest : public UQuestBuilderNode_Objective
{
	GENERATED_BODY()
public:
	UCompleteAQuest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (DisplayName = "Quest Tag To Complete", Categories = "Quest"))
	FGameplayTag QuestTag;

	virtual void BeginObjective() override;
	virtual void CompleteObjective() override;


	UFUNCTION()
	virtual void EvaluateQuestCompleted(UQuest* InQuest);

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};
