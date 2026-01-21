/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimBlueprintLibrary" - Source
 */

#include "Components/Characters/Player/Movements/Swim/SwimBlueprintLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"

bool USwimBlueprintLibrary::IsCharacterInWaterVolume(const ACharacter* Character)
{
	if (!Character)
	{
		return false;
	}

	const APhysicsVolume* PV = Character->GetPhysicsVolume();
	return PV && PV->bWaterVolume;
}

bool USwimBlueprintLibrary::GetCharacterFloorDistance(const ACharacter* Character, float TraceDepth, TEnumAsByte<ECollisionChannel> TraceChannel, float& OutFloorDistance)
{
	OutFloorDistance = TNumericLimits<float>::Max();

	if (!Character || !Character->GetWorld())
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const FVector ActorLoc = Character->GetActorLocation();
	const FVector Bottom = ActorLoc - FVector(0.f, 0.f, HalfHeight);

	const FVector Start = Bottom + FVector(0.f, 0.f, 10.f);
	const FVector End = Start - FVector(0.f, 0.f, FMath::Max(TraceDepth, 10.f));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SwimLibFloorTrace), false, Character);
	FHitResult Hit;

	const bool bHit = Character->GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(Radius * 0.75f),
		Params
	);

	if (!bHit)
	{
		return false;
	}

	OutFloorDistance = FMath::Max(0.f, Bottom.Z - Hit.ImpactPoint.Z);
	return true;
}

FVector USwimBlueprintLibrary::GetSwimForwardDirection(const ACharacter* Character, bool bUsePitch)
{
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	const AController* Controller = Character->GetController();
	if (!Controller)
	{
		return Character->GetActorForwardVector();
	}

	const FRotator ControlRot = Controller->GetControlRotation();
	if (bUsePitch)
	{
		return ControlRot.Vector();
	}

	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	return FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
}

FVector USwimBlueprintLibrary::GetSwimRightDirection(const ACharacter* Character)
{
	if (!Character)
	{
		return FVector::RightVector;
	}

	const AController* Controller = Character->GetController();
	if (!Controller)
	{
		return Character->GetActorRightVector();
	}

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	return FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
}

void USwimBlueprintLibrary::ApplySwimMovementInput(ACharacter* Character, float ForwardAxis, float RightAxis, bool bUsePitchForForward)
{
	if (!Character)
	{
		return;
	}

	if (FMath::IsNearlyZero(ForwardAxis) && FMath::IsNearlyZero(RightAxis))
	{
		return;
	}

	const FVector ForwardDir = GetSwimForwardDirection(Character, bUsePitchForForward);
	const FVector RightDir = GetSwimRightDirection(Character);

	Character->AddMovementInput(ForwardDir, ForwardAxis);
	Character->AddMovementInput(RightDir, RightAxis);
}

FName USwimBlueprintLibrary::SwimModeToName(EPlayerSwimMode Mode)
{
	switch (Mode)
	{
	case EPlayerSwimMode::None:       return FName("None");
	case EPlayerSwimMode::Surface:    return FName("Surface");
	case EPlayerSwimMode::Underwater: return FName("Underwater");
	default:                          return FName("Unknown");
	}
}
