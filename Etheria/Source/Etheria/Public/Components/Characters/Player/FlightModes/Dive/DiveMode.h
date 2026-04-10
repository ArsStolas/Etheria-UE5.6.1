/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: DiveMode - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "DiveMode.generated.h"

UCLASS()
class ETHERIA_API UDiveMode : public UFlightModeBase
{
    GENERATED_BODY()

public:
    virtual void Enter() override;
    virtual void Exit() override;
    virtual void TickMode(float DeltaTime) override;
    void ConfigureDiveTuning(
        float InMaxDiveSpeed,
        float InMinDiveSpeed,
        float InDiveAcceleration,
        float InDiveDeceleration,
        float InDiveEntrySpeedBonus,
        float InMaxPitch,
        float InMaxRoll,
        float InTurnRateDive,
        float InLiftFactor);

    // Called by WindStreamZone once per frame while the player is inside the stream.
    // TargetSpeed: desired speed enforced by the tunnel.
    // WindDirection: normalized spline tangent at the closest point.
    // Influence: 0-1 alignment strength toward the stream.
    // CenteringAccel: acceleration pulling the player back toward the spline core.
    void ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
                        float Influence, const FVector& CenteringAccel);

    float GetCurrentSpeed() const { return CurrentSpeed; }

protected:
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDiveSpeed = 2600.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="100", ClampMax="1000"))
    float MinDiveSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="2000"))
    float DiveAcceleration = 1400.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="200", ClampMax="1500"))
    float DiveDeceleration = 600.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="0", ClampMax="1500"))
    float DiveEntrySpeedBonus = 250.f;

    float CurrentSpeed = 400.f;

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="15", ClampMax="90"))
    float MaxPitch = 60.f;

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="10", ClampMax="90"))
    float MaxRoll = 30.f;

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="30", ClampMax="180"))
    float TurnRateDive = 90.f;

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="0.1", ClampMax="2.0"))
    float LiftFactor = 0.6f;

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="-50"))
    float DiveGravity = -100.f;

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="0"))
    float AutonomousGlideDecay = -200.f;

    UPROPERTY(EditAnywhere, Category="Dive|Debug")
    bool bDiveDebugMode = false;

private:
    bool bWindStreamActive = false;
    float PendingWindTargetSpeed = 0.f;
    float PendingWindAlignmentStrength = 0.f;
    FVector PendingWindDirection = FVector::ZeroVector;
    FVector PendingWindCenteringAccel = FVector::ZeroVector;

    float ComputeBaseSinkSpeed() const;
};
