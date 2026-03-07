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

    // Appelé par WindStreamZone chaque tick
    // SpeedBoost     : boost scalaire ajouté à CurrentSpeed
    // WindTangent    : direction normalisée de la spline au point le plus proche
    // Influence      : 0-1, guidance Yaw vers la tangente
    // VerticalForce  : force en cm/s² à ajouter à Velocity.Z (correction hauteur spline)
    // DeltaTime      : depuis le tick du stream
    void ApplyWindBoost(float SpeedBoost, const FVector& WindTangent,
                        float Influence, float VerticalForce, float DeltaTime);

    float GetCurrentSpeed() const { return CurrentSpeed; }

protected:
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDiveSpeed = 2600.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="100", ClampMax="1000"))
    float MinDiveSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="2000"))
    float DiveAcceleration = 1400.f;

    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="200", ClampMax="1500"))
    float DiveDeceleration = 800.f;

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
    float PendingWindSpeedBoost  = 0.f;
    float PendingWindYaw         = FLT_MAX;
    float WindYawInterpSpeed     = 0.f;
    // Force verticale accumulée depuis le stream (contrecarre la gravité dans le tube)
    float PendingWindVerticalForce = 0.f;
};