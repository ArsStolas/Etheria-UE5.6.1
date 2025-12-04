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
	StoreMovementSettings();

	Move->SetMovementMode(MOVE_Flying);
	Move->GravityScale = 0.f;
	Move->AirControl = 1.f;

	CurrentSpeed = MinDiveSpeed;

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(false);
}

void UDiveMode::Exit()
{
	RestoreMovementSettings();

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(false);

	Owner->SetActorRotation(FRotator(0.f, Owner->GetActorRotation().Yaw, 0.f));
}

void UDiveMode::TickMode(float DeltaTime)
{
	int Hor = Owner->GetHorizontalAxis();
	int Ver = Owner->GetVerticalAxis();

	FRotator Rot = Owner->GetActorRotation();

	float Pitch = FMath::Clamp(-Ver * MaxPitch, -MaxPitch, MaxPitch);
	float Roll  = FMath::Clamp(Hor * MaxRoll, -MaxRoll, MaxRoll);

	Rot = FMath::RInterpTo(Rot, FRotator(Pitch, Rot.Yaw, Roll), DeltaTime, 4.f);
	Owner->SetActorRotation(Rot);

	// Accélération / décélération
	if (Pitch < -5.f) CurrentSpeed += DiveAcceleration * DeltaTime;
	if (Pitch > 5.f)  CurrentSpeed -= DiveAcceleration * 0.6f * DeltaTime;

	CurrentSpeed = FMath::Clamp(CurrentSpeed, MinDiveSpeed, MaxDiveSpeed);

	// Direction
	FVector Forward = Owner->GetActorForwardVector();
	FVector Vel = Forward * CurrentSpeed;

	// Gravité arcade
	Vel.Z -= 300.f * DeltaTime;

	Move->Velocity = Vel;

	// Yaw turning based on roll
	if (FMath::Abs(Roll) > 5.f)
	{
		float Turn = (Roll / MaxRoll) * TurnRate * DeltaTime;
		Owner->AddControllerYawInput(Turn);
	}
}
