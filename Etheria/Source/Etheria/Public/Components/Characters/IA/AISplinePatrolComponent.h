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

UENUM(BlueprintType)
enum class ESplineFollowMode : uint8
{
    Points,
    FollowSpline
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAISplinePatrolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAISplinePatrolComponent();

    void StartPatrol();

    UFUNCTION(BlueprintCallable)
    void MoveToNextPoint();

    void AdvanceIndex();
    void SnapToClosestPoint();
    void OnMoveCompleted();
    void RequestReturnToSpline();
    void SetChasing(bool bInChasing) { bIsChasing = bInChasing; }

    UPROPERTY(EditAnywhere, Category="Spline")
    float DefaultWaitTimeAtPoint = 0.5f;

    UPROPERTY(EditAnywhere, Category="Spline")
    TArray<float> WaitTimesPerPoint;

    UPROPERTY(EditAnywhere, Category="Spline")
    ESplinePatrolMode PatrolMode = ESplinePatrolMode::Loop;

    UPROPERTY(EditAnywhere, Category="Spline")
    ESplineFollowMode FollowMode = ESplineFollowMode::Points;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    float FollowSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    float PointProximityRadius = 80.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spline")
    bool bIsMovingToPoint = false;

    UPROPERTY()
    FTimerHandle PatrolTimerHandle;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditInstanceOnly, Category="Spline")
    AAISplinePath* SplinePath;

private:
    int32 CurrentIndex = 0;
    int32 Direction   = 1;

    float CurrentDistanceOnSpline = 0.f;
    bool  bFollowPaused           = false;
    int32 LastPassedPointIndex    = INDEX_NONE;
    int32 NextPointIndex          = 0;
    bool  bReturningToSpline      = false;
    bool  bIsChasing              = false;

    float GetWaitTimeForPointIndex(int32 PointIndex) const;
    float GetWaitTimeForCurrentPoint() const;

    void MoveToNextPoint_PointsMode();
    void InitFollowSplineStart();
    void UpdateFollowSpline(float DeltaTime);
    void PauseFollowAtPoint(int32 PointIndex);
};
