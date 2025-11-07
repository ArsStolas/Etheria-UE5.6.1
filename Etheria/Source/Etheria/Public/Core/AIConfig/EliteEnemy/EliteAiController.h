/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: EliteAIController - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EliteAiController.generated.h"

UCLASS()
class ETHERIA_API AEliteAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	UBehaviorTree* BehaviorTreeAsset;

	UBlackboardComponent* BlackboardComponent;
};