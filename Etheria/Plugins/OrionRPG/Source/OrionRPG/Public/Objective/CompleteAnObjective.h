// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderNode_Objective.h"
#include "CompleteAnObjective.generated.h"


UCLASS()
class ORIONRPG_API UCompleteAnObjective : public UQuestBuilderNode_Objective
{
	GENERATED_BODY()
public:
	UCompleteAnObjective();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (DisplayName = "Objective Tag To Complete", Categories = "Quest"))
	FGameplayTag ObjectiveTag;
	virtual void BeginObjective() override;
	virtual void CompleteObjective() override;


	UFUNCTION()
	virtual void EvaluateObjectiveCompleted(UQuest* InQuest);

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};
