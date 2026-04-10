// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Decorator/OrionDecorator.h"
#include "IsQuestObjectiveCompleted.generated.h"

/**
 * Evaluate if objective node completed from tag.
 */
UCLASS()
class ORIONRPG_API UIsQuestObjectiveCompleted : public UOrionDecorator
{
	GENERATED_BODY()
public:
	UIsQuestObjectiveCompleted();


	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
