/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "LockTargetComponent" - Header
 */
#pragma once

#include "Components/ActorComponent.h"
#include "LockTargetComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEELockChanged, AActor*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API ULockTargetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockTargetComponent();

    UFUNCTION(BlueprintCallable, Category="Lock")
    bool ToggleLock(AActor* Preferred = nullptr);

    UFUNCTION(BlueprintCallable, Category="Lock")
    void ClearLock();

    UFUNCTION(BlueprintCallable, Category="Lock")
    bool SwitchTarget(bool bRight);

    UFUNCTION(BlueprintPure, Category="Lock")
    AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

    UPROPERTY(BlueprintAssignable, Category="Lock")
    FEELockChanged OnLockChanged;

protected:
    virtual void BeginPlay() override;

private:
    AActor* FindBestTarget(AActor* Preferred) const;
    TArray<AActor*> GatherCandidates() const;
    float ScoreCandidate(AActor* Candidate, const FVector& EyeLoc, const FVector& Forward) const;

private:
    UPROPERTY(EditAnywhere, Category="Lock|Config", meta=(ClampMin="100.0"))
    float MaxDistance = 2500.f;

    UPROPERTY(EditAnywhere, Category="Lock|Config", meta=(ClampMin="0.0", ClampMax="90.0"))
    float MaxAngleDeg = 55.f;

    UPROPERTY(EditAnywhere, Category="Lock|Config")
    TEnumAsByte<ECollisionChannel> TargetChannel = ECC_Pawn;

    TWeakObjectPtr<AActor> CurrentTarget;
};
