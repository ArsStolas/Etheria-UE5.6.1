// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "RestartQuest.generated.h"


/**
 * Restart quest from quest tag.
 */


UCLASS()
class ORIONRPG_API URestartQuest : public UOrionEvent
{
	GENERATED_BODY()
public:
	URestartQuest();

	
	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;

};
