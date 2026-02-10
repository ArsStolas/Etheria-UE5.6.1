/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimBlueprintLibrary" - Source
 */

#include "Components/Characters/Player/Movements/Swim/SwimBlueprintLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"

USwimComponent* USwimBlueprintLibrary::GetSwimComponentFromActor(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<USwimComponent>();
}

USwimComponent* USwimBlueprintLibrary::GetSwimComponentFromCharacter(const ACharacter* Character)
{
	return GetSwimComponentFromActor(Character);
}

bool USwimBlueprintLibrary::IsCharacterSwimmingMovementMode(const ACharacter* Character)
{
	if (!Character)
	{
		return false;
	}

	const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	return MoveComp && MoveComp->MovementMode == MOVE_Swimming;
}

bool USwimBlueprintLibrary::IsCharacterSwimmingActive(const ACharacter* Character)
{
	const USwimComponent* SwimComp = GetSwimComponentFromCharacter(Character);
	return SwimComp ? SwimComp->IsSwimmingActive() : false;
}

bool USwimBlueprintLibrary::IsCharacterUnderwater(const ACharacter* Character)
{
	const USwimComponent* SwimComp = GetSwimComponentFromCharacter(Character);
	return SwimComp ? SwimComp->IsUnderwater() : false;
}

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

	const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const FVector ActorLoc = Character->GetActorLocation();
	const FVector Bottom = ActorLoc - FVector(0.f, 0.f, HalfHeight);

	const FVector Start = Bottom + FVector(0.f, 0.f, 10.f);
	const FVector End = Start - FVector(0.f, 0.f, FMath::Max(TraceDepth, 10.f));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SwimHelperFloorTrace), false, Character);
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

	if (MoveComp && !MoveComp->IsWalkable(Hit))
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
		return FVector::ForwardVector;
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
		return FVector::RightVector;
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

	const FVector ForwardDir = GetSwimForwardDirection(Character, bUsePitchForForward);
	const FVector RightDir = GetSwimRightDirection(Character);

	if (!FMath::IsNearlyZero(ForwardAxis))
	{
		Character->AddMovementInput(ForwardDir, ForwardAxis);
	}
	if (!FMath::IsNearlyZero(RightAxis))
	{
		Character->AddMovementInput(RightDir, RightAxis);
	}
}

FName USwimBlueprintLibrary::SwimModeToName(EPlayerSwimMode Mode)
{
	switch (Mode)
	{
	case EPlayerSwimMode::None:       return TEXT("None");
	case EPlayerSwimMode::Surface:    return TEXT("Surface");
	case EPlayerSwimMode::Underwater: return TEXT("Underwater");
	default:                          return TEXT("Unknown");
	}
}

FString USwimBlueprintLibrary::GetSwimDebugString(const ACharacter* Character)
{
	const USwimComponent* SwimComp = GetSwimComponentFromCharacter(Character);
	if (!SwimComp)
	{
		const bool bInWater = IsCharacterInWaterVolume(Character);
		const bool bModeSwim = IsCharacterSwimmingMovementMode(Character);
		return FString::Printf(TEXT("SwimComp=None | InWater=%s | MoveModeSwim=%s"),
			bInWater ? TEXT("true") : TEXT("false"),
			bModeSwim ? TEXT("true") : TEXT("false"));
	}

	const FString ModeStr = SwimModeToName(SwimComp->GetSwimMode()).ToString();
	return FString::Printf(TEXT("InWater=%s | Mode=%s | Sprint=%s"),
		SwimComp->IsInWater() ? TEXT("true") : TEXT("false"),
		*ModeStr,
		SwimComp->IsSwimSprinting() ? TEXT("true") : TEXT("false"));
}
