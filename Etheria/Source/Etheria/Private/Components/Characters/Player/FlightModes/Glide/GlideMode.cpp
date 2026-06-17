/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
 * Class: GlideMode - Source
*/

#include "Components/Characters/Player/FlightModes/Glide/GlideMode.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Environment/WindColumn.h"
#include "World/Environment/WindStreamZone.h"

void UGlideMode::ConfigureGlideTuning(
	float InGlideSpeed,
	float InDescendRate,
	float InGlideInterp,
	float InDescentInterp,
	float InMinimumHeight)
{
	GlideSpeed = InGlideSpeed;
	DescendRate = InDescendRate;
	GlideInterp = InGlideInterp;
	DescentInterp = InDescentInterp;
	MinimumHeight = InMinimumHeight;
}

void UGlideMode::Enter()
{
	if (!Owner || !Move) return;

	StoreMovementSettings();

	Move->GravityScale = 0.f;
	Move->AirControl = 0.9f;
	Move->BrakingDecelerationFalling = 350.f;
	Move->MaxAcceleration = 1024.f;
	Move->MaxWalkSpeed = 640.f;
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;
	Move->RotationRate = FRotator(0.f, 250.f, 0.f);

}

void UGlideMode::Exit()
{
	if (!Owner || !Move) return;

	RestoreMovementSettings();

}

bool UGlideMode::CanStartGliding() const
{
	if (!Owner || !Owner->GetCharacterMovement()->IsFalling()) return false;

	// In a wind column / stream, gliding is allowed at any height — skip the ground-clearance check.
	if (IsInWindZone()) return true;

	FHitResult Hit;
	const FVector TraceStart = Owner->GetActorLocation();
	const FVector TraceEnd = TraceStart - Owner->GetActorUpVector() * MinimumHeight;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	const bool bGroundClose = Owner->GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	return !bGroundClose;
}

bool UGlideMode::IsInWindZone() const
{
	if (!Owner) return false;

	TArray<AActor*> Overlapping;
	Owner->GetOverlappingActors(Overlapping, AWindColumn::StaticClass());
	if (Overlapping.Num() > 0) return true;

	Owner->GetOverlappingActors(Overlapping, AWindStreamZone::StaticClass());
	return Overlapping.Num() > 0;
}

void UGlideMode::TickMode(float DeltaTime)
{
	if (!Owner || !Move) return;

	const float Hor = Owner->GetHorizontalInput();
	const float Ver = Owner->GetVerticalInput();

	const FRotator CamRot(0.f, Owner->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

	FVector InputDir = (Forward * Ver + Right * Hor).GetSafeNormal();
	FVector Vel = Move->Velocity;

	// === Horizontal ===
	if (!InputDir.IsNearlyZero())
	{
		FVector TargetHorizontal = InputDir * GlideSpeed;
		FVector CurrentHorizontal(Vel.X, Vel.Y, 0.f);
		FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, TargetHorizontal, DeltaTime, GlideInterp);

		Vel.X = NewHorizontal.X;
		Vel.Y = NewHorizontal.Y;

		FRotator TargetRotation = InputDir.Rotation();
		FRotator NewRotation = FMath::RInterpTo(Owner->GetActorRotation(), TargetRotation, DeltaTime, 4.f);
		Owner->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
	}
	else
	{
		// Décélération progressive sans input
		FVector CurrentHorizontal(Vel.X, Vel.Y, 0.f);
		FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, FVector::ZeroVector, DeltaTime, 1.5f);
		Vel.X = NewHorizontal.X;
		Vel.Y = NewHorizontal.Y;
	}

	// === Vertical (descente) ===
	Vel.Z = FMath::FInterpTo(Vel.Z, -DescendRate, DeltaTime, DescentInterp);

	Move->Velocity = Vel;
}
