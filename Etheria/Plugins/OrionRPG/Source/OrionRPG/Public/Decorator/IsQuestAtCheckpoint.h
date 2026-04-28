// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Decorator/OrionDecorator.h"
#include "IsQuestAtCheckpoint.generated.h"

/**
 * Evaluate if quest is in latest checkpoint node.
 */
UCLASS()
class ORIONRPG_API UIsQuestAtCheckpoint : public UOrionDecorator
{
	GENERATED_BODY()
public:
	UIsQuestAtCheckpoint();


	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
