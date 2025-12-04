/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: DiveMode - Source
*/

#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(false);
}

void UDiveMode::Exit()
{
	if (!Owner || !Move) return;

	// Réinitialiser rotation
	FRotator CurrentRot = Owner->GetActorRotation();
	Owner->SetActorRotation(FRotator(0.f, CurrentRot.Yaw, 0.f));

	RestoreMovementSettings();

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(false);

	CurrentSpeed = MinDiveSpeed;
}

void UDiveMode::TickMode(float DeltaTime)
{
	if (!Owner) return;

    // Lire les axes
    const int Hor = Owner->GetHorizontalAxis();
    const int Ver = Owner->GetVerticalAxis();

    // === PITCH / ROLL ===
    float TargetPitch = FMath::Clamp(-Ver * MaxPitch, -MaxPitch, MaxPitch);
    float TargetRoll  = FMath::Clamp(Hor * MaxRoll, -MaxRoll, MaxRoll);

    // Interpolation de la rotation
    FRotator CurrentRot = Owner->GetActorRotation();
    FRotator TargetRot  = FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll);
    FRotator NewRot     = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 3.f);
    Owner->SetActorRotation(NewRot);

    // === Gestion de la vitesse scalaire ===
    float PitchFactor = NewRot.Pitch / MaxPitch;

    if (PitchFactor < -0.1f) // pique → accélère
        CurrentSpeed += DiveAcceleration * (1.0f + 0.2f * FMath::Abs(PitchFactor)) * DeltaTime;
    else if (PitchFactor > 0.1f) // cabré → perd de la vitesse
        CurrentSpeed -= DiveDeceleration * (0.6f + 0.4f * PitchFactor) * DeltaTime;
    else // à plat → légère perte
        CurrentSpeed -= DiveDeceleration * 0.15f * DeltaTime;

    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxDiveSpeed * 1.3f);

    // === Yaw influencé par le Roll ===
    float RollFactor = NewRot.Roll / MaxRoll;
    if (FMath::Abs(RollFactor) > 0.1f)
    {
        float SpeedScale = FMath::Clamp(CurrentSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        float TurnRate   = RollFactor * TurnRateDive * SpeedScale;

        FRotator YawRot = Owner->GetActorRotation();
        YawRot.Yaw += TurnRate * DeltaTime;
        Owner->SetActorRotation(FRotator(NewRot.Pitch, YawRot.Yaw, NewRot.Roll));
        NewRot = Owner->GetActorRotation();
    }

    // === Direction horizontale ===
    FRotator HorizontalRot(0.f, NewRot.Yaw, 0.f);
    FVector ForwardDir = FRotationMatrix(HorizontalRot).GetUnitAxis(EAxis::X);

    FVector Velocity = Move->Velocity;
    Velocity.X = ForwardDir.X * CurrentSpeed;
    Velocity.Y = ForwardDir.Y * CurrentSpeed;

    // === Gravité arcade + portance ===
    const float GravityStrength = 400.f;        // gravité réduite pour arcade
    const float StallSpeed      = MaxDiveSpeed * 0.4f; // seuil de décrochage
    const float MaxLiftCoeff    = LiftFactor * 1.8f;   // portance boostée

    float LiftCoeff = 0.f;
    if (CurrentSpeed > StallSpeed)
    {
        float SpeedRatio = (CurrentSpeed - StallSpeed) / (MaxDiveSpeed - StallSpeed);
        SpeedRatio = FMath::Clamp(SpeedRatio, 0.f, 1.f);
        LiftCoeff = MaxLiftCoeff * SpeedRatio;
    }

    float LiftAccelZ = 0.f;
    if (PitchFactor > 0.f && LiftCoeff > 0.f)
    {
        float PitchGain = FMath::Clamp(PitchFactor, 0.f, 1.f);
        LiftAccelZ = LiftCoeff * PitchGain * (CurrentSpeed * 0.5f);
    }

    // Accélération verticale finale
    float AccelZ = LiftAccelZ - GravityStrength;
    Velocity.Z += AccelZ * DeltaTime;

    // Clamp pour éviter les abus
    Velocity.Z = FMath::Clamp(Velocity.Z, -2400.f, 900.f);

    Move->Velocity = Velocity;

    // === Debug ===
    if (GEngine)
    {
        FString Msg = FString::Printf(
            TEXT("Alt: %.0f | VZ: %.0f | Speed: %.0f | Stall: %.0f | LiftCoeff: %.2f | Pitch: %.1f°"),
            Owner->GetActorLocation().Z,
            Velocity.Z,
            CurrentSpeed,
            StallSpeed,
            LiftCoeff,
            NewRot.Pitch
        );

        FColor DebugColor = (Velocity.Z > 50.f) ? FColor::Green :
                            (Velocity.Z < -50.f) ? FColor::Red : FColor::White;

        GEngine->AddOnScreenDebugMessage(1, 0.f, DebugColor, Msg);
    }
}
