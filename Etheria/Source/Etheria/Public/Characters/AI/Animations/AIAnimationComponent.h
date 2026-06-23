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
class ABaseAICharacter;

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
	/** Hit reaction chosen by the local octant of the incoming hit. Falls back to the generic pool if no directional clips are set. */
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDirectionalHitReaction(const FVector& WorldHitDir);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayStartle();
	/** Play the combat "menace/howl" montage (used while a pack member holds back waiting its turn). Works in
	 *  DirectPlayback because the AI holds still while menacing. No-op if MenaceMontage is unset. */
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayMenace();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.f);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void StopCurrentAction();
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingAction() const { return bIsPlayingAction; }
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingIdleVariation() const { return bIsPlayingIdleVariation; }

	UPROPERTY(BlueprintAssignable) FOnAIAnimStarted OnAIAnimStarted;
	UPROPERTY(BlueprintAssignable) FOnAIAnimEnded OnAIAnimEnded;
	UPROPERTY(BlueprintAssignable) FOnLocomotionStateChanged OnLocomotionStateChanged;

	/* ═══════════ Animation Mode ═══════════ */

	/** Direct = no ABP (creatures). AnimBlueprint = ABP handles locomotion, component feeds variables (humanoids). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Mode",
		meta=(ToolTip="DirectPlayback: plays montages directly on mesh, no ABP needed.\nAnimBlueprint: uses your ABP for locomotion, feeds GroundSpeed/Direction/etc."))
	EAIAnimationMode AnimationMode = EAIAnimationMode::DirectPlayback;

	/* ── ABP Variable Names (must match your ABP variables) ── */

	/** Name of the Velocity vector variable in your ABP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint",
		ToolTip="Name of the FVector variable in your AnimBP that receives the character velocity."))
	FName ABP_VelocityName = TEXT("Velocity");

	/** Name of the GroundSpeed float variable in your ABP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_GroundSpeedName = TEXT("GroundSpeed");

	/** Name of the FallSpeed float variable in your ABP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_FallSpeedName = TEXT("FallSpeed");

	/** Name of the Direction float variable in your ABP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|ABP Variables", meta=(EditCondition="AnimationMode==EAIAnimationMode::AnimBlueprint"))
	FName ABP_DirectionName = TEXT("Direction");

	/* ═══════════ Locomotion Montages (DirectPlayback mode only) ═══════════ */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> IdleBaseMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkBackwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> RunForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> RunBackwardMontage;
	/** Optional strafe clips for combat orbiting (body faces target, moves sideways). Fall back to WalkForward if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkLeftMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> WalkRightMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> FallingMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion", meta=(EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) TObjectPtr<UAnimMontage> LandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float IdleSpeedThreshold = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float RunSpeedThreshold = 300.f;

	/** Speed the WALK clip was authored for; its play rate is matched to actual speed to kill foot-sliding (DirectPlayback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="1", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float WalkRefSpeed = 200.f;
	/** Speed the RUN clip was authored for (play-rate speed-matching). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="1", EditCondition="AnimationMode==EAIAnimationMode::DirectPlayback")) float RunRefSpeed = 500.f;

	/* ═══════════ Action Pools (both modes) ═══════════ */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle", meta=(ToolTip="Special idle animations. Play to completion before resuming patrol.")) TArray<TObjectPtr<UAnimMontage>> IdleVariations;

	/** Longer "activity" animations played at patrol points (graze, peck, drink, sleep...). Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle", meta=(ToolTip="Dwell activities (graze/peck/sleep) played at patrol points.")) TArray<TObjectPtr<UAnimMontage>> ActivityMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Attack") TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction") TArray<TObjectPtr<UAnimMontage>> HitReactionMontages;

	/** Optional directional hit reactions (by where the hit came from). Empty sets fall back to HitReactionMontages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactFront;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction|Directional") TArray<TObjectPtr<UAnimMontage>> HitReactRight;

	/** Played once when this AI starts fleeing (startle/panic reaction). Optional — leave empty for no startle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Flee") TObjectPtr<UAnimMontage> StartleMontage;

	/** Played while this AI holds back waiting its turn to attack in a pack (howl/menace/snarl). Optional. Add the howl
	 *  SFX as an AnimNotify on the montage. Leave empty for no menace animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Combat") TObjectPtr<UAnimMontage> MenaceMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Death") TArray<TObjectPtr<UAnimMontage>> DeathMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Interaction") TObjectPtr<UAnimMontage> InteractionMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation", meta=(ClampMin="0.5", ClampMax="2.0")) float IdlePlayRate = 1.f;

	/** Blend time when transitioning between animations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Transition", meta=(ClampMin="0", ClampMax="1.0")) float ActionBlendTime = 0.2f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateLocomotionDirect();
	void ApplyLocomotionSpeedMatch();
	void UpdateABPVariables();
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
	bool bMontageCallbackBound = false;
	float ActionTimer = 0.f;
	int32 LastIdleIndex = -1;
	int32 LastActivityIndex = -1;
};
