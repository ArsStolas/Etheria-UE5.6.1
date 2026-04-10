// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "BeginQuestGraph.generated.h"

/**
 * Begin the quest graph asset.
 */
UCLASS()
class ORIONRPG_API UBeginQuestGraph : public UOrionEvent
{
	GENERATED_BODY()
public:
	UBeginQuestGraph();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestEvent")
	TObjectPtr<UQuestBuilderGraph> QuestAsset;

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
