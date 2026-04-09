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
    MaxDiveSpeed = InMaxDiveSpeed;
    MinDiveSpeed = InMinDiveSpeed;
    DiveAcceleration = InDiveAcceleration;
    DiveDeceleration = InDiveDeceleration;
    DiveEntrySpeedBonus = InDiveEntrySpeedBonus;
    MaxPitch = InMaxPitch;
    MaxRoll = InMaxRoll;
    TurnRateDive = InTurnRateDive;
    LiftFactor = InLiftFactor;
}

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

    CurrentSpeed = FMath::Clamp(MinDiveSpeed + DiveEntrySpeedBonus, MinDiveSpeed, MaxDiveSpeed);
    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;

}

void UDiveMode::Exit()
{
    if (!Owner || !Move) return;

    const FRotator Rotation = Owner->GetActorRotation();
    Owner->SetActorRotation(FRotator(0.f, Rotation.Yaw, 0.f));

    RestoreMovementSettings();

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
    if (!bHasWind)
    {
        const float TurnDrag = FMath::Abs(RollFactor) * DiveDeceleration * 0.3f * DeltaTime;
        const float ClimbDrag = FMath::Max(PitchFactor, 0.f) * DiveDeceleration * 0.65f * DeltaTime;
        CurrentSpeed -= (TurnDrag + ClimbDrag);
    }
    if (bHasWind)
    {
        CurrentSpeed = FMath::Max(CurrentSpeed, PendingWindTargetSpeed);
    }

    const float MaxAllowedDiveSpeed = bHasWind
        ? FMath::Max(MaxDiveSpeed * 1.5f, PendingWindTargetSpeed * 1.15f)
        : MaxDiveSpeed * 1.5f;
    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxAllowedDiveSpeed);

    if (FMath::Abs(RollFactor) > 0.1f)
    {
        const float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        NewRot.Yaw += RollFactor * TurnRateDive * SpeedScale * DeltaTime;
    }

    const bool bPlayerProvidingSteer = !FMath::IsNearlyZero(HorizontalInput) || !FMath::IsNearlyZero(VerticalInput);
    if (bHasWind)
    {
        const float AlignmentStrength = bPlayerProvidingSteer
            ? PendingWindAlignmentStrength * 0.08f
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
        const FVector CurrentDirection = Velocity.IsNearlyZero()
            ? NewRot.Vector().GetSafeNormal()
            : Velocity.GetSafeNormal();
        const float HorizontalSteeringStrength = FMath::Clamp(FMath::Abs(HorizontalInput), 0.f, 1.f);
        const float VerticalSteeringStrength = FMath::Clamp(FMath::Abs(VerticalInput), 0.f, 1.f);

        FRotator CurrentDirRot = CurrentDirection.Rotation();
        const FRotator TargetDirRot = NewRot;

        const float YawBlend = FMath::Lerp(0.32f, 0.62f, HorizontalSteeringStrength);
        const float PitchBlend = FMath::Lerp(0.18f, 0.38f, VerticalSteeringStrength);

        FRotator BlendedDirRot = CurrentDirRot;
        BlendedDirRot.Yaw = FMath::Lerp(CurrentDirRot.Yaw, TargetDirRot.Yaw, YawBlend);
        BlendedDirRot.Pitch = FMath::Lerp(CurrentDirRot.Pitch, TargetDirRot.Pitch, PitchBlend);

        const FVector PlayerDesiredDirection = BlendedDirRot.Vector().GetSafeNormal();
        const FVector StreamVelocity = PendingWindDirection * CurrentSpeed;
        const FVector PlayerVelocity = PlayerDesiredDirection * CurrentSpeed;

        const float GuidanceBlend = bPlayerProvidingSteer
            ? FMath::Clamp(PendingWindAlignmentStrength * 0.18f, 0.f, 0.45f)
            : FMath::Clamp(PendingWindAlignmentStrength * 0.7f, 0.f, 0.92f);

        FVector DesiredVelocity = FMath::Lerp(PlayerVelocity, StreamVelocity, GuidanceBlend);
        DesiredVelocity += PendingWindCenteringAccel * DeltaTime;

        if (DesiredVelocity.IsNearlyZero())
        {
            DesiredVelocity = StreamVelocity;
        }

        const float MinForwardSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 0.72f : 0.9f);
        const float ForwardAlongStream = FVector::DotProduct(DesiredVelocity, PendingWindDirection);
        if (ForwardAlongStream < MinForwardSpeed)
        {
            DesiredVelocity += PendingWindDirection * (MinForwardSpeed - ForwardAlongStream);
        }

        const float TargetWindSpeed = FMath::Max(CurrentSpeed, PendingWindTargetSpeed);
        const float SmoothedSpeed = FMath::FInterpTo(CurrentSpeed, TargetWindSpeed, DeltaTime, bPlayerProvidingSteer ? 2.5f : 4.5f);
        CurrentSpeed = FMath::Max(CurrentSpeed, SmoothedSpeed);

        const float MaxAllowedSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 1.06f : 1.02f);
        DesiredVelocity = FMath::Lerp(StreamVelocity, DesiredVelocity, bPlayerProvidingSteer ? 0.72f : 0.9f);
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
            LiftAccelZ = LiftCoeff * FMath::Clamp(PitchFactor, 0.f, 1.f) * (CurrentSpeed * 0.38f);
        }

        const float DivePushZ = FMath::Abs(FMath::Min(PitchFactor, 0.f)) * FMath::Lerp(350.f, 1100.f, FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.f, 1.f));
        const float BaseSinkSpeed = ComputeBaseSinkSpeed();
        const float LowSpeedSinkPenalty = FMath::Lerp(380.f, 0.f, FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.f, 1.f));
        const float VerticalTargetSpeed = -BaseSinkSpeed - DivePushZ - LowSpeedSinkPenalty + LiftAccelZ;
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
