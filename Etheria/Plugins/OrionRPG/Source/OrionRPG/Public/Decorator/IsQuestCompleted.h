// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Decorator/OrionDecorator.h"
#include "IsQuestCompleted.generated.h"


class UQuest;
/**
 * Evaluate if quest completed.
 */
UCLASS()
class ORIONRPG_API UIsQuestCompleted : public UOrionDecorator
{
	GENERATED_BODY()

public:
	UIsQuestCompleted();


	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
