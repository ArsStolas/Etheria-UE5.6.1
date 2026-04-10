// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Decorator/OrionDecorator.h"
#include "HasQuestEverReachedCheckpoint.generated.h"

/**
 * Evaluate if quest has ever reached a certain checkpoint node.
 */
UCLASS()
class ORIONRPG_API UHasQuestEverReachedCheckpoint : public UOrionDecorator
{
	GENERATED_BODY()
public:
	UHasQuestEverReachedCheckpoint();


	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
