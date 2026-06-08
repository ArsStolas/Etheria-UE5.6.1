/**
 * Etheria's End Project, 2025
 * Created by: Mato
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

private:
	struct FAttackLease
	{
		TWeakObjectPtr<AActor> Attacker;
		float ExpiryTime = 0.f;
	};

	static void PruneLeases(TArray<FAttackLease>& Leases, float Now);
	float NowSeconds() const;

	TMap<TWeakObjectPtr<AActor>, TArray<FAttackLease>> TokensByTarget;
};