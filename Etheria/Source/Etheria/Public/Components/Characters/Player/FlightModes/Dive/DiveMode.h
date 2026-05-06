/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: DiveMode - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "DiveMode.generated.h"

UCLASS(BlueprintType)
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
        float InDiveCruiseSpeed,
        float InDiveAcceleration,
        float InDiveDeceleration,
        float InDiveEntrySpeedBonus,
        float InWindStreamAcceleration,
        float InMaxPitch,
        float InMaxRoll,
        float InTurnRateDive,
        float InLiftFactor,
        float InPitchResponse,
        float InRollResponse,
        float InCruiseInterpSpeed,
        float InTurnDrag,
        float InNeutralSinkSpeed,
        float InMaxDiveSinkSpeed,
        float InMaxClimbSpeed,
        float InLowSpeedClimbSink,
        float InClimbSpeedCostMultiplier,
        bool bInUseInputAttitude,
        bool bInDiveDebugMode);

    UFUNCTION(BlueprintCallable, Category="Dive|Debug")
    void SetDiveDebugMode(bool bEnabled) { bDiveDebugMode = bEnabled; }
    UFUNCTION(BlueprintCallable, Category="Dive|Rotation")
    void SetUseInputAttitude(bool bEnabled) { bUseInputAttitude = bEnabled; }

    // Called by WindStreamZone once per frame while the player is inside the stream.
    // TargetSpeed: desired speed enforced by the tunnel.
    // WindDirection: normalized spline tangent at the closest point.
    // Influence: 0-1 alignment strength toward the stream.
    // CenteringAccel: acceleration pulling the player back toward the spline core.
    void ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
                        float Influence, const FVector& CenteringAccel);

    float GetCurrentSpeed() const { return CurrentSpeed; }
    UFUNCTION(BlueprintPure, Category="Dive|Animation")
    FVector2D GetDiveDirection() const { return DiveDirection; }
    UFUNCTION(BlueprintPure, Category="Dive|Animation")
    float GetDiveBankDirection() const { return DiveDirection.X; }
    UFUNCTION(BlueprintPure, Category="Dive|Animation")
    float GetDivePitchDirection() const { return DiveDirection.Y; }

protected:
    // Highest autonomous dive speed in cm/s, before wind streams apply their own boost cap.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDiveSpeed = 2600.f;

    // Lowest speed kept while diving; climbing stalls toward this instead of stopping instantly.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="100", ClampMax="1000"))
    float MinDiveSpeed = 300.f;

    // Speed the dive naturally settles toward with no vertical input.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
    float DiveCruiseSpeed = 1500.f;

    // Acceleration gained when pitching down.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="2000"))
    float DiveAcceleration = 1400.f;

    // Deceleration used by climb, neutral drag, and turn drag.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="200", ClampMax="1500"))
    float DiveDeceleration = 600.f;

    // Extra speed added on dive entry, preserving existing fall/glide momentum when possible.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="0", ClampMax="1500"))
    float DiveEntrySpeedBonus = 250.f;

    // Acceleration used when a WindStreamZone asks the dive to ramp toward stream speed.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="20000"))
    float WindStreamAcceleration = 2400.f;

    float CurrentSpeed = 400.f;

    // Maximum pitch used only when input-driven attitude is enabled.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="15", ClampMax="90"))
    float MaxPitch = 60.f;

    // Maximum bank angle used only when input-driven attitude is enabled.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="10", ClampMax="90"))
    float MaxRoll = 30.f;

    // Yaw rate at normal dive speed; high speed scales this up for an arcade banking feel.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="30", ClampMax="180"))
    float TurnRateDive = 90.f;

    // Scales how much speed turns into climb potential. 0.6 is the neutral/default wingsuit value.
    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="0.1", ClampMax="2.0"))
    float LiftFactor = 0.6f;

    // How quickly the character reaches the requested pitch when input-driven attitude is enabled.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0"))
    float PitchResponse = 5.5f;

    // How quickly the character banks into left/right input when input-driven attitude is enabled.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0"))
    float RollResponse = 7.f;

    // Interp speed back toward DiveCruiseSpeed when the player is not pitching up/down.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="0.1", ClampMax="10.0"))
    float CruiseInterpSpeed = 1.4f;

    // Extra speed loss while banking, keeping hard turns readable and not free.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="0.0", ClampMax="2.0"))
    float TurnDrag = 0.28f;

    // Downward vertical target in normal wingsuit glide with no pitch input.
    UPROPERTY(EditAnywhere, Category="Dive|Vertical", meta=(ClampMin="50", ClampMax="1500"))
    float NeutralSinkSpeed = 260.f;

    // Downward vertical target when pitching fully down.
    UPROPERTY(EditAnywhere, Category="Dive|Vertical", meta=(ClampMin="500", ClampMax="4000"))
    float MaxDiveSinkSpeed = 2300.f;

    // Upward vertical target when pitching fully up with enough speed.
    UPROPERTY(EditAnywhere, Category="Dive|Vertical", meta=(ClampMin="100", ClampMax="1500"))
    float MaxClimbSpeed = 1200.f;

    // Extra downward penalty applied to climb attempts at low speed to prevent infinite flight.
    UPROPERTY(EditAnywhere, Category="Dive|Vertical", meta=(ClampMin="0", ClampMax="1500"))
    float LowSpeedClimbSink = 180.f;

    // Multiplier for speed loss while pulling up.
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="0.5", ClampMax="4.0"))
    float ClimbSpeedCostMultiplier = 1.2f;

    // If enabled, movement inputs also pitch/roll the actor. If disabled, inputs affect physics only.
    UPROPERTY(EditAnywhere, Category="Dive|Rotation")
    bool bUseInputAttitude = false;

    UPROPERTY(EditAnywhere, Category="Dive|Debug")
    bool bDiveDebugMode = false;

private:
    // X: bank left/right (-1 left, +1 right)
    // Y: pitch/climb intent (-1 climb, +1 dive)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dive|Animation", meta=(AllowPrivateAccess="true"))
    FVector2D DiveDirection = FVector2D::ZeroVector;

    bool bWindStreamActive = false;
    float PendingWindTargetSpeed = 0.f;
    float PendingWindAlignmentStrength = 0.f;
    FVector PendingWindDirection = FVector::ZeroVector;
    FVector PendingWindCenteringAccel = FVector::ZeroVector;

    float GetSpeedAlpha() const;
};
