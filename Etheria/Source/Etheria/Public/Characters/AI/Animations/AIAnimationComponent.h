/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "AIAnimationComponent - Header"
 * Notes: Two modes:
 *   DirectPlayback — PlayAnimation(Montage) on mesh. No ABP. For creatures.
 *   AnimBlueprint  — Feeds GroundSpeed/Direction/Velocity to the ABP. Montage_Play for actions.
 *                    The ABP handles locomotion blend spaces. For humanoids.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/AI_Types.h"
#include "AIAnimationComponent.generated.h"

class UAnimMontage;
class UAnimInstance;
class ABaseAICharacter;
class FBoolProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIAnimStarted, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIAnimEnded, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLocomotionStateChanged);

UENUM(BlueprintType)
enum class EAILocomotionState : uint8
{
	Idle, WalkForward, WalkBackward, RunForward, RunBackward, Falling, Landing, WalkLeft, WalkRight
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAIAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIAnimationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="AI|Animation") void SetLocomotionState(EAILocomotionState NewState);
	UFUNCTION(BlueprintPure, Category="AI|Animation") EAILocomotionState GetLocomotionState() const { return CurrentLocomotionState; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void PauseLocomotion() { bLocomotionPaused = true; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void ResumeLocomotion() { bLocomotionPaused = false; }

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomIdle();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayIdleByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomActivity();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayAttackByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayInteraction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayHitReaction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomHitReaction();

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDirectionalHitReaction(const FVector& WorldHitDir);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayStartle();

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayMenace();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayLoopingAction(UAnimMontage* Montage, float MaxDuration, float PlayRate = 1.f);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void StopCurrentAction();
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingAction() const { return bIsPlayingAction; }
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingIdleVariation() const { return bIsPlayingIdleVariation; }
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingHitReact() const { return bIsPlayingAction && bCurrentActionIsHitReact; }
	UFUNCTION(BlueprintPure, Category="AI|Animation") UAnimMontage* GetCurrentActionMontage() const { return CurrentActionMontage; }

	UPROPERTY(BlueprintAssignable) FOnAIAnimStarted OnAIAnimStarted;
	UPROPERTY(BlueprintAssignable) FOnAIAnimEnded OnAIAnimEnded;
	UPROPERTY(BlueprintAssignable) FOnLocomotionStateChanged OnLocomotionStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Mode",
		meta=(ToolTip="DirectPlayback: plays montages directly on mesh, no ABP needed.\nAnimBlueprint: uses your ABP for locomotion, feeds GroundSpeed/Direction/etc."))
	EAIAnimationMode AnimationMode = EAIAnimationMode::DirectPlayback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Mode",
		meta=(ToolTip="Fix a mis-set mode automatically so every AI animates out of the box. OFF = trust AnimationMode as-is."))
	bool bAutoCorrectAnimationMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint",
		ToolTip="Name of the FVector variable in your AnimBP that receives the character velocity."))
	FName ABP_VelocityName = TEXT("Velocity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_GroundSpeedName = TEXT("GroundSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_FallSpeedName = TEXT("FallSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_DirectionName = TEXT("Direction");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint",
		ToolTip="Smoothed yaw rotation speed (deg/s, signed) fed to the ABP so a turn-in-place blendspace/lean can react when the AI pivots while stationary."))
	FName ABP_YawSpeedName = TEXT("YawDeltaSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> IdleBaseMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkBackwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> RunForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> RunBackwardMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkLeftMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkRightMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> FallingMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> LandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback",
		ToolTip="Played when the AI pivots left while stationary (turn-in-place). Optional — without it the body just rotates."))
	TObjectPtr<UAnimMontage> TurnLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback",
		ToolTip="Played when the AI pivots right while stationary (turn-in-place). Optional."))
	TObjectPtr<UAnimMontage> TurnRightMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="10", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback",
		ToolTip="Yaw speed (deg/s) above which a stationary AI plays its turn montage instead of ice-skating."))
	float TurnInPlaceYawSpeed = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float IdleSpeedThreshold = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float RunSpeedThreshold = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="1", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float WalkRefSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="1", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float RunRefSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0.1", ClampMax="0.99", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float RunHysteresisFraction = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Smoothing", meta=(ClampMin="0.1", ClampMax="1")) float PlayRateMin = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Smoothing", meta=(ClampMin="1", ClampMax="4")) float PlayRateMax = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Smoothing", meta=(ClampMin="1")) float PlayRateInterpSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Smoothing", meta=(ClampMin="1")) float GroundSpeedInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Smoothing", meta=(ClampMin="1")) float DirectionInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle", meta=(ToolTip="Special idle animations. Play to completion before resuming patrol.")) TArray<TObjectPtr<UAnimMontage>> IdleVariations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle", meta=(ToolTip="Dwell activities (graze/peck/sleep) played at patrol points.")) TArray<TObjectPtr<UAnimMontage>> ActivityMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Attack") TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction") TArray<TObjectPtr<UAnimMontage>> HitReactionMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactFront;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Flee") TObjectPtr<UAnimMontage> StartleMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Combat") TObjectPtr<UAnimMontage> MenaceMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Death") TArray<TObjectPtr<UAnimMontage>> DeathMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Interaction") TObjectPtr<UAnimMontage> InteractionMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation", meta=(ClampMin="0.5", ClampMax="2.0")) float IdlePlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Transition", meta=(ClampMin="0", ClampMax="1.0")) float ActionBlendTime = 0.2f;

protected:
	virtual void BeginPlay() override;

private:

	void EnsureModeInitialized();
	void UpdateLocomotionDirect();
	void ApplyLocomotionSpeedMatch(float DeltaTime);
	void UpdateABPVariables(float DeltaTime);
	void PlayOnMesh(UAnimMontage* Montage, bool bLoop, float PlayRate = 1.f);
	UAnimMontage* PlayViaMontageSystem(UAnimMontage* Montage, float PlayRate = 1.f);

	void SetABPFloat(UAnimInstance* Anim, FName Name, float Value);
	void SetABPVector(UAnimInstance* Anim, FName Name, const FVector& Value);

	UFUNCTION() void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UAnimMontage> CurrentLocomotionMontage;
	UPROPERTY() TObjectPtr<UAnimMontage> CurrentActionMontage;

	EAILocomotionState CurrentLocomotionState = EAILocomotionState::Idle;
	bool bLocomotionPaused = false;
	bool bIsPlayingAction = false;
	bool bIsPlayingIdleVariation = false;
	bool bCurrentActionIsHitReact = false;
	bool bModeInitialized = false;
	TWeakObjectPtr<UAnimInstance> BoundAnimInstance;
	float ActionTimer = 0.f;
	int32 LastIdleIndex = -1;
	int32 LastActivityIndex = -1;

	float SmoothedPlayRate = 1.f;
	float SmoothedGroundSpeed = 0.f;
	float SmoothedDirection = 0.f;
	bool  bABPShouldMove = false;
	FBoolProperty* ShouldMovePropCached = nullptr;
	FBoolProperty* IsFallingPropCached  = nullptr;

	FName ResolvedVelocityName;
	FName ResolvedGroundSpeedName;
	FName ResolvedFallSpeedName;
	FName ResolvedDirectionName;
	FName ResolvedYawSpeedName;
	float SmoothedYawSpeed = 0.f;
	float LastOwnerYaw = 0.f;
	bool bHasLastYaw = false;
	float TurnMontageCooldown = 0.f;

	TWeakObjectPtr<UClass> CachedABPClass;
};
