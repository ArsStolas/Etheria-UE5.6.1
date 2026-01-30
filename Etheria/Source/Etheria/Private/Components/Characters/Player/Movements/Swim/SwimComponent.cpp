/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimComponent" - Source
 */

#include "Components/Characters/Player/Movements/Swim/SwimComponent.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"

USwimComponent::USwimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void USwimComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter.IsValid())
	{
		MoveComp = OwnerCharacter->GetCharacterMovement();
		if (MoveComp)
		{
			// Swimming is driven by MovementMode (MOVE_Swimming) + WaterVolume (bWaterVolume).
			MoveComp->NavAgentProps.bCanSwim = true; // optional, mainly for navmesh/AI
		}
	}

	bInWater = IsOwnerInWaterVolume();
	OnInWaterChanged.Broadcast(bInWater);
}

void USwimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bAutoSwim || !OwnerCharacter.IsValid() || !MoveComp || !GetWorld())
	{
		return;
	}

#pragma region WaterState
	const bool bNowInWater = IsOwnerInWaterVolume();
	if (bNowInWater != bInWater)
	{
		bInWater = bNowInWater;
		OnInWaterChanged.Broadcast(bInWater);

		if (!bInWater)
		{
			StopSwimming();
			return;
		}
	}
	if (!bInWater)
	{
		return;
	}
#pragma endregion

#pragma region FloorChecks
	const float Time = GetWorld()->GetTimeSeconds();
	if (Time >= NextFloorCheckTime)
	{
		NextFloorCheckTime = Time + FloorCheckInterval;

		float FloorDist = 0.f;
		const bool bHasFloor = FindWalkableFloor(FloorDist);

		const bool bUnderwater = (CurrentMode == EPlayerSwimMode::Underwater);
		const bool bShouldWalkInWater =
			!bWantsDive &&
			!bUnderwater &&
			bHasFloor &&
			FloorDist <= MaxFloorDistanceToStayWalking;

		// Shallow water => walking (unless diving)
		if (bShouldWalkInWater)
		{
			if (CurrentMode != EPlayerSwimMode::None)
			{
				SetMode(EPlayerSwimMode::None);
			}
			if (MoveComp->MovementMode == MOVE_Swimming)
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
		else
		{
			// Need swim
			if (MoveComp->MovementMode != MOVE_Swimming)
			{
				MoveComp->SetMovementMode(MOVE_Swimming);
			}

			const EPlayerSwimMode Desired = bWantsDive ? EPlayerSwimMode::Underwater : EPlayerSwimMode::Surface;
			if (CurrentMode != Desired)
			{
				SetMode(Desired);
			}

			// Exit swim to walking if we can stand and we are not underwater
			if (!bWantsDive && CurrentMode == EPlayerSwimMode::Surface && bHasFloor && FloorDist <= MaxFloorDistanceToExitSwim)
			{
				SetMode(EPlayerSwimMode::None);
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
	}
#pragma endregion

	ApplyMovementTuning();

#pragma region AutoApplyInput
	if (bAutoApplyCachedInput && CurrentMode != EPlayerSwimMode::None)
	{
		ApplyCachedSwimMovementInput();
	}
#pragma endregion
}

void USwimComponent::Input_SetMoveAxis(float ForwardAxis, float RightAxis)
{
	CachedForwardAxis = ForwardAxis;
	CachedRightAxis = RightAxis;
}

void USwimComponent::Input_DivePressed()
{
	if (!bInWater || !MoveComp)
	{
		return;
	}

	if (bDiveToggle)
	{
		bWantsDive = !bWantsDive;
	}
	else
	{
		bWantsDive = true;
	}

	// If player requests dive while still walking, force swimming
	if (MoveComp->MovementMode != MOVE_Swimming)
	{
		MoveComp->SetMovementMode(MOVE_Swimming);
	}

	SetMode(EPlayerSwimMode::Underwater);
	ApplyMovementTuning();
}

void USwimComponent::Input_DiveReleased()
{
	if (!bDiveToggle)
	{
		bWantsDive = false;

		if (bInWater)
		{
			SetMode(EPlayerSwimMode::Surface);
			ApplyMovementTuning();
		}
	}
}

void USwimComponent::SetSwimSprinting(bool bSprint)
{
	bSwimSprinting = bSprint;
	ApplyMovementTuning();
}

void USwimComponent::ForceSwimMode(EPlayerSwimMode NewMode)
{
	if (!OwnerCharacter.IsValid() || !MoveComp)
	{
		return;
	}

	if (NewMode == EPlayerSwimMode::None)
	{
		StopSwimming();
		return;
	}

	MoveComp->SetMovementMode(MOVE_Swimming);
	SetMode(NewMode);

	bWantsDive = (NewMode == EPlayerSwimMode::Underwater);
	ApplyMovementTuning();
}

void USwimComponent::StopSwimming()
{
	if (MoveComp && MoveComp->MovementMode == MOVE_Swimming)
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	bWantsDive = false;
	bSwimSprinting = false;
	CachedForwardAxis = 0.f;
	CachedRightAxis = 0.f;

	SetMode(EPlayerSwimMode::None);
}

void USwimComponent::ApplyCachedSwimMovementInput()
{
	if (CurrentMode == EPlayerSwimMode::None)
	{
		return;
	}

	ApplySwimMovementInput(CachedForwardAxis, CachedRightAxis);
}

bool USwimComponent::IsOwnerInWaterVolume() const
{
	if (!OwnerCharacter.IsValid())
	{
		return false;
	}

	const APhysicsVolume* PV = OwnerCharacter->GetPhysicsVolume();
	return PV && PV->bWaterVolume;
}

bool USwimComponent::FindWalkableFloor(float& OutFloorDist) const
{
	OutFloorDist = TNumericLimits<float>::Max();

	if (!OwnerCharacter.IsValid() || !GetWorld())
	{
		return false;
	}

	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const FVector ActorLoc = OwnerCharacter->GetActorLocation();
	const FVector Bottom = ActorLoc - FVector(0.f, 0.f, HalfHeight);

	const FVector Start = Bottom + FVector(0.f, 0.f, 10.f);
	const FVector End = Start - FVector(0.f, 0.f, FloorTraceDepth);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SwimFloorTrace), false, OwnerCharacter.Get());
	FHitResult Hit;

	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		FloorTraceChannel,
		FCollisionShape::MakeSphere(Radius * 0.75f),
		Params
	);

	if (!bHit)
	{
		return false;
	}

	// Walkable check (uses CharacterMovement slope rules)
	if (MoveComp && !MoveComp->IsWalkable(Hit))
	{
		return false;
	}

	OutFloorDist = FMath::Max(0.f, Bottom.Z - Hit.ImpactPoint.Z);
	return true;
}

void USwimComponent::SetMode(EPlayerSwimMode NewMode)
{
	if (NewMode == CurrentMode)
	{
		return;
	}

	const EPlayerSwimMode Prev = CurrentMode;
	CurrentMode = NewMode;

	OnSwimModeChanged.Broadcast(Prev, CurrentMode);
}

void USwimComponent::ApplyMovementTuning() const
{
	if (!MoveComp || CurrentMode == EPlayerSwimMode::None)
	{
		return;
	}

	const bool bUnderwater = (CurrentMode == EPlayerSwimMode::Underwater);
	const float TargetSpeed =
		bUnderwater
			? (bSwimSprinting ? UnderwaterSprintSpeed : UnderwaterSpeed)
			: (bSwimSprinting ? SurfaceSprintSpeed : SurfaceSpeed);

	MoveComp->MaxSwimSpeed = TargetSpeed;
	MoveComp->MaxAcceleration = SwimAcceleration;
	MoveComp->BrakingDecelerationSwimming = BrakingDecelSwimming;
	MoveComp->Buoyancy = bUnderwater ? UnderwaterBuoyancy : SurfaceBuoyancy;
}

void USwimComponent::ApplySwimMovementInput(float ForwardAxis, float RightAxis) const
{
	if (!OwnerCharacter.IsValid() || !MoveComp)
	{
		return;
	}

	if (FMath::IsNearlyZero(ForwardAxis) && FMath::IsNearlyZero(RightAxis))
	{
		return;
	}

	const AController* Controller = OwnerCharacter->GetController();
	if (!Controller)
	{
		return;
	}

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	// Forward direction:
	// - Surface: yaw only
	// - Underwater: optionally full pitch
	FVector ForwardDir;
	if (CurrentMode == EPlayerSwimMode::Underwater && bUseCameraPitchUnderwater)
	{
		ForwardDir = ControlRot.Vector();
	}
	else
	{
		ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	}

	const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	OwnerCharacter->AddMovementInput(ForwardDir, ForwardAxis);
	OwnerCharacter->AddMovementInput(RightDir, RightAxis);
}
