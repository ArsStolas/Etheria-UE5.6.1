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

	bool IsAttackWindowOpen(AActor* Target, float MinInterval, float HitGrace = 0.f) const;

	void NotifyAttackStarted(AActor* Target);

	void NotifyAttackConnected(AActor* Target);

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
		float Angle = 0.f;
		float ExpiryTime = 0.f;
	};

	static void PruneLeases(TArray<FAttackLease>& Leases, float Now);
	static void PruneAngleClaims(TArray<FAngleClaim>& Claims, float Now);
	float NowSeconds() const;

	void SweepStaleEntries();
	float LastSweepTime = 0.f;

	TMap<TWeakObjectPtr<AActor>, TArray<FAttackLease>> TokensByTarget;

	TMap<TWeakObjectPtr<AActor>, float> LastAttackStartByTarget;

	TMap<TWeakObjectPtr<AActor>, float> LastHitConnectByTarget;

	TMap<TWeakObjectPtr<AActor>, TArray<FAngleClaim>> AnglesByTarget;
};
