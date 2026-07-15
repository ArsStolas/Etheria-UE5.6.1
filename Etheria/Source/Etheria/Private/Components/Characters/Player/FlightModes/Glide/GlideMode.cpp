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
	float InMinimumHeight,
	float InWindStreamAcceleration,
	float InWindEscapeInputThreshold,
	float InWindEscapeHoldTime)
{
	GlideSpeed = InGlideSpeed;
	DescendRate = InDescendRate;
	GlideInterp = InGlideInterp;
	DescentInterp = InDescentInterp;
	MinimumHeight = InMinimumHeight;
	WindStreamAcceleration = InWindStreamAcceleration;
	WindEscapeInputThreshold = InWindEscapeInputThreshold;
	WindEscapeHoldTime = InWindEscapeHoldTime;
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

	WindRideSpeed = 0.f;
	WindEscapeTime = 0.f;
	ResetWindStreamState();
}

void UGlideMode::Exit()
{
	if (!Owner || !Move) return;

	RestoreMovementSettings();

	WindRideSpeed = 0.f;
	WindEscapeTime = 0.f;
	ResetWindStreamState();
}

bool UGlideMode::CanStartGliding() const
{
	if (!Owner || !Owner->GetCharacterMovement()->IsFalling()) return false;

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

void UGlideMode::ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
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

float UGlideMode::GetCurrentSpeed() const
{
	const float VelocitySpeed = Move ? static_cast<float>(Move->Velocity.Size2D()) : 0.f;
	return FMath::Max(WindRideSpeed, VelocitySpeed);
}

void UGlideMode::ResetWindStreamState()
{
	bWindStreamActive = false;
	PendingWindTargetSpeed = 0.f;
	PendingWindAlignmentStrength = 0.f;
	PendingWindDirection = FVector::ZeroVector;
	PendingWindCenteringAccel = FVector::ZeroVector;
}

void UGlideMode::TickMode(float DeltaTime)
{
	if (!Owner || !Move) return;

	const float Hor = Owner->GetHorizontalInput();
	const float Ver = Owner->GetVerticalInput();

	const FRotator CamRot(0.f, Owner->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

	const FVector InputDir = (Forward * Ver + Right * Hor).GetSafeNormal();

	const bool bHasWind = bWindStreamActive
		&& PendingWindTargetSpeed > 0.f
		&& !PendingWindDirection.IsNearlyZero();

	if (bHasWind)
	{
		TickWindStream(DeltaTime, InputDir);
	}
	else
	{
		WindRideSpeed = 0.f;
		WindEscapeTime = 0.f;

		FVector Vel = Move->Velocity;

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
			FVector CurrentHorizontal(Vel.X, Vel.Y, 0.f);
			FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, FVector::ZeroVector, DeltaTime, 1.5f);
			Vel.X = NewHorizontal.X;
			Vel.Y = NewHorizontal.Y;
		}

		Vel.Z = FMath::FInterpTo(Vel.Z, -DescendRate, DeltaTime, DescentInterp);

		Move->Velocity = Vel;
	}

	ResetWindStreamState();
}

void UGlideMode::TickWindStream(float DeltaTime, const FVector& InputDir)
{
	const FVector Velocity = Move->Velocity;

	FVector StreamHorizontal(PendingWindDirection.X, PendingWindDirection.Y, 0.f);
	StreamHorizontal = StreamHorizontal.GetSafeNormal();

	float LateralInputStrength = 0.f;
	FVector LateralInputDir = FVector::ZeroVector;
	if (!InputDir.IsNearlyZero())
	{
		if (StreamHorizontal.IsNearlyZero())
		{
			LateralInputStrength = 1.f;
			LateralInputDir = InputDir;
		}
		else
		{
			const float ForwardAmount = FVector::DotProduct(InputDir, StreamHorizontal);
			const FVector LateralInput = InputDir - StreamHorizontal * ForwardAmount;
			LateralInputStrength = static_cast<float>(LateralInput.Size());
			LateralInputDir = LateralInput.GetSafeNormal();
		}
	}

	const bool bEscaping = LateralInputStrength >= WindEscapeInputThreshold;
	if (bEscaping)
	{
		WindEscapeTime += DeltaTime;
	}
	else
	{
		WindEscapeTime = FMath::Max(0.f, WindEscapeTime - DeltaTime * 2.f);
	}

	const float EscapeAlpha = WindEscapeHoldTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(WindEscapeTime / WindEscapeHoldTime, 0.f, 1.f)
		: 1.f;
	const float EscapeStrength = bEscaping ? EscapeAlpha * LateralInputStrength : 0.f;

	WindRideSpeed = FMath::Max(WindRideSpeed, static_cast<float>(Velocity.Size2D()));
	WindRideSpeed = FMath::FInterpConstantTo(
		WindRideSpeed,
		FMath::Max(WindRideSpeed, PendingWindTargetSpeed),
		DeltaTime,
		WindStreamAcceleration * FMath::Lerp(1.f, 0.78f, EscapeStrength));
	WindRideSpeed = FMath::Min(WindRideSpeed, FMath::Max(GlideSpeed * 1.5f, PendingWindTargetSpeed * 1.15f));

	FVector CurrentDirection = Velocity.GetSafeNormal();
	if (CurrentDirection.IsNearlyZero())
	{
		CurrentDirection = PendingWindDirection;
	}

	const FVector SteeredDirection = EscapeStrength > 0.f
		? FMath::Lerp(CurrentDirection, LateralInputDir, 0.45f * EscapeStrength).GetSafeNormal()
		: CurrentDirection;

	const float FullGuidance = FMath::Clamp(PendingWindAlignmentStrength * 0.7f, 0.f, 0.92f);
	const float EscapeGuidance = FMath::Clamp(PendingWindAlignmentStrength * 0.18f, 0.f, 0.45f);
	const float GuidanceBlend = FMath::Lerp(FullGuidance, EscapeGuidance, EscapeStrength);

	FVector DesiredVelocity = FMath::Lerp(
		SteeredDirection * WindRideSpeed,
		PendingWindDirection * WindRideSpeed,
		GuidanceBlend);
	DesiredVelocity += PendingWindCenteringAccel * DeltaTime;

	if (DesiredVelocity.IsNearlyZero())
	{
		DesiredVelocity = PendingWindDirection * WindRideSpeed;
	}

	const float StreamForwardAssist = FMath::Clamp(PendingWindAlignmentStrength, 0.f, 1.f);
	const float MinForwardSpeed = WindRideSpeed * FMath::Lerp(0.9f, 0.72f, EscapeStrength) * StreamForwardAssist;
	const float ForwardAlongStream = static_cast<float>(FVector::DotProduct(DesiredVelocity, PendingWindDirection));
	if (StreamForwardAssist > KINDA_SMALL_NUMBER && ForwardAlongStream < MinForwardSpeed)
	{
		DesiredVelocity += PendingWindDirection * (MinForwardSpeed - ForwardAlongStream);
	}

	Move->Velocity = DesiredVelocity.GetClampedToMaxSize(WindRideSpeed * FMath::Lerp(1.02f, 1.06f, EscapeStrength));

	const FVector HorizontalDirection(Move->Velocity.X, Move->Velocity.Y, 0.f);
	if (!HorizontalDirection.IsNearlyZero())
	{
		const FRotator NewRotation = FMath::RInterpTo(
			Owner->GetActorRotation(),
			HorizontalDirection.Rotation(),
			DeltaTime,
			4.f);
		Owner->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
	}
}
