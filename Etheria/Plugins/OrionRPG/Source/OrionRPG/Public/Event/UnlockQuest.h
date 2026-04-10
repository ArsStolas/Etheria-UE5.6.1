// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "UnlockQuest.generated.h"

/**
 * Unlock quest from quest tag.
 * Can only unlock locked quests.
 */
UCLASS()
class ORIONRPG_API UUnlockQuest : public UOrionEvent
{
	GENERATED_BODY()
public:
	UUnlockQuest();

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
