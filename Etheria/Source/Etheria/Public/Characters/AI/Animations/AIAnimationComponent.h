/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIAnimationComponent - Header"
 * Notes: Supports two modes: Direct Playback (no ABP) and Animation Blueprint.
 *        Handles locomotion, idle variations, action montages, crossfade transitions.
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
	Idle, WalkForward, WalkBackward, RunForward, RunBackward, Falling, Landing
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAIAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIAnimationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* ═══════════ Locomotion ═══════════ */

	UFUNCTION(BlueprintCallable, Category="AI|Animation") void SetLocomotionState(EAILocomotionState NewState);
	UFUNCTION(BlueprintPure, Category="AI|Animation") EAILocomotionState GetLocomotionState() const { return CurrentLocomotionState; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void PauseLocomotion() { bLocomotionPaused = true; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void ResumeLocomotion() { bLocomotionPaused = false; }

	/* ═══════════ Actions ═══════════ */

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomIdle();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayIdleByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayAttackByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayInteraction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayHitReaction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomHitReaction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomDeath();

	/** Play any montage as a one-shot action. Pauses locomotion, resumes when done. */
	UFUNCTION(BlueprintCallable, Category="AI|Animation")
	UAnimMontage* PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.f);

	/** Stop current action and resume locomotion. */
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void StopCurrentAction();

	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingAction() const { return bIsPlayingAction; }

	/** Is the AI currently playing a non-interruptible idle variation? */
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingIdleVariation() const { return bIsPlayingIdleVariation; }

	/* ═══════════ Dispatchers ═══════════ */

	UPROPERTY(BlueprintAssignable) FOnAIAnimStarted OnAIAnimStarted;
	UPROPERTY(BlueprintAssignable) FOnAIAnimEnded OnAIAnimEnded;
	UPROPERTY(BlueprintAssignable) FOnLocomotionStateChanged OnLocomotionStateChanged;

	/* ═══════════ Animation Mode ═══════════ */

	/** How animations are played. Direct = no ABP needed (creatures). AnimBlueprint = uses ABP with Slot (humanoids). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Mode")
	EAIAnimationMode AnimationMode = EAIAnimationMode::DirectPlayback;

	/* ═══════════ Locomotion Montages ═══════════ */

	/** Base idle loop. Plays when the AI is standing still. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> IdleBaseMontage;

	/** Walk forward animation (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> WalkForwardMontage;

	/** Walk backward animation (looping). Falls back to WalkForward if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> WalkBackwardMontage;

	/** Run forward animation (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> RunForwardMontage;

	/** Run backward animation (looping). Falls back to RunForward if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> RunBackwardMontage;

	/** Falling/in-air animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> FallingMontage;

	/** Landing animation (one-shot after falling). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> LandingMontage;

	/* ═══════════ Thresholds ═══════════ */

	/** Speed below this value = Idle state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0"))
	float IdleSpeedThreshold = 5.f;

	/** Speed above this value = Running (below = Walking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0"))
	float RunSpeedThreshold = 300.f;

	/* ═══════════ Transition ═══════════ */

	/** Blend time when switching between locomotion animations. Higher = smoother but slower transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Transition", meta=(ClampMin="0", ClampMax="1.0"))
	float LocomotionBlendTime = 0.25f;

	/** Blend time when starting/ending action montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Transition", meta=(ClampMin="0", ClampMax="1.0"))
	float ActionBlendTime = 0.2f;

	/* ═══════════ Action Pools ═══════════ */

	/** Special idle animations (looking around, scratching, yawning). Plays to completion before moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle")
	TArray<TObjectPtr<UAnimMontage>> IdleVariations;

	/** Attack animation montages. Picked randomly or by index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Attack")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** Hit reaction montages. One is picked randomly when the AI takes damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction")
	TArray<TObjectPtr<UAnimMontage>> HitReactionMontages;

	/** Death animation montages. One is picked randomly. Never resumes locomotion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Death")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	/** Montage for NPC interaction (talking, quest giving). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Interaction")
	TObjectPtr<UAnimMontage> InteractionMontage;

	/** Playback rate for idle variation montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation", meta=(ClampMin="0.5", ClampMax="2.0"))
	float IdlePlayRate = 1.f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateLocomotion();
	void PlayOnMesh(UAnimMontage* Montage, bool bLoop, float PlayRate = 1.f);
	void PlayViaMontageSystem(UAnimMontage* Montage, float PlayRate = 1.f);

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
};
