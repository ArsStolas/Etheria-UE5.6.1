/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: DiveMode - Source
*/

#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

#if UE_BUILD_SHIPPING
    #define DIVE_LOG(Verbosity, Format, ...)
    #define DIVE_SCREEN(Key, Color, Format, ...)
#else
    #define DIVE_LOG(Verbosity, Format, ...) \
        if (bDiveDebugMode) UE_LOG(LogTemp, Verbosity, Format, ##__VA_ARGS__)
    #define DIVE_SCREEN(Key, Color, Format, ...) \
        if (bDiveDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

void UDiveMode::Enter()
{
    if (!Owner || !Move) return;

    StoreMovementSettings();
    Move->SetMovementMode(MOVE_Flying);
    Move->bUseControllerDesiredRotation = false;
    Move->bOrientRotationToMovement     = false;
    Move->GravityScale                  = 0.f;
    Move->AirControl                    = 1.f;
    Move->BrakingDecelerationFalling    = 0.f;

    CurrentSpeed               = MinDiveSpeed;
    PendingWindSpeedBoost      = 0.f;
    PendingWindYaw             = FLT_MAX;
    PendingWindVerticalForce   = 0.f;

    if (Owner->GetGliderVisual())
        Owner->GetGliderVisual()->SetVisibility(false);
}

void UDiveMode::Exit()
{
    if (!Owner || !Move) return;

    FRotator R = Owner->GetActorRotation();
    Owner->SetActorRotation(FRotator(0.f, R.Yaw, 0.f));

    RestoreMovementSettings();

    if (Owner->GetGliderVisual())
        Owner->GetGliderVisual()->SetVisibility(false);

    CurrentSpeed             = MinDiveSpeed;
    PendingWindSpeedBoost    = 0.f;
    PendingWindYaw           = FLT_MAX;
    PendingWindVerticalForce = 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// ApplyWindBoost — appelé par WindStreamZone avant TickMode
// ─────────────────────────────────────────────────────────────────────────────
void UDiveMode::ApplyWindBoost(float SpeedBoost, const FVector& WindTangent,
                                float Influence, float VerticalForce, float DeltaTime)
{
    PendingWindSpeedBoost    += SpeedBoost;
    PendingWindVerticalForce += VerticalForce;

    if (Influence > 0.f && !WindTangent.IsNearlyZero())
    {
        PendingWindYaw     = FMath::RadiansToDegrees(FMath::Atan2(WindTangent.Y, WindTangent.X));
        WindYawInterpSpeed = Influence;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TickMode
// ─────────────────────────────────────────────────────────────────────────────
void UDiveMode::TickMode(float DeltaTime)
{
    if (!Owner) return;

    const int Hor = Owner->GetHorizontalInput();
    const int Ver = Owner->GetVerticalInput();

    // ── PITCH / ROLL ──────────────────────────────────────────────────────────
    const float TargetPitch = FMath::Clamp(-Ver * MaxPitch, -MaxPitch, MaxPitch);
    const float TargetRoll  = FMath::Clamp(Hor  * MaxRoll,  -MaxRoll,  MaxRoll);

    FRotator CurrentRot = Owner->GetActorRotation();
    FRotator NewRot     = FMath::RInterpTo(CurrentRot,
        FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll), DeltaTime, 3.f);
    Owner->SetActorRotation(NewRot);

    const float PitchFactor = NewRot.Pitch / MaxPitch;
    const float RollFactor  = NewRot.Roll  / MaxRoll;

    // ── VITESSE SCALAIRE ──────────────────────────────────────────────────────
    if      (PitchFactor < -0.1f)
        CurrentSpeed += DiveAcceleration * (1.0f + 0.2f * FMath::Abs(PitchFactor)) * DeltaTime;
    else if (PitchFactor >  0.1f)
        CurrentSpeed -= DiveDeceleration * (0.6f + 0.4f * PitchFactor) * DeltaTime;
    else
        CurrentSpeed -= DiveDeceleration * 0.15f * DeltaTime;

    // ── WIND SPEED BOOST ──────────────────────────────────────────────────────
    const bool bHasWind = PendingWindSpeedBoost > 0.f;
    if (bHasWind)
    {
        CurrentSpeed += PendingWindSpeedBoost;
        DIVE_SCREEN(10, FColor::Cyan, TEXT("[Wind] +%.0f cm/s → Speed: %.0f"), PendingWindSpeedBoost, CurrentSpeed);
    }
    PendingWindSpeedBoost = 0.f;

    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxDiveSpeed * 1.5f);

    // ── YAW joueur ────────────────────────────────────────────────────────────
    if (FMath::Abs(RollFactor) > 0.1f)
    {
        const float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        FRotator YawRot = Owner->GetActorRotation();
        YawRot.Yaw += RollFactor * TurnRateDive * SpeedScale * DeltaTime;
        Owner->SetActorRotation(FRotator(NewRot.Pitch, YawRot.Yaw, NewRot.Roll));
        NewRot = Owner->GetActorRotation();
    }

    // ── WIND GUIDANCE YAW ────────────────────────────────────────────────────
    // Actif seulement si le joueur ne steer pas fort (il garde le contrôle)
    const bool bPlayerSteering = FMath::Abs(RollFactor) > 0.4f;
    if (PendingWindYaw != FLT_MAX && !bPlayerSteering)
    {
        const float InterpSpeed = FMath::Clamp(WindYawInterpSpeed * 60.f, 10.f, 80.f);
        FRotator GuidedRot = Owner->GetActorRotation();
        GuidedRot.Yaw = FMath::FInterpTo(GuidedRot.Yaw, PendingWindYaw, DeltaTime, InterpSpeed);
        Owner->SetActorRotation(FRotator(NewRot.Pitch, GuidedRot.Yaw, NewRot.Roll));
        NewRot = Owner->GetActorRotation();
        DIVE_SCREEN(11, FColor::Cyan, TEXT("[Wind] Yaw → %.1f° (interp %.1f)"), PendingWindYaw, InterpSpeed);
    }
    PendingWindYaw     = FLT_MAX;
    WindYawInterpSpeed = 0.f;

    // ── VÉLOCITÉ HORIZONTALE ──────────────────────────────────────────────────
    const FVector ForwardDir = FRotationMatrix(FRotator(0.f, NewRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
    FVector Velocity = Move->Velocity;
    Velocity.X = ForwardDir.X * CurrentSpeed;
    Velocity.Y = ForwardDir.Y * CurrentSpeed;

    // ── GRAVITÉ ARCADE + PORTANCE ─────────────────────────────────────────────
    const float GravityStrength = 400.f;
    const float StallSpeed      = MaxDiveSpeed * 0.4f;
    const float MaxLiftCoeff    = LiftFactor * 1.8f;

    float LiftCoeff = 0.f;
    if (CurrentSpeed > StallSpeed)
    {
        const float SpeedRatio = FMath::Clamp(
            (CurrentSpeed - StallSpeed) / (MaxDiveSpeed - StallSpeed), 0.f, 1.f);
        LiftCoeff = MaxLiftCoeff * SpeedRatio;
    }

    float LiftAccelZ = 0.f;
    if (PitchFactor > 0.f && LiftCoeff > 0.f)
        LiftAccelZ = LiftCoeff * FMath::Clamp(PitchFactor, 0.f, 1.f) * (CurrentSpeed * 0.5f);

    // Accélération Z = gravité arcade + portance + correction verticale du wind stream
    // La correction wind annule partiellement la gravité pour maintenir le joueur
    // dans le tube — proportionnelle à l'écart Z et au Falloff (voir WindStreamZone)
    float AccelZ = LiftAccelZ - GravityStrength + PendingWindVerticalForce;
    PendingWindVerticalForce = 0.f;

    Velocity.Z = FMath::Clamp(Velocity.Z + AccelZ * DeltaTime, -2400.f, 900.f);
    Move->Velocity = Velocity;

    // ── DEBUG SCREEN ──────────────────────────────────────────────────────────
    if (GEngine)
    {
        const FColor C = (Velocity.Z > 50.f) ? FColor::Green :
                         (Velocity.Z < -50.f) ? FColor::Red : FColor::White;
        GEngine->AddOnScreenDebugMessage(1, 0.f, C, FString::Printf(
            TEXT("Alt: %.0f | VZ: %.0f | Speed: %.0f | Stall: %.0f | Lift: %.2f | Pitch: %.1f° %s"),
            Owner->GetActorLocation().Z, Velocity.Z, CurrentSpeed,
            StallSpeed, LiftCoeff, NewRot.Pitch,
            bHasWind ? TEXT("| 💨 WIND") : TEXT("")));
    }
}