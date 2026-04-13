/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIAnimationComponent - Header"
 * Notes: All animations via UAnimMontage played with PlayAnimation() on the mesh.
 *        No ABP. Notifies/VFX in montages work. Set mesh to "Use Animation Asset".
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	UFUNCTION(BlueprintCallable, Category="AI|Animation") void SetLocomotionState(EAILocomotionState NewState);
	UFUNCTION(BlueprintPure, Category="AI|Animation") EAILocomotionState GetLocomotionState() const { return CurrentLocomotionState; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void PauseLocomotion() { bLocomotionPaused = true; }
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void ResumeLocomotion() { bLocomotionPaused = false; }

	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomIdle();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayIdleByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayAttackByIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayInteraction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayHitReaction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomHitReaction();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayRandomDeath();
	UFUNCTION(BlueprintCallable, Category="AI|Animation") UAnimMontage* PlayActionMontage(UAnimMontage* Montage, float PlayRate = 1.f);
	UFUNCTION(BlueprintCallable, Category="AI|Animation") void StopCurrentAction();
	UFUNCTION(BlueprintPure, Category="AI|Animation") bool IsPlayingAction() const { return bIsPlayingAction; }

	UPROPERTY(BlueprintAssignable) FOnAIAnimStarted OnAIAnimStarted;
	UPROPERTY(BlueprintAssignable) FOnAIAnimEnded OnAIAnimEnded;
	UPROPERTY(BlueprintAssignable) FOnLocomotionStateChanged OnLocomotionStateChanged;

	/* ── Locomotion ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> IdleBaseMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> WalkForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> WalkBackwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> RunForwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> RunBackwardMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> FallingMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Locomotion") TObjectPtr<UAnimMontage> LandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0")) float IdleSpeedThreshold = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Thresholds", meta=(ClampMin="0")) float RunSpeedThreshold = 300.f;

	/* ── Action pools ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Idle") TArray<TObjectPtr<UAnimMontage>> IdleVariations;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Attack") TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|HitReaction") TArray<TObjectPtr<UAnimMontage>> HitReactionMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Death") TArray<TObjectPtr<UAnimMontage>> DeathMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation|Interaction") TObjectPtr<UAnimMontage> InteractionMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Animation", meta=(ClampMin="0.5", ClampMax="2.0")) float IdlePlayRate = 1.f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateLocomotion();
	void PlayOnMesh(UAnimMontage* Montage, bool bLoop, float PlayRate = 1.f);

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UAnimMontage> CurrentLocomotionMontage;
	UPROPERTY() TObjectPtr<UAnimMontage> CurrentActionMontage;

	EAILocomotionState CurrentLocomotionState = EAILocomotionState::Idle;
	bool bLocomotionPaused = false;
	bool bIsPlayingAction = false;
	float ActionTimer = 0.f;
};
