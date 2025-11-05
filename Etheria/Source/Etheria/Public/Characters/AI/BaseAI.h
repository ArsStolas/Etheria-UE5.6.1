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
    float SplineFollowSpeed = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement")
    float SplinePointReachDist = 80.f;

    UFUNCTION(BlueprintImplementableEvent, Category = "AI|Animation")
    void PlayIdleAnimation();

    UFUNCTION(BlueprintImplementableEvent, Category = "AI|VoiceLine")
    void PlayIdleVoiceLine();

    UFUNCTION(BlueprintImplementableEvent, Category = "AI")
    void OnAIDeath();

    int CurrentIdlePointIndex = 0;
    float CurrentSplineProgress = 0.f;
    FVector SplineCurrentTarget = FVector::ZeroVector;
    bool bSplineActive = false;
    int SplineDirection = 1; // 1 avant, -1 arrière

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    bool bIsHostile = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    AActor* CurrentTarget = nullptr;

    UFUNCTION(BlueprintCallable, Category="AI|Movement")
    void MoveToIdlePoint();

    UFUNCTION(BlueprintCallable, Category="AI|Movement")
    void UpdateSplineMove(float DeltaTime);
};
