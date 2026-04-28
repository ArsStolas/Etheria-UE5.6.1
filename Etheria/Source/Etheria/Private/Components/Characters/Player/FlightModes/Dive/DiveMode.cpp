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

// ── Tuning constants ──────────────────────────────────────────────────────────

// Vitesse à laquelle l'influence du wind monte/descend (alpha 0↔1)
static constexpr float WindInfluenceRampUpSpeed   = 1.8f;   // secondes pour atteindre 1
static constexpr float WindInfluenceRampDownSpeed = 2.5f;   // secondes pour revenir à 0

// Vitesse d'interpolation vers la target speed du stream
static constexpr float WindSpeedInterpEntering    = 1.4f;   // lent à l'entrée (sensation de prise)
static constexpr float WindSpeedInterpFull        = 3.5f;   // plus rapide une fois aligné

// Cap du stream (évite les vitesses folles en cas de re-enter)
static constexpr float WindSpeedOvershootCap      = 1.25f;

// ─────────────────────────────────────────────────────────────────────────────

void UDiveMode::ConfigureDiveTuning(
    float InMaxDiveSpeed,
    float InMinDiveSpeed,
    float InDiveAcceleration,
    float InDiveDeceleration,
    float InDiveEntrySpeedBonus,
    float InMaxPitch,
    float InMaxRoll,
    float InTurnRateDive,
    float InLiftFactor)
{
    MaxDiveSpeed         = InMaxDiveSpeed;
    MinDiveSpeed         = InMinDiveSpeed;
    DiveAcceleration     = InDiveAcceleration;
    DiveDeceleration     = InDiveDeceleration;
    DiveEntrySpeedBonus  = InDiveEntrySpeedBonus;
    MaxPitch             = InMaxPitch;
    MaxRoll              = InMaxRoll;
    TurnRateDive         = InTurnRateDive;
    LiftFactor           = InLiftFactor;
}

// ─────────────────────────────────────────────────────────────────────────────

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

    CurrentSpeed = FMath::Clamp(MinDiveSpeed + DiveEntrySpeedBonus, MinDiveSpeed, MaxDiveSpeed);

    ResetWindState();
}

void UDiveMode::Exit()
{
    if (!Owner || !Move) return;

    const FRotator Rotation = Owner->GetActorRotation();
    Owner->SetActorRotation(FRotator(0.f, Rotation.Yaw, 0.f));

    RestoreMovementSettings();

    CurrentSpeed = MinDiveSpeed;
    ResetWindState();
}

// ─────────────────────────────────────────────────────────────────────────────

void UDiveMode::ResetWindState()
{
    bWindStreamActive          = false;
    PendingWindTargetSpeed     = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection       = FVector::ZeroVector;
    PendingWindCenteringAccel  = FVector::ZeroVector;
    WindInfluenceAlpha         = 0.f;
    WindSpeedBlendCurrent      = 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────

void UDiveMode::ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
                               float Influence, const FVector& CenteringAccel)
{
    if (TargetSpeed <= 0.f || WindDirection.IsNearlyZero())
        return;

    bWindStreamActive            = true;
    PendingWindTargetSpeed       = FMath::Max(PendingWindTargetSpeed, TargetSpeed);
    PendingWindAlignmentStrength = FMath::Max(PendingWindAlignmentStrength, Influence);
    PendingWindDirection         = WindDirection.GetSafeNormal();
    // Accumulation douce si plusieurs capsules overlappent simultanément
    PendingWindCenteringAccel    = FMath::Lerp(PendingWindCenteringAccel, CenteringAccel, 0.6f);
}

// ─────────────────────────────────────────────────────────────────────────────

float UDiveMode::ComputeBaseSinkSpeed() const
{
    const float SpeedAlpha = FMath::GetMappedRangeValueClamped(
        FVector2D(MinDiveSpeed, MaxDiveSpeed),
        FVector2D(0.f, 1.f),
        CurrentSpeed);

    return FMath::Lerp(850.f, 120.f, SpeedAlpha);
}

// ─────────────────────────────────────────────────────────────────────────────

void UDiveMode::TickMode(float DeltaTime)
{
    if (!Owner || !Move) return;

    const float HorizontalInput = Owner->GetHorizontalInput();
    const float VerticalInput   = Owner->GetVerticalInput();

    // ── 1. Rotation cible ────────────────────────────────────────────────────

    const float TargetPitch = FMath::Clamp(-VerticalInput * MaxPitch, -MaxPitch, MaxPitch);
    const float TargetRoll  = FMath::Clamp(HorizontalInput * MaxRoll, -MaxRoll, MaxRoll);

    FRotator CurrentRot = Owner->GetActorRotation();
    FRotator NewRot = FMath::RInterpTo(
        CurrentRot,
        FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll),
        DeltaTime,
        3.f);

    const float PitchFactor = MaxPitch > KINDA_SMALL_NUMBER ? (NewRot.Pitch / MaxPitch) : 0.f;
    const float RollFactor  = MaxRoll  > KINDA_SMALL_NUMBER ? (NewRot.Roll  / MaxRoll)  : 0.f;

    // ── 2. Wind influence alpha (ramp smooth entrée/sortie) ──────────────────

    const bool bHasWind = bWindStreamActive
        && PendingWindTargetSpeed > 0.f
        && !PendingWindDirection.IsNearlyZero();

    if (bHasWind)
    {
        WindInfluenceAlpha = FMath::FInterpTo(
            WindInfluenceAlpha, 1.f, DeltaTime, WindInfluenceRampUpSpeed);
    }
    else
    {
        WindInfluenceAlpha = FMath::FInterpTo(
            WindInfluenceAlpha, 0.f, DeltaTime, WindInfluenceRampDownSpeed);
    }

    const float EffectiveWindAlpha = WindInfluenceAlpha; // 0-1

    // ── 3. Vitesse de base (aérodynamique) ───────────────────────────────────

    if (PitchFactor < -0.1f)
    {
        CurrentSpeed += DiveAcceleration * (1.0f + 0.2f * FMath::Abs(PitchFactor)) * DeltaTime;
    }
    else if (PitchFactor > 0.1f)
    {
        CurrentSpeed -= DiveDeceleration * (0.6f + 0.4f * PitchFactor) * DeltaTime;
    }
    else
    {
        CurrentSpeed -= DiveDeceleration * 0.15f * DeltaTime;
    }

    // Drag de virage et montée uniquement hors stream (ou atténué dans le stream)
    {
        const float DragScale  = FMath::Lerp(1.f, 0.f, EffectiveWindAlpha); // le stream absorbe le drag
        const float TurnDrag   = FMath::Abs(RollFactor) * DiveDeceleration * 0.3f  * DeltaTime * DragScale;
        const float ClimbDrag  = FMath::Max(PitchFactor, 0.f) * DiveDeceleration * 0.65f * DeltaTime * DragScale;
        CurrentSpeed -= (TurnDrag + ClimbDrag);
    }

    // ── 4. Accélération progressive vers la target wind ──────────────────────

    if (EffectiveWindAlpha > 0.01f && PendingWindTargetSpeed > 0.f)
    {
        // Vitesse d'interp plus rapide une fois que l'alpha est élevé
        const float InterpSpeed = FMath::Lerp(WindSpeedInterpEntering, WindSpeedInterpFull, EffectiveWindAlpha);

        WindSpeedBlendCurrent = FMath::FInterpTo(
            WindSpeedBlendCurrent,
            PendingWindTargetSpeed,
            DeltaTime,
            InterpSpeed);

        // On force la vitesse à monter vers le blend (jamais forcer à descendre)
        CurrentSpeed = FMath::Max(CurrentSpeed, WindSpeedBlendCurrent * EffectiveWindAlpha
                                                + CurrentSpeed * (1.f - EffectiveWindAlpha));
    }
    else if (!bHasWind)
    {
        // Décroissance douce du speed blend quand on quitte le stream
        WindSpeedBlendCurrent = FMath::FInterpTo(WindSpeedBlendCurrent, 0.f, DeltaTime, 1.2f);
    }

    // ── 5. Clamp vitesse ──────────────────────────────────────────────────────

    const float MaxAllowedDiveSpeed = bHasWind
        ? FMath::Max(MaxDiveSpeed * WindSpeedOvershootCap, PendingWindTargetSpeed * 1.1f)
        : MaxDiveSpeed * 1.5f;
    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxAllowedDiveSpeed);

    // ── 6. Yaw (virage) ───────────────────────────────────────────────────────

    if (FMath::Abs(RollFactor) > 0.1f)
    {
        const float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        NewRot.Yaw += RollFactor * TurnRateDive * SpeedScale * DeltaTime;
    }

    // ── 7. Orientation vers le stream (smooth) ────────────────────────────────

    const bool bPlayerProvidingSteer = !FMath::IsNearlyZero(HorizontalInput)
                                    || !FMath::IsNearlyZero(VerticalInput);

    if (EffectiveWindAlpha > 0.01f)
    {
        const float AlignmentStrength = bPlayerProvidingSteer
            ? PendingWindAlignmentStrength * 0.08f * EffectiveWindAlpha
            : PendingWindAlignmentStrength * EffectiveWindAlpha;

        if (AlignmentStrength > KINDA_SMALL_NUMBER)
        {
            FRotator WindRot = PendingWindDirection.Rotation();
            WindRot.Pitch    = NewRot.Pitch;
            WindRot.Roll     = NewRot.Roll;

            const float AlignInterpSpeed = FMath::Lerp(
                0.8f, 4.5f,
                FMath::Clamp(AlignmentStrength, 0.f, 1.f));
            NewRot = FMath::RInterpTo(NewRot, WindRot, DeltaTime, AlignInterpSpeed);
        }
    }

    Owner->SetActorRotation(NewRot);

    // ── 8. Velocity ───────────────────────────────────────────────────────────

    FVector Velocity = Move->Velocity;

    if (EffectiveWindAlpha > 0.01f)
    {
        // Même logique qu'avant mais pondérée par EffectiveWindAlpha

        const FVector CurrentDirection = Velocity.IsNearlyZero()
            ? NewRot.Vector().GetSafeNormal()
            : Velocity.GetSafeNormal();

        const float HorizontalSteeringStrength = FMath::Clamp(FMath::Abs(HorizontalInput), 0.f, 1.f);
        const float VerticalSteeringStrength   = FMath::Clamp(FMath::Abs(VerticalInput),   0.f, 1.f);

        FRotator CurrentDirRot = CurrentDirection.Rotation();
        const FRotator TargetDirRot = NewRot;

        const float YawBlend   = FMath::Lerp(0.32f, 0.62f, HorizontalSteeringStrength);
        const float PitchBlend = FMath::Lerp(0.18f, 0.38f, VerticalSteeringStrength);

        FRotator BlendedDirRot = CurrentDirRot;
        BlendedDirRot.Yaw   = FMath::Lerp(CurrentDirRot.Yaw,   TargetDirRot.Yaw,   YawBlend);
        BlendedDirRot.Pitch = FMath::Lerp(CurrentDirRot.Pitch, TargetDirRot.Pitch, PitchBlend);

        const FVector PlayerDesiredDirection = BlendedDirRot.Vector().GetSafeNormal();
        const FVector StreamVelocity         = PendingWindDirection * CurrentSpeed;
        const FVector PlayerVelocity         = PlayerDesiredDirection * CurrentSpeed;

        // Guidance blend : réduit proportionnellement à alpha (entrée douce)
        const float RawGuidance = bPlayerProvidingSteer
            ? FMath::Clamp(PendingWindAlignmentStrength * 0.18f, 0.f, 0.45f)
            : FMath::Clamp(PendingWindAlignmentStrength * 0.70f, 0.f, 0.92f);
        const float GuidanceBlend = RawGuidance * EffectiveWindAlpha;

        FVector DesiredVelocity = FMath::Lerp(PlayerVelocity, StreamVelocity, GuidanceBlend);

        // Centering — atténué en début de stream
        DesiredVelocity += PendingWindCenteringAccel * DeltaTime * EffectiveWindAlpha;

        if (DesiredVelocity.IsNearlyZero())
            DesiredVelocity = StreamVelocity;

        // Maintain minimum forward projection along stream
        const float MinForwardSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 0.72f : 0.90f);
        const float ForwardAlongStream = FVector::DotProduct(DesiredVelocity, PendingWindDirection);
        if (ForwardAlongStream < MinForwardSpeed)
            DesiredVelocity += PendingWindDirection * (MinForwardSpeed - ForwardAlongStream);

        // Smooth speed maintenance
        const float TargetWindSpeed = FMath::Max(CurrentSpeed, PendingWindTargetSpeed * EffectiveWindAlpha);
        const float InterpRate      = bPlayerProvidingSteer ? 2.5f : 4.5f;
        const float SmoothedSpeed   = FMath::FInterpTo(CurrentSpeed, TargetWindSpeed, DeltaTime, InterpRate);
        CurrentSpeed = FMath::Max(CurrentSpeed, SmoothedSpeed);

        const float MaxAllowedSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 1.06f : 1.02f);
        DesiredVelocity = FMath::Lerp(StreamVelocity, DesiredVelocity,
            bPlayerProvidingSteer ? 0.72f : 0.90f);
        Velocity = DesiredVelocity.GetClampedToMaxSize(MaxAllowedSpeed);

        if (!Velocity.IsNearlyZero())
        {
            const float MaintainedSpeed = FMath::Clamp(
                FMath::Max(TargetWindSpeed * (bPlayerProvidingSteer ? 0.94f : 0.99f), Velocity.Size()),
                0.f,
                MaxAllowedSpeed);
            Velocity = Velocity.GetSafeNormal() * MaintainedSpeed;
        }

        DIVE_SCREEN(10, FColor::Cyan,
            TEXT("[Wind] Alpha %.2f | Speed %.0f | Align %.2f | VZ %.0f"),
            EffectiveWindAlpha,
            CurrentSpeed,
            PendingWindAlignmentStrength,
            Velocity.Z);
    }
    else
    {
        // ── Mode vol libre (pas de stream actif) ──────────────────────────────

        const FVector ForwardDir = FRotationMatrix(FRotator(0.f, NewRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
        Velocity.X = ForwardDir.X * CurrentSpeed;
        Velocity.Y = ForwardDir.Y * CurrentSpeed;

        const float StallSpeed    = MaxDiveSpeed * 0.4f;
        const float MaxLiftCoeff  = LiftFactor * 1.8f;

        float LiftCoeff = 0.f;
        if (CurrentSpeed > StallSpeed)
        {
            const float SpeedRatio = FMath::Clamp(
                (CurrentSpeed - StallSpeed) / (MaxDiveSpeed - StallSpeed), 0.f, 1.f);
            LiftCoeff = MaxLiftCoeff * SpeedRatio;
        }

        float LiftAccelZ = 0.f;
        if (PitchFactor > 0.f && LiftCoeff > 0.f)
        {
            LiftAccelZ = LiftCoeff
                * FMath::Clamp(PitchFactor, 0.f, 1.f)
                * (CurrentSpeed * 0.38f);
        }

        const float DivePushZ = FMath::Abs(FMath::Min(PitchFactor, 0.f))
            * FMath::Lerp(350.f, 1100.f, FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.f, 1.f));
        const float BaseSinkSpeed        = ComputeBaseSinkSpeed();
        const float LowSpeedSinkPenalty  = FMath::Lerp(380.f, 0.f,
            FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.f, 1.f));

        const float VerticalTargetSpeed = -BaseSinkSpeed - DivePushZ - LowSpeedSinkPenalty + LiftAccelZ;
        const float VerticalInterp      = (VerticalTargetSpeed < Velocity.Z) ? 2.8f : 1.6f;
        Velocity.Z = FMath::FInterpTo(Velocity.Z, VerticalTargetSpeed, DeltaTime, VerticalInterp);
        Velocity.Z = FMath::Clamp(Velocity.Z, -2400.f, 900.f);
    }

    Move->Velocity = Velocity;

    // ── 9. Debug HUD ─────────────────────────────────────────────────────────

    if (GEngine)
    {
        const FColor DebugColor = (Velocity.Z > 50.f)  ? FColor::Green
                                : (Velocity.Z < -50.f) ? FColor::Red
                                                        : FColor::White;
        GEngine->AddOnScreenDebugMessage(
            1, 0.f, DebugColor,
            FString::Printf(
                TEXT("Alt: %.0f | VZ: %.0f | Speed: %.0f | WindAlpha: %.2f %s"),
                Owner->GetActorLocation().Z,
                Velocity.Z,
                CurrentSpeed,
                EffectiveWindAlpha,
                bHasWind ? TEXT("| WIND STREAM") : TEXT("")));
    }

    // ── 10. Reset pending wind (re-accumulé au prochain tick par WindStreamZone) ──

    bWindStreamActive            = false;
    PendingWindTargetSpeed       = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection         = FVector::ZeroVector;
    PendingWindCenteringAccel    = FVector::ZeroVector;
}