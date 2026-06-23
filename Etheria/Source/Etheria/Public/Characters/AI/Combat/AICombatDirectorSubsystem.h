/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "AICombatDirectorSubsystem - Header"
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AICombatDirectorSubsystem.generated.h"

UCLASS()
class ETHERIA_API UAICombatDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool RequestAttackToken(AActor* Target, AActor* Attacker, int32 MaxAttackers, float LeaseDuration);
	void ReleaseAttackToken(AActor* Target, AActor* Attacker);
	bool HasAttackToken(AActor* Target, AActor* Attacker) const;
	int32 GetAttackerCount(AActor* Target) const;

	/** Group attack pacing (READ-ONLY): true if at least MinInterval has elapsed since the last attack START
	 *  against this target by ANYONE. Staggers a group so they don't swing in unison. MinInterval <= 0 = always open.
	 *  Does NOT mutate — call NotifyAttackStarted only when an attack actually fires, so a failed swing doesn't block the group. */
	bool IsAttackWindowOpen(AActor* Target, float MinInterval) const;

	/** Stamp "an attack just started on this target, now". Call this only on a confirmed swing. */
	void NotifyAttackStarted(AActor* Target);

	/** Reserve a distinct angular lane (radians, world-space atan2) around Target for Attacker so packmates fully
	 *  ENCIRCLE the target. Keeps an already-held clear lane (hysteresis); otherwise drops the claim in the middle of
	 *  the largest empty angular gap (gap-filling) so the back fills even when everyone approaches from the front —
	 *  k claims settle at 360/k apart. MinSeparation in radians. The claim auto-expires after LeaseDuration — refresh
	 *  it each reposition; no explicit release is needed. */
	float ReserveAttackAngle(AActor* Target, AActor* Attacker, float PreferredAngle, float MinSeparation, float LeaseDuration);

private:
	struct FAttackLease
	{
		TWeakObjectPtr<AActor> Attacker;
		float ExpiryTime = 0.f;
	};

	struct FAngleClaim
	{
		TWeakObjectPtr<AActor> Attacker;
		float Angle = 0.f;       // world-space slot angle around the target (radians)
		float ExpiryTime = 0.f;
	};

	static void PruneLeases(TArray<FAttackLease>& Leases, float Now);
	static void PruneAngleClaims(TArray<FAngleClaim>& Claims, float Now);
	float NowSeconds() const;

	/** Periodically drop entries whose target weak-pointer has gone stale (died/despawned). */
	void SweepStaleEntries();
	float LastSweepTime = 0.f;

	TMap<TWeakObjectPtr<AActor>, TArray<FAttackLease>> TokensByTarget;

	/** Last attack-start time per target, used for group attack pacing (TryReserveAttackWindow). */
	TMap<TWeakObjectPtr<AActor>, float> LastAttackStartByTarget;

	/** Claimed attack-slot angles per target, used to spread attackers around it (ReserveAttackAngle). */
	TMap<TWeakObjectPtr<AActor>, TArray<FAngleClaim>> AnglesByTarget;
};