/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIController_Base - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AIController_Base.generated.h"

UCLASS()
class ETHERIA_API AAIController_Base : public AAIController
{
	GENERATED_BODY()

public:
	AAIController_Base();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UAIPerceptionComponent* PerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	AActor* TargetActor = nullptr;
};
