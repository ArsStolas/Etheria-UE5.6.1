/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GlideMode - Source
*/

#include "Components/Characters/Player/FlightModes/Glide/GlideMode.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGlideMode::Enter()
{
	StoreMovementSettings();

	Move->GravityScale = 0.f;
	Move->AirControl = 1.f;
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(true);
}

void UGlideMode::Exit()
{
	RestoreMovementSettings();

	if (Owner->GetGliderVisual())
		Owner->GetGliderVisual()->SetVisibility(false);
}

void UGlideMode::TickMode(float DeltaTime)
{
	int Hor = Owner->GetHorizontalAxis();
	int Ver = Owner->GetVerticalAxis();

	const FRotator CamRot(0.f, Owner->GetControlRotation().Yaw, 0.f);
	FVector Forward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
	FVector Right   = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

	FVector InputDir = (Forward * Ver + Right * Hor).GetSafeNormal();

	FVector Vel = Move->Velocity;

	// Horizontal gliding
	if (!InputDir.IsNearlyZero())
	{
		FVector TargetHorizontal = InputDir * GlideSpeed;
		FVector CurrentHorizontal(Vel.X, Vel.Y, 0.f);
		FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, TargetHorizontal, DeltaTime, GlideInterp);

		Vel.X = NewHorizontal.X;
		Vel.Y = NewHorizontal.Y;

		Owner->SetActorRotation(InputDir.Rotation());
	}

	// Vertical descent
	Vel.Z = FMath::FInterpTo(Vel.Z, -DescendRate, DeltaTime, DescentInterp);

	Move->Velocity = Vel;
}
