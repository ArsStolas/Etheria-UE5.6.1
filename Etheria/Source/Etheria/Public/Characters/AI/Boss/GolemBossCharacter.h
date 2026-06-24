/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossCharacter - Header"
 * Notes: Drop-in arena Golem. A BaseAICharacter that owns a UGolemBossComponent and ships with
 *        boss-friendly defaults (Aggressive, Boss rank, hand-faced, paired with AGolemBossController).
 *        Make a Blueprint child to assign the mesh, montages, attack list and bind the Golem dispatchers.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAICharacter.h"
#include "GolemBossCharacter.generated.h"

class UGolemBossComponent;

UCLASS()
class ETHERIA_API AGolemBossCharacter : public ABaseAICharacter
{
	GENERATED_BODY()

public:
	AGolemBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Golem") UGolemBossComponent* GetGolemBoss() const { return GolemBossComponent; }

	/** Anchored giant: it can never be knocked back or shoved out of place by the player, yet stays fully hittable.
	 *  The body is frozen in world space (it only yaws to face the target). Turn OFF for a pushable Golem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact") bool bImmovable = true;

	/** While bImmovable, swallow every launch (player combat-impulse notify, AoE knockback) so the body never moves. */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;

	/** Funnel incoming damage to the Golem's weak points (arm nearest the attacker, or the head crystal while toppled)
	 *  instead of the invulnerable body — so the existing melee, which only resolves "the boss actor", still hurts the giant. */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Re-pin the immovable body to its anchor + zero its velocity. Called from the pawn Tick AND the component's
	 *  BrainTick timer, because an AI boss pawn can stop ticking per-frame (dormancy) — the timer path always runs. */
	void EnforceImmovable();

protected:
	virtual void BeginPlay() override;

	/** While bImmovable, hard-pin the body to its placed location every frame so nothing (capsule depenetration after
	 *  scaling, a stray BP/physics knockback that bypasses LaunchCharacter, etc.) can ever fling the giant into the air. */
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UGolemBossComponent> GolemBossComponent;

private:
	/** Fills the component with the 6 demo attacks + a light 2-phase ramp as editable defaults (overridable in a BP child). */
	void BuildDefaultAttacks();

	FVector ImmovableAnchor = FVector::ZeroVector; // placed world location the immovable Golem is pinned to
	bool bImmovableAnchorSet = false;
};
