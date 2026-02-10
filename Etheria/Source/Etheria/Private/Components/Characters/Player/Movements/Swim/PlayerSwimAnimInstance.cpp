/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerSwimAnimInstance" - Source
 */

#include "Components/Characters/Player/Movements/Swim/PlayerSwimAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"

#if __has_include("KismetAnimationLibrary.h")
	#include "KismetAnimationLibrary.h"
	#define ETHERIA_HAS_KISMET_ANIM_LIB 1
#else
	#define ETHERIA_HAS_KISMET_ANIM_LIB 0
#endif

#include "Components/Characters/Player/Movements/Swim/SwimComponent.h"
#include "Components/Characters/Player/Movements/Swim/SwimAnimationSet.h"

UPlayerSwimAnimInstance::UPlayerSwimAnimInstance()
{
}

void UPlayerSwimAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
		SwimComponent = OwnerCharacter->FindComponentByClass<USwimComponent>();
	}
}

void UPlayerSwimAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
		if (OwnerCharacter)
		{
			MovementComponent = OwnerCharacter->GetCharacterMovement();
			SwimComponent = OwnerCharacter->FindComponentByClass<USwimComponent>();
		}
	}

	if (!OwnerCharacter || !MovementComponent)
	{
		return;
	}

	const FVector Vel = OwnerCharacter->GetVelocity();
	SwimSpeed = Vel.Size2D();

#if ETHERIA_HAS_KISMET_ANIM_LIB
	SwimDirection = UKismetAnimationLibrary::CalculateDirection(Vel, OwnerCharacter->GetActorRotation());
#else
	// Fallback without extra module dependency (may be deprecated in future engine versions).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SwimDirection = CalculateDirection(Vel, OwnerCharacter->GetActorRotation());
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif

	// Smooth values for more dynamic feeling (less snappy)
	SmoothedSpeed = FMath::FInterpTo(SmoothedSpeed, SwimSpeed, DeltaSeconds, 8.f);
	SmoothedDirection = FMath::FInterpTo(SmoothedDirection, SwimDirection, DeltaSeconds, 10.f);

	bIsSwimming = (MovementComponent->MovementMode == MOVE_Swimming);

	if (SwimComponent)
	{
		bInWater = SwimComponent->IsInWater();
		bIsUnderwater = SwimComponent->IsUnderwater();
	}
	else
	{
		bInWater = false;
		bIsUnderwater = false;
	}

	UpdateAutoMontage(DeltaSeconds);
}

UAnimMontage* UPlayerSwimAnimInstance::SelectDirectionalLoopMontage(const USwimAnimationSet* AnimSet) const
{
	if (!AnimSet || !SwimComponent)
	{
		return nullptr;
	}

	const bool bUnder = SwimComponent->IsUnderwater();
	const bool bSprint = SwimComponent->IsSwimSprinting();

	// Direction selection based on velocity in local space (2D).
	const FVector WorldVel = OwnerCharacter ? OwnerCharacter->GetVelocity() : FVector::ZeroVector;
	const FRotator YawRot(0.f, OwnerCharacter ? OwnerCharacter->GetActorRotation().Yaw : 0.f, 0.f);
	const FVector Local = FRotationMatrix(YawRot).InverseTransformVector(WorldVel);
	const float Fwd = Local.X;
	const float Right = Local.Y;

	const float AbsF = FMath::Abs(Fwd);
	const float AbsR = FMath::Abs(Right);

	auto Pick = [&](UAnimMontage* Idle, UAnimMontage* F, UAnimMontage* B, UAnimMontage* L, UAnimMontage* R,
		UAnimMontage* SF, UAnimMontage* SB, UAnimMontage* SL, UAnimMontage* SR) -> UAnimMontage*
	{
		if (SmoothedSpeed <= IdleSpeedThreshold)
		{
			return Idle;
		}

		const bool bForwardDominant = (AbsF >= AbsR);

		if (bForwardDominant)
		{
			if (Fwd >= 0.f)
			{
				return bSprint ? (SF ? SF : F) : F;
			}
			return bSprint ? (SB ? SB : B) : B;
		}

		if (Right >= 0.f)
		{
			return bSprint ? (SR ? SR : R) : R;
		}
		return bSprint ? (SL ? SL : L) : L;
	};

	if (!bUnder)
	{
		return Pick(
			AnimSet->SurfaceIdleMontage,
			AnimSet->SurfaceForwardMontage,
			AnimSet->SurfaceBackwardMontage,
			AnimSet->SurfaceLeftMontage,
			AnimSet->SurfaceRightMontage,
			AnimSet->SurfaceSprintForwardMontage,
			AnimSet->SurfaceSprintBackwardMontage,
			AnimSet->SurfaceSprintLeftMontage,
			AnimSet->SurfaceSprintRightMontage
		);
	}

	return Pick(
		AnimSet->UnderwaterIdleMontage,
		AnimSet->UnderwaterForwardMontage,
		AnimSet->UnderwaterBackwardMontage,
		AnimSet->UnderwaterLeftMontage,
		AnimSet->UnderwaterRightMontage,
		AnimSet->UnderwaterSprintForwardMontage,
		AnimSet->UnderwaterSprintBackwardMontage,
		AnimSet->UnderwaterSprintLeftMontage,
		AnimSet->UnderwaterSprintRightMontage
	);
}

void UPlayerSwimAnimInstance::UpdateAutoMontage(float DeltaSeconds)
{
	if (!bAutoPlayDirectionalMontages || !bIsSwimming)
	{
		return;
	}

	USwimAnimationSet* AnimSet = AnimationSetOverride;
	if (!AnimSet && SwimComponent)
	{
		AnimSet = SwimComponent->GetAnimationSet();
	}
	if (!AnimSet)
	{
		return;
	}

	// If BlendSpaces are used, we don't auto-play directional montages.
	if ((!SwimComponent->IsUnderwater() && AnimSet->SurfaceBlendSpace) || (SwimComponent->IsUnderwater() && AnimSet->UnderwaterBlendSpace))
	{
		return;
	}

	UAnimMontage* Desired = SelectDirectionalLoopMontage(AnimSet);
	if (!Desired)
	{
		return;
	}

	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Time - LastMontageSwitchTime < MinTimeBetweenMontageSwitch)
	{
		return;
	}

	UAnimMontage* Current = GetCurrentActiveMontage();
	if (Current == Desired && Montage_IsPlaying(Desired))
	{
		return;
	}

	if (Current && Montage_IsPlaying(Current))
	{
		Montage_Stop(MontageBlendOutTime, Current);
	}

	Montage_Play(Desired, 1.f);
	LastMontageSwitchTime = Time;
}
