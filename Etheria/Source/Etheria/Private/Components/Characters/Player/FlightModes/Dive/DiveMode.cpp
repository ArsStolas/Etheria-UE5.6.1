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
    Move->bOrientRotationToMovement = false;
    Move->GravityScale = 0.f;
    Move->AirControl = 1.f;
    Move->BrakingDecelerationFalling = 0.f;

    CurrentSpeed = MinDiveSpeed;
    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;

    if (Owner->GetGliderVisual())
    {
        Owner->GetGliderVisual()->SetVisibility(false);
    }
}

void UDiveMode::Exit()
{
    if (!Owner || !Move) return;

    const FRotator Rotation = Owner->GetActorRotation();
    Owner->SetActorRotation(FRotator(0.f, Rotation.Yaw, 0.f));

    RestoreMovementSettings();

    if (Owner->GetGliderVisual())
    {
        Owner->GetGliderVisual()->SetVisibility(false);
    }

    CurrentSpeed = MinDiveSpeed;
    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;
}

void UDiveMode::ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
                               float Influence, const FVector& CenteringAccel)
{
    if (TargetSpeed <= 0.f || WindDirection.IsNearlyZero())
    {
        return;
    }

    bWindStreamActive = true;
    PendingWindTargetSpeed = FMath::Max(PendingWindTargetSpeed, TargetSpeed);
    PendingWindAlignmentStrength = FMath::Max(PendingWindAlignmentStrength, Influence);
    PendingWindDirection = WindDirection.GetSafeNormal();
    PendingWindCenteringAccel = CenteringAccel;
}

float UDiveMode::ComputeBaseSinkSpeed() const
{
    const float SpeedAlpha = FMath::GetMappedRangeValueClamped(
        FVector2D(MinDiveSpeed, MaxDiveSpeed),
        FVector2D(0.f, 1.f),
        CurrentSpeed);

    return FMath::Lerp(850.f, 120.f, SpeedAlpha);
}

void UDiveMode::TickMode(float DeltaTime)
{
    if (!Owner || !Move) return;

    const float HorizontalInput = Owner->GetHorizontalInput();
    const float VerticalInput = Owner->GetVerticalInput();

    const float TargetPitch = FMath::Clamp(-VerticalInput * MaxPitch, -MaxPitch, MaxPitch);
    const float TargetRoll = FMath::Clamp(HorizontalInput * MaxRoll, -MaxRoll, MaxRoll);

    FRotator CurrentRot = Owner->GetActorRotation();
    FRotator NewRot = FMath::RInterpTo(
        CurrentRot,
        FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll),
        DeltaTime,
        3.f);

    const float PitchFactor = MaxPitch > KINDA_SMALL_NUMBER ? (NewRot.Pitch / MaxPitch) : 0.f;
    const float RollFactor = MaxRoll > KINDA_SMALL_NUMBER ? (NewRot.Roll / MaxRoll) : 0.f;

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

    const bool bHasWind = bWindStreamActive && PendingWindTargetSpeed > 0.f && !PendingWindDirection.IsNearlyZero();
    if (bHasWind)
    {
        CurrentSpeed = FMath::Max(CurrentSpeed, PendingWindTargetSpeed);
    }

    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxDiveSpeed * 1.5f);

    if (FMath::Abs(RollFactor) > 0.1f)
    {
        const float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        NewRot.Yaw += RollFactor * TurnRateDive * SpeedScale * DeltaTime;
    }

    const bool bPlayerProvidingSteer = !FMath::IsNearlyZero(HorizontalInput) || !FMath::IsNearlyZero(VerticalInput);
    if (bHasWind)
    {
        const float AlignmentStrength = bPlayerProvidingSteer
            ? PendingWindAlignmentStrength * 0.15f
            : PendingWindAlignmentStrength;

        if (AlignmentStrength > 0.f)
        {
            FRotator WindRot = PendingWindDirection.Rotation();
            WindRot.Pitch = NewRot.Pitch;
            WindRot.Roll = NewRot.Roll;

            const float AlignInterpSpeed = FMath::Lerp(1.2f, 5.f, FMath::Clamp(AlignmentStrength, 0.f, 1.f));
            NewRot = FMath::RInterpTo(NewRot, WindRot, DeltaTime, AlignInterpSpeed);
        }
    }

    Owner->SetActorRotation(NewRot);

    FVector Velocity = Move->Velocity;
    if (bHasWind)
    {
        const FVector CurrentForward = FVector::VectorPlaneProject(
            Velocity.IsNearlyZero() ? NewRot.Vector() * CurrentSpeed : Velocity,
            FVector::UpVector);

        const FVector CurrentLateralToStream = FVector::VectorPlaneProject(CurrentForward, PendingWindDirection);
        const float SteeringCarry = bPlayerProvidingSteer ? 0.95f : 0.75f;
        const FVector CarriedLateralVelocity = CurrentLateralToStream * SteeringCarry;

        FVector DesiredVelocity = (PendingWindDirection * CurrentSpeed) + CarriedLateralVelocity;
        DesiredVelocity += PendingWindCenteringAccel * DeltaTime;

        const float MinForwardSpeed = CurrentSpeed * 0.9f;
        const float ForwardAlongStream = FVector::DotProduct(DesiredVelocity, PendingWindDirection);
        if (ForwardAlongStream < MinForwardSpeed)
        {
            DesiredVelocity += PendingWindDirection * (MinForwardSpeed - ForwardAlongStream);
        }

        const float MaxAllowedSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 1.1f : 1.02f);
        Velocity = DesiredVelocity.GetClampedToMaxSize(MaxAllowedSpeed);

        DIVE_SCREEN(10, FColor::Cyan,
            TEXT("[Wind] Speed %.0f | Align %.2f | VZ %.0f"),
            CurrentSpeed,
            PendingWindAlignmentStrength,
            Velocity.Z);
    }
    else
    {
        const FVector ForwardDir = FRotationMatrix(FRotator(0.f, NewRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
        Velocity.X = ForwardDir.X * CurrentSpeed;
        Velocity.Y = ForwardDir.Y * CurrentSpeed;

        const float StallSpeed = MaxDiveSpeed * 0.4f;
        const float MaxLiftCoeff = LiftFactor * 1.8f;

        float LiftCoeff = 0.f;
        if (CurrentSpeed > StallSpeed)
        {
            const float SpeedRatio = FMath::Clamp(
                (CurrentSpeed - StallSpeed) / (MaxDiveSpeed - StallSpeed),
                0.f,
                1.f);
            LiftCoeff = MaxLiftCoeff * SpeedRatio;
        }

        float LiftAccelZ = 0.f;
        if (PitchFactor > 0.f && LiftCoeff > 0.f)
        {
            LiftAccelZ = LiftCoeff * FMath::Clamp(PitchFactor, 0.f, 1.f) * (CurrentSpeed * 0.5f);
        }

        const float DivePushZ = FMath::Abs(FMath::Min(PitchFactor, 0.f)) * FMath::Lerp(350.f, 1100.f, FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.f, 1.f));
        const float BaseSinkSpeed = ComputeBaseSinkSpeed();
        const float VerticalTargetSpeed = -BaseSinkSpeed - DivePushZ + LiftAccelZ;
        const float VerticalInterp = (VerticalTargetSpeed < Velocity.Z) ? 2.8f : 1.6f;
        Velocity.Z = FMath::FInterpTo(Velocity.Z, VerticalTargetSpeed, DeltaTime, VerticalInterp);
        Velocity.Z = FMath::Clamp(Velocity.Z, -2400.f, 900.f);
    }

    Move->Velocity = Velocity;

    if (GEngine)
    {
        const FColor DebugColor = (Velocity.Z > 50.f) ? FColor::Green :
                                  (Velocity.Z < -50.f) ? FColor::Red : FColor::White;
        GEngine->AddOnScreenDebugMessage(
            1,
            0.f,
            DebugColor,
            FString::Printf(
                TEXT("Alt: %.0f | VZ: %.0f | Speed: %.0f %s"),
                Owner->GetActorLocation().Z,
                Velocity.Z,
                CurrentSpeed,
                bHasWind ? TEXT("| WIND STREAM") : TEXT("")));
    }

    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;
}
