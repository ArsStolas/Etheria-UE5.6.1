// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Quest.h"
#include "Decorator/OrionDecorator.h"
#include "IsQuestAtState.generated.h"


/**
 * Evaluate if quest is at certain state.
 */
UCLASS()
class ORIONRPG_API UIsQuestAtState : public UOrionDecorator
{
	GENERATED_BODY()

public:
	UIsQuestAtState();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "OrionDecorator")
	EQuestState QuestState = EQuestState::E_Active;
	
	virtual bool PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
