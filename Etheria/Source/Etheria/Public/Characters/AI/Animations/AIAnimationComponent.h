/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIAnimationComponent - Header
 * Manages animation montage slots for AI characters.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIAnimationComponent.generated.h"

class UAnimMontage;
class ABaseAICharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIMontageStartedDelegate, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIMontageEndedDelegate, UAnimMontage*, Montage);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAIAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIAnimationComponent();

	/* ── Play API ── */

	/** Play a random idle montage from the pool. Returns the picked montage. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayRandomIdle();

	/** Play a specific idle montage by index. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayIdleByIndex(int32 Index);

	/** Play the walk/run montage (blended by speed in ABP is recommended, but this is a fallback). */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayMovement();

	/** Play a random attack montage. Returns it for chaining. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayRandomAttack();

	/** Play attack montage by index. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayAttackByIndex(int32 Index);

	/** Play interaction montage (talk, quest give, etc.). */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayInteraction();

	/** Play hit reaction. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayHitReaction();

	/** Play death montage. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	UAnimMontage* PlayDeath();

	/** Stop any currently playing montage on this character. */
	UFUNCTION(BlueprintCallable, Category = "AI|Animation")
	void StopCurrentMontage(float BlendOut = 0.25f);

	UFUNCTION(BlueprintPure, Category = "AI|Animation")
	bool IsPlayingMontage() const;

	/* ── Dispatchers ── */

	UPROPERTY(BlueprintAssignable, Category = "AI|Animation")
	FOnAIMontageStartedDelegate OnAIMontageStarted;

	UPROPERTY(BlueprintAssignable, Category = "AI|Animation")
	FOnAIMontageEndedDelegate OnAIMontageEnded;

	/* ── Montage Pools (set in editor or per-child BP) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|Idle")
	TArray<TObjectPtr<UAnimMontage>> IdleMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|Movement")
	TObjectPtr<UAnimMontage> MovementMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|Attack")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|Interaction")
	TObjectPtr<UAnimMontage> InteractionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|HitReaction")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** Playback rate for idle montages (useful for variety). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Animation", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float IdlePlayRate = 1.f;

protected:
	virtual void BeginPlay() override;

private:
	UAnimMontage* PlayMontageInternal(UAnimMontage* Montage, float PlayRate = 1.f);

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY()
	TObjectPtr<ABaseAICharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentMontage;
};