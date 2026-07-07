/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerCameraEffectsComponent" - Source
 */

#include "Components/Characters/Player/Camera/PlayerCameraEffectsComponent.h"

#include "Camera/EtheriaCameraShakes.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Core/System/EtheriaGameplayTags.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

UPlayerCameraEffectsComponent::UPlayerCameraEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Run after character movement so offsets use this frame's final velocity.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	HitShakeClass = UEtheriaCameraShake_HitLight::StaticClass();
	CritShakeClass = UEtheriaCameraShake_HitHeavy::StaticClass();
	LandShakeClass = UEtheriaCameraShake_Land::StaticClass();
}

void UPlayerCameraEffectsComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerOwner = Cast<APlayerCharacter>(GetOwner());
	if (!PlayerOwner)
	{
		SetComponentTickEnabled(false);
		return;
	}

	Camera = PlayerOwner->GetFollowCamera();

	if (UCombatComponent* Combat = PlayerOwner->GetCombatComponent())
	{
		Combat->OnHit.AddDynamic(this, &UPlayerCameraEffectsComponent::HandleHit);
		Combat->OnHitCrit.AddDynamic(this, &UPlayerCameraEffectsComponent::HandleHitCrit);
	}

	PlayerOwner->LandedDelegate.AddDynamic(this, &UPlayerCameraEffectsComponent::HandleLanded);
}

void UPlayerCameraEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOwner)
	{
		if (UCombatComponent* Combat = PlayerOwner->GetCombatComponent())
		{
			Combat->OnHit.RemoveDynamic(this, &UPlayerCameraEffectsComponent::HandleHit);
			Combat->OnHitCrit.RemoveDynamic(this, &UPlayerCameraEffectsComponent::HandleHitCrit);
		}
		PlayerOwner->LandedDelegate.RemoveDynamic(this, &UPlayerCameraEffectsComponent::HandleLanded);
	}

	Super::EndPlay(EndPlayReason);
}

void UPlayerCameraEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEffectsEnabled || !PlayerOwner || !Camera)
	{
		return;
	}

	NoiseTime += DeltaTime;

	UpdateMovementTilt(DeltaTime);
	UpdateFallShake(DeltaTime);
	ApplyCameraOffsets();
}

void UPlayerCameraEffectsComponent::SetEffectsEnabled(bool bEnabled)
{
	if (bEffectsEnabled == bEnabled)
	{
		return;
	}

	bEffectsEnabled = bEnabled;

	if (!bEnabled)
	{
		ClearCameraOffsets();
	}
}

// ============================================================
// MOVEMENT TILT & BOB
// ============================================================
void UPlayerCameraEffectsComponent::UpdateMovementTilt(float DeltaTime)
{
	const UCharacterMovementComponent* Move = PlayerOwner->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	FRotator TargetTilt = FRotator::ZeroRotator;
	FVector TargetBob = FVector::ZeroVector;

	FVector Velocity = PlayerOwner->GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	const float MaxSpeed = FMath::Max(Move->GetMaxSpeed(), 1.f);

	const bool bSprinting = IsSprinting();
	const bool bGroundedMoving = Move->IsMovingOnGround() && Speed > 25.f;

	if (bGroundedMoving && (bSprinting || bApplyWhileWalking) && !IsRopeActive())
	{
		const float Boost = bSprinting ? SprintTiltMultiplier : 1.f;
		const float SpeedFactor = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.15f);

		// Advance the bob cycle proportionally to distance traveled.
		BobPhase += DeltaTime * TWO_PI * (Speed / FMath::Max(BobCycleLength, 50.f));

		if (bEnableMovementTilt)
		{
			// Velocity expressed relative to the camera view.
			const FRotator CamYaw(0.f, Camera->GetComponentRotation().Yaw, 0.f);
			const FVector CamForward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
			const FVector CamRight = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);

			const FVector Dir = Velocity / MaxSpeed;
			const float LatFactor = FMath::Clamp(FVector::DotProduct(Dir, CamRight), -1.f, 1.f);
			const float FwdFactor = FMath::Clamp(FVector::DotProduct(Dir, CamForward), -1.f, 1.f);

			TargetTilt.Roll = LatFactor * TiltRollAngle * Boost
				+ FMath::Sin(BobPhase) * TiltSwayAngle * SpeedFactor * Boost;
			TargetTilt.Pitch = -FwdFactor * TiltPitchAngle * Boost;
		}

		if (bEnableMovementBob)
		{
			const float BobBoost = bSprinting ? SprintBobMultiplier : 1.f;
			const float Amp = BobAmplitude * SpeedFactor * BobBoost;
			TargetBob.Z = -FMath::Abs(FMath::Sin(BobPhase)) * Amp;
			TargetBob.Y = FMath::Sin(BobPhase) * Amp * BobLateralRatio;
		}
	}
	else
	{
		BobPhase = 0.f;
	}

	CurrentTilt = FMath::RInterpTo(CurrentTilt, TargetTilt, DeltaTime, TiltInterpSpeed);
	CurrentBob = FMath::VInterpTo(CurrentBob, TargetBob, DeltaTime, 10.f);
}

// ============================================================
// FALL SHAKE
// ============================================================
void UPlayerCameraEffectsComponent::UpdateFallShake(float DeltaTime)
{
	const UCharacterMovementComponent* Move = PlayerOwner->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	const float VelocityZ = PlayerOwner->GetVelocity().Z;

	// Track the true descent speed while airborne so the landing shake always
	// reflects the actual impact velocity (a glide landing stays soft).
	if (!Move->IsMovingOnGround())
	{
		LastFallSpeedZ = (VelocityZ < 0.f) ? -VelocityZ : 0.f;
	}

	const UFlightComponent* Flight = PlayerOwner->GetFlightComponent();
	const bool bFlightActive = Flight && Flight->GetCurrentMode() != EFlightMode::None;

	const bool bFreeFalling =
		bEnableFallShake
		&& Move->MovementMode == MOVE_Falling
		&& VelocityZ < 0.f
		&& !bFlightActive
		&& !IsRopeActive();

	float TargetTrauma = 0.f;
	if (bFreeFalling)
	{
		const float Range = FMath::Max(FallShakeMaxSpeed - FallShakeMinSpeed, 1.f);
		TargetTrauma = FMath::Clamp((-VelocityZ - FallShakeMinSpeed) / Range, 0.f, 1.f);
	}

	// Builds slowly while falling; stabilizes quickly when gliding or grounded.
	const float Rate = (TargetTrauma > FallTrauma) ? FallShakeBuildRate : FallStabilizeRate;
	FallTrauma = FMath::FInterpConstantTo(FallTrauma, TargetTrauma, DeltaTime, Rate);

	const float Intensity = FMath::Pow(FallTrauma, 1.5f);
	if (Intensity > KINDA_SMALL_NUMBER)
	{
		const float T = NoiseTime * FallShakeFrequency;
		ShakeRot.Pitch = FMath::PerlinNoise1D(T) * FallShakeRotAmplitude * Intensity;
		ShakeRot.Yaw   = FMath::PerlinNoise1D(T + 39.1f) * FallShakeRotAmplitude * Intensity;
		ShakeRot.Roll  = FMath::PerlinNoise1D(T + 71.7f) * FallShakeRotAmplitude * 0.6f * Intensity;
		ShakeLoc.Y = FMath::PerlinNoise1D(T + 13.3f) * FallShakeLocAmplitude * Intensity;
		ShakeLoc.Z = FMath::PerlinNoise1D(T + 91.9f) * FallShakeLocAmplitude * Intensity;
	}
	else
	{
		ShakeRot = FRotator::ZeroRotator;
		ShakeLoc = FVector::ZeroVector;
	}
}

// ============================================================
// CAMERA APPLICATION
// ============================================================
void UPlayerCameraEffectsComponent::ApplyCameraOffsets()
{
	const FRotator NewRot = CurrentTilt + ShakeRot;
	const FVector NewLoc = CurrentBob + ShakeLoc;

	// Remove last frame's contribution first so any external change to the
	// camera's relative transform is preserved.
	const FRotator BaseRot = Camera->GetRelativeRotation() - LastAppliedRot;
	const FVector BaseLoc = Camera->GetRelativeLocation() - LastAppliedLoc;

	Camera->SetRelativeRotation(BaseRot + NewRot);
	Camera->SetRelativeLocation(BaseLoc + NewLoc);

	LastAppliedRot = NewRot;
	LastAppliedLoc = NewLoc;
}

void UPlayerCameraEffectsComponent::ClearCameraOffsets()
{
	if (Camera)
	{
		Camera->SetRelativeRotation(Camera->GetRelativeRotation() - LastAppliedRot);
		Camera->SetRelativeLocation(Camera->GetRelativeLocation() - LastAppliedLoc);
	}

	LastAppliedRot = FRotator::ZeroRotator;
	LastAppliedLoc = FVector::ZeroVector;
	CurrentTilt = FRotator::ZeroRotator;
	CurrentBob = FVector::ZeroVector;
	ShakeRot = FRotator::ZeroRotator;
	ShakeLoc = FVector::ZeroVector;
	FallTrauma = 0.f;
	BobPhase = 0.f;
}

// ============================================================
// EVENT HANDLERS
// ============================================================
void UPlayerCameraEffectsComponent::HandleHit(AActor* HitActor, float Damage)
{
	if (bEffectsEnabled && bEnableHitShake)
	{
		PlayShake(HitShakeClass, ComputeDamageShakeScale(Damage));
	}
}

void UPlayerCameraEffectsComponent::HandleHitCrit(AActor* HitActor, float Damage)
{
	if (bEffectsEnabled && bEnableHitShake)
	{
		PlayShake(CritShakeClass, ComputeDamageShakeScale(Damage));
	}
}

void UPlayerCameraEffectsComponent::HandleLanded(const FHitResult& Hit)
{
	if (bEffectsEnabled && bEnableLandingShake && LastFallSpeedZ >= LandShakeMinFallSpeed)
	{
		const float Range = FMath::Max(LandShakeMaxFallSpeed - LandShakeMinFallSpeed, 1.f);
		const float Alpha = FMath::Clamp((LastFallSpeedZ - LandShakeMinFallSpeed) / Range, 0.f, 1.f);
		PlayShake(LandShakeClass, FMath::Lerp(0.4f, LandShakeMaxScale, Alpha));
	}

	LastFallSpeedZ = 0.f;
}

// ============================================================
// HELPERS
// ============================================================
void UPlayerCameraEffectsComponent::PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale) const
{
	if (!ShakeClass)
	{
		return;
	}

	if (const APlayerController* PC = GetPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
		}
	}
}

float UPlayerCameraEffectsComponent::ComputeDamageShakeScale(float Damage) const
{
	return FMath::Clamp(Damage / FMath::Max(HitShakeReferenceDamage, 1.f), HitShakeMinScale, HitShakeMaxScale);
}

APlayerController* UPlayerCameraEffectsComponent::GetPlayerController() const
{
	return PlayerOwner ? Cast<APlayerController>(PlayerOwner->GetController()) : nullptr;
}

bool UPlayerCameraEffectsComponent::IsSprinting() const
{
	const UCharacterStateComponent* State = PlayerOwner ? PlayerOwner->GetStateComponent() : nullptr;
	return State && State->IsInMovementState(EtheriaTags::State_Movement_Grounded_Sprinting);
}

bool UPlayerCameraEffectsComponent::IsRopeActive() const
{
	const UCharacterStateComponent* State = PlayerOwner ? PlayerOwner->GetStateComponent() : nullptr;
	return State && State->IsInMovementState(EtheriaTags::State_Movement_Rope);
}
