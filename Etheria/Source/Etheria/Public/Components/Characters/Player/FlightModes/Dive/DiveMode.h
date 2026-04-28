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

    /**
     * Appelé par WindStreamZone une fois par frame tant que le player est dans le stream.
     * @param TargetSpeed  Vitesse souhaitée par le tunnel.
     * @param WindDirection  Tangente normalisée de la spline au point le plus proche.
     * @param Influence  Force d'alignement 0-1 vers le stream.
     * @param CenteringAccel  Accélération ramenant vers le core de la spline.
     */
    void ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
                        float Influence, const FVector& CenteringAccel);

    float GetCurrentSpeed() const { return CurrentSpeed; }

protected:

    // ── Speed ──────────────────────────────────────────────────────────────────

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

    // ── Rotation ───────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="15", ClampMax="90"))
    float MaxPitch = 60.f;

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="10", ClampMax="90"))
    float MaxRoll = 30.f;

    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="30", ClampMax="180"))
    float TurnRateDive = 90.f;

    // ── Physics ────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="0.1", ClampMax="2.0"))
    float LiftFactor = 0.6f;

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="-50"))
    float DiveGravity = -100.f;

    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="0"))
    float AutonomousGlideDecay = -200.f;

    // ── Debug ──────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, Category="Dive|Debug")
    bool bDiveDebugMode = false;

private:

    // ── Wind stream state (reset chaque tick) ─────────────────────────────────

    bool    bWindStreamActive            = false;
    float   PendingWindTargetSpeed       = 0.f;
    float   PendingWindAlignmentStrength = 0.f;
    FVector PendingWindDirection         = FVector::ZeroVector;
    FVector PendingWindCenteringAccel    = FVector::ZeroVector;

    // ── Wind smooth entry/exit ────────────────────────────────────────────────

    /**
     * Alpha 0→1 représentant l'influence progressive du wind stream.
     * Monte lentement à l'entrée, descend lentement à la sortie.
     * Permet une prise de vitesse et un alignement graduels.
     */
    float WindInfluenceAlpha    = 0.f;

    /**
     * Vitesse actuellement interpolée vers PendingWindTargetSpeed.
     * Évite le snap instantané à la vitesse maximale du stream.
     */
    float WindSpeedBlendCurrent = 0.f;

    // ── Helpers ───────────────────────────────────────────────────────────────

    float ComputeBaseSinkSpeed() const;
    void  ResetWindState();
};