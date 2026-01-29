/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerSwimAnimInstance" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerSwimAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UAnimMontage;
class USwimComponent;
class USwimAnimationSet;

/**
 * Animation Instance helper for swimming.
 *
 * Goals:
 * - Provide reliable, cached variables to the AnimBP (avoid casts/None).
 * - Optional auto-play of directional montages using a USwimAnimationSet.
 * - Provide smoothed values (less snappy).
 */
UCLASS(BlueprintType)
class ETHERIA_API UPlayerSwimAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPlayerSwimAnimInstance();

#pragma region RuntimeValues
	/** True if CharacterMovement is in MOVE_Swimming. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="True if CharacterMovement is in MOVE_Swimming."))
	bool bIsSwimming = false;

	/** True if owner is inside Water PhysicsVolume (bWaterVolume). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="True if owner is inside a Water PhysicsVolume (bWaterVolume)."))
	bool bInWater = false;

	/** True if swim mode is underwater (from USwimComponent). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="True if swim mode is underwater (from USwimComponent)."))
	bool bIsUnderwater = false;

	/** 2D speed (XY) while swimming. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="2D speed (XY) while swimming."))
	float SwimSpeed = 0.f;

	/** Direction angle (-180..180) computed from velocity vs actor rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="Direction angle (-180..180) computed from velocity vs actor rotation."))
	float SwimDirection = 0.f;

	/** Smoothed Speed (FInterp) for dynamic feeling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="Smoothed swim speed (FInterp) for less snappy feel."))
	float SmoothedSpeed = 0.f;

	/** Smoothed Direction (FInterp) for dynamic feeling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Swim|Runtime", meta=(ToolTip="Smoothed swim direction (FInterp) for less snappy feel."))
	float SmoothedDirection = 0.f;
#pragma endregion

#pragma region Settings
	/** If true, this AnimInstance will auto-play loop montages from the Animation Set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swim|AutoMontage", meta=(ToolTip="If true, this AnimInstance auto-plays directional loop montages from the Animation Set (if BlendSpaces are not provided)."))
	bool bAutoPlayDirectionalMontages = false;

	/** Optional override. If unset, AnimInstance uses SwimComponent->AnimationSet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swim|AutoMontage", meta=(ToolTip="Optional override for swim animation set. If unset, uses SwimComponent's AnimationSet."))
	TObjectPtr<USwimAnimationSet> AnimationSetOverride = nullptr;

	/** Speed below which we consider the character idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swim|AutoMontage", meta=(ClampMin="0.0", ToolTip="Speed below which we consider the character idle for montage selection."))
	float IdleSpeedThreshold = 12.f;

	/** Minimum time between montage switches to prevent spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swim|AutoMontage", meta=(ClampMin="0.0", ToolTip="Minimum time between montage switches to prevent spam."))
	float MinTimeBetweenMontageSwitch = 0.25f;

	/** Blend-out time when switching loop montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Swim|AutoMontage", meta=(ClampMin="0.0", ToolTip="Blend-out time when switching loop montages."))
	float MontageBlendOutTime = 0.15f;
#pragma endregion

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
#pragma region CachedRefs
	UPROPERTY(Transient) TObjectPtr<ACharacter> OwnerCharacter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;
	UPROPERTY(Transient) TObjectPtr<USwimComponent> SwimComponent = nullptr;
#pragma endregion

#pragma region AutoMontageInternals
	float LastMontageSwitchTime = -1000.f;

	void UpdateAutoMontage(float DeltaSeconds);
	UAnimMontage* SelectDirectionalLoopMontage(const USwimAnimationSet* AnimSet) const;
#pragma endregion
};
