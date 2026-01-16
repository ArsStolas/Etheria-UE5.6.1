/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIWanderComponent - Header
*/
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIWanderComponent.generated.h"

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAIWanderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void StartWander();
	void StopWander();

	FTimerHandle WanderTimer;


protected:
	virtual void BeginPlay() override;
	FVector GetRandomPointInRadius();

protected:
	UPROPERTY(EditAnywhere, Category="Wander") float WanderRadius = 800.f;
	UPROPERTY(EditAnywhere, Category="Wander") float WaitTime = 2.f;

private:
};
