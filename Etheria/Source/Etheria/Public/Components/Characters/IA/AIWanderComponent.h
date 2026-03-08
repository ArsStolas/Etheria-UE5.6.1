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
	/** Appelé par le controller quand la destination wander est atteinte : pause WaitTime puis prochain point. */
	void OnDestinationReached();

	FTimerHandle WanderTimer;


protected:
	virtual void BeginPlay() override;
	FVector GetRandomPointInRadius();

protected:
	UPROPERTY(EditAnywhere, Category="Wander") float WanderRadius = 800.f;
	/** Pause (s) à l'arrivée avant de repartir vers un nouveau point. 0 = repart immédiatement. */
	UPROPERTY(EditAnywhere, Category="Wander", meta=(ClampMin="0"))
	float WaitTime = 2.f;
	/** Délai avant le premier StartWander après BeginPlay (s). */
	UPROPERTY(EditAnywhere, Category="Wander", meta=(ClampMin="0.01"))
	float StartWanderDelay = 0.5f;

private:
};
