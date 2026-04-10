// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Decorator/OrionDecorator.h"
#include "IsQuestFailed.generated.h"

/**
 * Evaluate if quest failed.
 */
UCLASS()
class ORIONRPG_API UIsQuestFailed : public UOrionDecorator
{
	GENERATED_BODY()

public:
	UIsQuestFailed();


	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
