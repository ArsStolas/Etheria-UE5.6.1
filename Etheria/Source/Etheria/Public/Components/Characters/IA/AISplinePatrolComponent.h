/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AISplinePatrolComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/Paths/AISplinePath.h"
#include "AISplinePatrolComponent.generated.h"

UENUM(BlueprintType)
enum class ESplinePatrolMode : uint8
{
	Loop,
	BackAndForth
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAISplinePatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void StartPatrol();
	UFUNCTION(BlueprintCallable)
	void MoveToNextPoint();

	void AdvanceIndex();

	void SnapToClosestPoint();

	FTimerHandle PatrolTimerHandle;
	UPROPERTY(EditAnywhere, Category="Spline") float WaitTimeAtPoint = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spline")
	bool bIsMovingToPoint = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, Category="Spline") AAISplinePath* SplinePath;
	UPROPERTY(EditAnywhere, Category="Spline") ESplinePatrolMode PatrolMode = ESplinePatrolMode::Loop;

private:
	int32 CurrentIndex = 0;
	int32 Direction = 1;
};
