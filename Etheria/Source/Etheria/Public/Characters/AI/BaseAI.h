/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseAI - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/TargetPoint.h"
#include "BaseAI.generated.h"

UENUM(BlueprintType)
enum class EAIIdleMoveType : uint8
{
    Points  UMETA(DisplayName = "Idle Points"),
    Spline  UMETA(DisplayName = "Follow Spline")
};

UENUM(BlueprintType)
enum class ESplinePatrolMode : uint8
{
    Loop        UMETA(DisplayName = "Loop"),
    BackAndForth UMETA(DisplayName = "Back And Forth")
};

UCLASS()
class ETHERIA_API ABaseAI : public ABaseCharacter
{
    GENERATED_BODY()

public:
    ABaseAI();

protected:
    virtual void BeginPlay() override;
    virtual void HandlePerception();
    virtual void HandleDecisionMaking();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerception;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement")
    EAIIdleMoveType IdleMoveType = EAIIdleMoveType::Points;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Movement")
    USplineComponent* IdleSpline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Movement")
    TArray<ATargetPoint*> IdlePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement")
    ESplinePatrolMode SplinePatrolMode = ESplinePatrolMode::Loop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement")
    float SplineOffset = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement")
    float SplinePointReachDist = 100.f;

    UFUNCTION(BlueprintImplementableEvent, Category = "AI|Animation")
    void PlayIdleAnimation();

    UFUNCTION(BlueprintImplementableEvent, Category = "AI|VoiceLine")
    void PlayIdleVoiceLine();

    UFUNCTION(BlueprintImplementableEvent, Category="AI")
    void OnAIDeath();

    virtual bool CanIdleMove() const { return true; }

    int CurrentIdlePointIndex = 0;
    FVector SplineCurrentTarget = FVector::ZeroVector;
    bool bSplineActive = false;

    UPROPERTY(Transient)
    int SplineDirection = 1;

    UPROPERTY(Transient)
    TArray<float> SplineWaypoints;

    UPROPERTY(Transient)
    int32 CurrentWaypointIndex = 0;

    void MoveTowards(const FVector& TargetLocation, float DeltaTime);

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    bool bIsHostile = false;

    UFUNCTION(BlueprintCallable, Category="AI|Movement")
    void MoveToIdlePoint(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="AI|Movement")
    void UpdateSplineMove(float DeltaTime);

    void InitSplineWaypoints(float Step);
    void MoveToCurrentSplineWaypoint();
};
