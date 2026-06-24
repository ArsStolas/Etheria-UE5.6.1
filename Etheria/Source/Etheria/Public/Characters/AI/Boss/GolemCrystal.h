/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemCrystal - Header"
 * Notes: Destructible crystal the Golem spawns — the small arena crystals planted at the two-hand-slam
 *        fissure points, and the big crystal that opens at the arena centre during the stun. It collides on
 *        the player's melee object channel (ECC_Pawn) and routes every hit it takes back to the boss brain
 *        (UGolemBossComponent::HitArenaCrystal / HitBigCrystal), so NO Blueprint event-graph wiring is
 *        needed: just set the Golem's ArenaCrystalActorClass / BigCrystalActorClass to a child BP that
 *        carries a crystal mesh.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GolemCrystal.generated.h"

class UStaticMeshComponent;
class UGolemBossComponent;

UCLASS()
class ETHERIA_API AGolemCrystal : public AActor
{
	GENERATED_BODY()

public:
	AGolemCrystal();

	/** Wire this as a small ARENA crystal (the slam plants two). Id maps it to the boss's hit/destroy bookkeeping. */
	void InitAsArenaCrystal(UGolemBossComponent* InOwnerComp, int32 InCrystalId);

	/** Wire this as the single BIG stun crystal — breaking it removes a big HP chunk and ends the stun. */
	void InitAsBigCrystal(UGolemBossComponent* InOwnerComp);

	/** Called by the boss after a NON-killing hit — drives the "getting more cracked" anim (fires OnCrystalDamaged). */
	void NotifyDamaged(int32 InHitsRemaining, int32 InHitsTotal);

	/** Disable collision + fire OnCrystalShattered (the BP plays the break anim), then self-destruct after ShatterLifetime.
	 *  Deferred (not an immediate Destroy) so the actor stays valid for the rest of the current hit's processing — e.g. the
	 *  player BP doing Spawn-Sound-Attached on the hit actor, which errors if we go pending-kill mid-callstack. */
	void Shatter();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	bool IsBigCrystal() const { return bIsBig; }

protected:
	/** Visible crystal mesh. In a child BP, select this component and set its Static Mesh. Collides on ECC_Pawn so
	 *  the player's melee object-trace finds it, but blocks nothing (it never shoves the player or movement). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Golem") TObjectPtr<UStaticMeshComponent> MeshComp;

	/** How long the shattered actor lingers (so your break anim/VFX can finish) before it self-destructs. Match your shatter anim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystal", meta = (ClampMin = "0.05")) float ShatterLifetime = 2.f;

	/* ── Anim hooks: override these in the crystal BP to drive its animations / VFX / material state ── */

	/** SPAWN anim: the crystal just appeared — play its rise / appear anim. bBig = the big stun crystal (vs a small arena one). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Golem|Crystal") void OnCrystalSpawned(bool bBig);

	/** A-BIT-BROKEN anim: took a hit but still standing. HitsRemaining/HitsTotal + DamageFraction (0 = fresh → 1 = about to
	 *  break) so you can drive a progressively-cracked state (a material scalar, a damage-stage anim, a mesh swap…). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Golem|Crystal") void OnCrystalDamaged(int32 HitsRemaining, int32 HitsTotal, float DamageFraction);

	/** FULLY-BROKEN anim: it shattered — play the break anim/VFX. The actor self-destructs after ShatterLifetime. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Golem|Crystal") void OnCrystalShattered();

private:
	TWeakObjectPtr<UGolemBossComponent> OwnerComp;
	int32 CrystalId = -1;
	bool bIsBig = false;
	bool bShattered = false;
};
