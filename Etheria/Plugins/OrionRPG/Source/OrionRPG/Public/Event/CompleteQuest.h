// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "CompleteQuest.generated.h"

/**
 * Complete quest from quest tag.
 * Can only complete currently active quests.
 */
UCLASS()
class ORIONRPG_API UCompleteQuest : public UOrionEvent
{
	GENERATED_BODY()
public:
	UCompleteQuest();

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};

