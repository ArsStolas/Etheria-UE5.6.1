// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "CompleteObjective.generated.h"

/**
 * Complete quest from quest tag.
 * Can only complete currently active quests.
 */
UCLASS()
class ORIONRPG_API UCompleteObjective : public UOrionEvent
{
	GENERATED_BODY()
public:
	UCompleteObjective();

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};

