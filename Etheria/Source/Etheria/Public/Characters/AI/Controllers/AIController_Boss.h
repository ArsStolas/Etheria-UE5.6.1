/*
* Etheria's End Project, 2025
* AIController_Boss - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/Controllers/AIController_Base.h"
#include "AIController_Boss.generated.h"

class UBehaviorTree;

/** Controller du boss : lance le Behavior Tree en OnPossess, ne gère pas la chase par perception. */
UCLASS()
class ETHERIA_API AAIController_Boss : public AAIController_Base
{
	GENERATED_BODY()

public:
	AAIController_Boss();

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AI")
	TObjectPtr<UBehaviorTree> BossBehaviorTreeAsset;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus) override;
};
