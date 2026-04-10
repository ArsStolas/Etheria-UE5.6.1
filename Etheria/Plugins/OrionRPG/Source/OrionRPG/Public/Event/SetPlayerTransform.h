// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "SetPlayerTransform.generated.h"

/**
 * Set player transform based on location vector / player start
 */
UCLASS()
class ORIONRPG_API USetPlayerTransform : public UOrionEvent
{
	GENERATED_BODY()
public:
	USetPlayerTransform();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bUsePlayerStart;

	// player transform to set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "bUsePlayerStart == false", HideEditConditionToggle, EditConditionHides))
	FTransform Transform;

	// player start tag to use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "bUsePlayerStart == true", HideEditConditionToggle, EditConditionHides))
	FString PlayerStartTag;

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
