/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: DiveMode - Source
 */

#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"

#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogDiveMode, Log, All);

#if UE_BUILD_SHIPPING
    #define DIVE_LOG(Verbosity, Format, ...)
    #define DIVE_SCREEN(Key, Color, Format, ...)
#else
    #define DIVE_LOG(Verbosity, Format, ...) \
        if (bDiveDebugMode) UE_LOG(LogDiveMode, Verbosity, Format, ##__VA_ARGS__)
    #define DIVE_SCREEN(Key, Color, Format, ...) \
        if (bDiveDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

void UDiveMode::ConfigureDiveTuning(
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
    bool bInDiveDebugMode)
{
    MaxDiveSpeed = InMaxDiveSpeed;
    MinDiveSpeed = InMinDiveSpeed;
    DiveCruiseSpeed = InDiveCruiseSpeed;
    DiveAcceleration = InDiveAcceleration;
    DiveDeceleration = InDiveDeceleration;
    DiveEntrySpeedBonus = InDiveEntrySpeedBonus;
    WindStreamAcceleration = InWindStreamAcceleration;
    MaxPitch = InMaxPitch;
    MaxRoll = InMaxRoll;
    TurnRateDive = InTurnRateDive;
    LiftFactor = InLiftFactor;
    PitchResponse = InPitchResponse;
    RollResponse = InRollResponse;
    CruiseInterpSpeed = InCruiseInterpSpeed;
    TurnDrag = InTurnDrag;
    NeutralSinkSpeed = InNeutralSinkSpeed;
    MaxDiveSinkSpeed = InMaxDiveSinkSpeed;
    MaxClimbSpeed = InMaxClimbSpeed;
    LowSpeedClimbSink = InLowSpeedClimbSink;
    ClimbSpeedCostMultiplier = InClimbSpeedCostMultiplier;
    bUseInputAttitude = bInUseInputAttitude;
    bDiveDebugMode = bInDiveDebugMode;
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

    const float EntrySpeed = Move->Velocity.Size2D() + DiveEntrySpeedBonus;
    CurrentSpeed = FMath::Clamp(
        FMath::Max(EntrySpeed, MinDiveSpeed + DiveEntrySpeedBonus),
        MinDiveSpeed,
        MaxDiveSpeed);
    DiveDirection = FVector2D::ZeroVector;
    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;

    DIVE_LOG(Log, TEXT("[Dive] ENTER | EntrySpeed=%.0f Velocity=(%.0f,%.0f,%.0f)"),
        CurrentSpeed,
        Move->Velocity.X,
        Move->Velocity.Y,
        Move->Velocity.Z);

}

void UDiveMode::Exit()
{
    if (!Owner || !Move) return;

    const FRotator Rotation = Owner->GetActorRotation();
    Owner->SetActorRotation(FRotator(0.f, Rotation.Yaw, 0.f));

    RestoreMovementSettings();

    CurrentSpeed = MinDiveSpeed;
    DiveDirection = FVector2D::ZeroVector;
    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;

    DIVE_LOG(Log, TEXT("[Dive] EXIT"));
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

float UDiveMode::GetSpeedAlpha() const
{
    return FMath::GetMappedRangeValueClamped(
        FVector2D(MinDiveSpeed, MaxDiveSpeed),
        FVector2D(0.f, 1.f),
        CurrentSpeed);
}

void UDiveMode::TickMode(float DeltaTime)
{
    if (!Owner || !Move) return;

    const float HorizontalInput = Owner->GetHorizontalInput();
    const float VerticalInput = Owner->GetVerticalInput();
    const float DiveInputAlpha = FMath::Clamp(VerticalInput, 0.f, 1.f);
    const float ClimbInputAlpha = FMath::Clamp(-VerticalInput, 0.f, 1.f);
    const float BankInputAlpha = FMath::Abs(HorizontalInput);

    const float TargetPitch = bUseInputAttitude
        ? FMath::Clamp(-VerticalInput * MaxPitch, -MaxPitch, MaxPitch)
        : 0.f;
    const float TargetRoll = bUseInputAttitude
        ? FMath::Clamp(HorizontalInput * MaxRoll, -MaxRoll, MaxRoll)
        : 0.f;

    FRotator CurrentRot = Owner->GetActorRotation().GetNormalized();
    FRotator NewRot = CurrentRot;
    NewRot.Pitch = FMath::FInterpTo(CurrentRot.Pitch, TargetPitch, DeltaTime, PitchResponse);
    NewRot.Roll = FMath::FInterpTo(CurrentRot.Roll, TargetRoll, DeltaTime, RollResponse);
    NewRot.Yaw = CurrentRot.Yaw;

    const float PitchFactor = MaxPitch > KINDA_SMALL_NUMBER ? (NewRot.Pitch / MaxPitch) : 0.f;
    const float RollFactor = MaxRoll > KINDA_SMALL_NUMBER ? (NewRot.Roll / MaxRoll) : 0.f;
    const float SteeringFactor = bUseInputAttitude ? RollFactor : HorizontalInput;
    DiveDirection = FVector2D(
        FMath::Clamp(HorizontalInput, -1.f, 1.f),
        FMath::Clamp(VerticalInput, -1.f, 1.f));

    if (DiveInputAlpha > 0.05f)
    {
        const float DiveAccelScale = FMath::Lerp(0.75f, 1.55f, DiveInputAlpha);
        CurrentSpeed += DiveAcceleration * DiveAccelScale * DeltaTime;
    }
    else if (ClimbInputAlpha > 0.05f)
    {
        const float ClimbCost = DiveDeceleration
            * FMath::Lerp(0.45f, ClimbSpeedCostMultiplier * 0.55f, ClimbInputAlpha)
            * DeltaTime;
        CurrentSpeed -= ClimbCost;
    }
    else
    {
        CurrentSpeed = FMath::FInterpTo(CurrentSpeed, DiveCruiseSpeed, DeltaTime, CruiseInterpSpeed);
    }

    const bool bHasWind = bWindStreamActive && PendingWindTargetSpeed > 0.f && !PendingWindDirection.IsNearlyZero();
    const bool bPlayerProvidingSteer = !FMath::IsNearlyZero(HorizontalInput) || !FMath::IsNearlyZero(VerticalInput);
    if (!bHasWind)
    {
        const float BankDrag = BankInputAlpha * DiveDeceleration * TurnDrag * DeltaTime;
        const float ClimbDrag = ClimbInputAlpha * DiveDeceleration * 0.35f * DeltaTime;
        CurrentSpeed -= (BankDrag + ClimbDrag);
    }
    if (bHasWind)
    {
        const float AccelerationScale = bPlayerProvidingSteer ? 0.78f : 1.f;
        CurrentSpeed = FMath::FInterpConstantTo(
            CurrentSpeed,
            FMath::Max(CurrentSpeed, PendingWindTargetSpeed),
            DeltaTime,
            WindStreamAcceleration * AccelerationScale);
    }

    const float MaxAllowedDiveSpeed = bHasWind
        ? FMath::Max(MaxDiveSpeed * 1.5f, PendingWindTargetSpeed * 1.15f)
        : MaxDiveSpeed;
    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxAllowedDiveSpeed);

    if (FMath::Abs(SteeringFactor) > 0.1f)
    {
        const float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        NewRot.Yaw += SteeringFactor * TurnRateDive * SpeedScale * DeltaTime;
    }

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

        const float StreamForwardAssist = FMath::Clamp(PendingWindAlignmentStrength, 0.f, 1.f);
        const float MinForwardSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 0.72f : 0.9f) * StreamForwardAssist;
        const float ForwardAlongStream = FVector::DotProduct(DesiredVelocity, PendingWindDirection);
        if (StreamForwardAssist > KINDA_SMALL_NUMBER && ForwardAlongStream < MinForwardSpeed)
        {
            DesiredVelocity += PendingWindDirection * (MinForwardSpeed - ForwardAlongStream);
        }

        const float TargetWindSpeed = CurrentSpeed;

        const float MaxAllowedSpeed = CurrentSpeed * (bPlayerProvidingSteer ? 1.06f : 1.02f);
        const float StreamVelocityBlend = FMath::Lerp(
            1.f,
            bPlayerProvidingSteer ? 0.72f : 0.9f,
            StreamForwardAssist);
        DesiredVelocity = FMath::Lerp(StreamVelocity, DesiredVelocity, StreamVelocityBlend);
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

        const float SpeedAlpha = GetSpeedAlpha();
        const float LiftSpeedAlpha = FMath::GetMappedRangeValueClamped(
            FVector2D(MinDiveSpeed + 100.f, MaxDiveSpeed * 0.65f),
            FVector2D(0.f, 1.f),
            CurrentSpeed);
        const float LiftScale = FMath::Max(LiftFactor / 0.6f, 0.1f);
        const float LiftAlpha = FMath::Clamp(
            FMath::InterpEaseOut(0.f, 1.f, LiftSpeedAlpha, 1.35f) * LiftScale,
            0.f,
            1.f);
        const float NeutralVerticalTarget = -NeutralSinkSpeed;
        const float DiveVerticalTarget = -FMath::Lerp(NeutralSinkSpeed, MaxDiveSinkSpeed, DiveInputAlpha)
            * FMath::Lerp(0.78f, 1.f, SpeedAlpha);
        const float ClimbPotential = FMath::Lerp(-NeutralSinkSpeed - LowSpeedClimbSink, MaxClimbSpeed, LiftAlpha);
        const float ClimbVerticalTarget = ClimbPotential;

        float VerticalTargetSpeed = NeutralVerticalTarget;
        if (DiveInputAlpha > 0.05f)
        {
            VerticalTargetSpeed = FMath::Lerp(NeutralVerticalTarget, DiveVerticalTarget, DiveInputAlpha);
        }
        else if (ClimbInputAlpha > 0.05f)
        {
            VerticalTargetSpeed = FMath::Lerp(NeutralVerticalTarget, ClimbVerticalTarget, ClimbInputAlpha);
        }

        const float VerticalInterp = (VerticalTargetSpeed < Velocity.Z) ? 5.2f : 3.1f;
        Velocity.Z = FMath::FInterpTo(Velocity.Z, VerticalTargetSpeed, DeltaTime, VerticalInterp);
        Velocity.Z = FMath::Clamp(Velocity.Z, -MaxDiveSinkSpeed * 1.15f, MaxClimbSpeed);

        if (ClimbInputAlpha > 0.05f && Velocity.Z > 0.f)
        {
            const float EnergyCost = Velocity.Z * ClimbInputAlpha * ClimbSpeedCostMultiplier * 0.2f * DeltaTime;
            CurrentSpeed = FMath::Clamp(CurrentSpeed - EnergyCost, MinDiveSpeed, MaxDiveSpeed);
        }

        Velocity.X = ForwardDir.X * CurrentSpeed;
        Velocity.Y = ForwardDir.Y * CurrentSpeed;
    }

    Move->Velocity = Velocity;

    const FColor DebugColor = (Velocity.Z > 50.f) ? FColor::Green :
                              (Velocity.Z < -50.f) ? FColor::Red : FColor::White;
    DIVE_SCREEN(1, DebugColor,
        TEXT("[Dive] Alt %.0f | VZ %.0f | Speed %.0f | Input %.1f/%.1f %s"),
        Owner->GetActorLocation().Z,
        Velocity.Z,
        CurrentSpeed,
        HorizontalInput,
        VerticalInput,
        bHasWind ? TEXT("| WIND STREAM") : TEXT(""));
    DIVE_LOG(Verbose, TEXT("[Dive] Tick | Speed=%.1f VZ=%.1f Input=(%.2f,%.2f) Pitch=%.1f Roll=%.1f Wind=%s"),
        CurrentSpeed,
        Velocity.Z,
        HorizontalInput,
        VerticalInput,
        NewRot.Pitch,
        NewRot.Roll,
        bHasWind ? TEXT("true") : TEXT("false"));

    bWindStreamActive = false;
    PendingWindTargetSpeed = 0.f;
    PendingWindAlignmentStrength = 0.f;
    PendingWindDirection = FVector::ZeroVector;
    PendingWindCenteringAccel = FVector::ZeroVector;
}
