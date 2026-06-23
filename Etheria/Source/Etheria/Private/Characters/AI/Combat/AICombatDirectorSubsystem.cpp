/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AICombatDirectorSubsystem - Source"
 */

#include "Characters/AI/Combat/AICombatDirectorSubsystem.h"
#include "Engine/World.h"

float UAICombatDirectorSubsystem::NowSeconds() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetTimeSeconds() : 0.f;
}

void UAICombatDirectorSubsystem::PruneLeases(TArray<FAttackLease>& Leases, float Now)
{
	Leases.RemoveAll([Now](const FAttackLease& L)
	{
		return !L.Attacker.IsValid() || L.ExpiryTime <= Now;
	});
}

void UAICombatDirectorSubsystem::SweepStaleEntries()
{
	const float Now = NowSeconds();
	if (Now - LastSweepTime < 5.f) return; // amortized cleanup, every ~5s
	LastSweepTime = Now;
	for (auto It = TokensByTarget.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
	for (auto It = LastAttackStartByTarget.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
}

bool UAICombatDirectorSubsystem::RequestAttackToken(AActor* Target, AActor* Attacker, int32 MaxAttackers, float LeaseDuration)
{
	if (!Target || !Attacker) return false;

	SweepStaleEntries();
	const float Now = NowSeconds();
	const float Lease = FMath::Max(LeaseDuration, 0.1f);

	TArray<FAttackLease>& Leases = TokensByTarget.FindOrAdd(Target);
	PruneLeases(Leases, Now);

	for (FAttackLease& L : Leases)
	{
		if (L.Attacker.Get() == Attacker)
		{
			L.ExpiryTime = Now + Lease;
			return true;
		}
	}

	const int32 Cap = FMath::Max(1, MaxAttackers);
	if (Leases.Num() < Cap)
	{
		FAttackLease NewLease;
		NewLease.Attacker = Attacker;
		NewLease.ExpiryTime = Now + Lease;
		Leases.Add(NewLease);
		return true;
	}

	return false;
}

void UAICombatDirectorSubsystem::ReleaseAttackToken(AActor* Target, AActor* Attacker)
{
	if (!Attacker) return;

	auto ScrubAttacker = [Attacker](TArray<FAttackLease>& Leases)
	{
		Leases.RemoveAll([Attacker](const FAttackLease& L)
		{
			return !L.Attacker.IsValid() || L.Attacker.Get() == Attacker;
		});
	};

	if (Target)
	{
		if (TArray<FAttackLease>* Leases = TokensByTarget.Find(Target))
		{
			ScrubAttacker(*Leases);
			if (Leases->Num() == 0) TokensByTarget.Remove(Target);
		}
		return;
	}

	for (auto It = TokensByTarget.CreateIterator(); It; ++It)
	{
		ScrubAttacker(It.Value());
		if (It.Value().Num() == 0) It.RemoveCurrent();
	}
}

bool UAICombatDirectorSubsystem::HasAttackToken(AActor* Target, AActor* Attacker) const
{
	if (!Target || !Attacker) return false;

	const float Now = NowSeconds();
	if (const TArray<FAttackLease>* Leases = TokensByTarget.Find(Target))
		for (const FAttackLease& L : *Leases)
			if (L.Attacker.Get() == Attacker && L.ExpiryTime > Now)
				return true;

	return false;
}

bool UAICombatDirectorSubsystem::IsAttackWindowOpen(AActor* Target, float MinInterval) const
{
	if (!Target || MinInterval <= 0.f) return true;
	const float Now = NowSeconds();
	if (const float* Last = LastAttackStartByTarget.Find(Target))
		if ((Now - *Last) < MinInterval)
			return false;
	return true;
}

void UAICombatDirectorSubsystem::NotifyAttackStarted(AActor* Target)
{
	if (Target) LastAttackStartByTarget.Add(Target, NowSeconds());
}

int32 UAICombatDirectorSubsystem::GetAttackerCount(AActor* Target) const
{
	if (!Target) return 0;

	const float Now = NowSeconds();
	int32 Count = 0;
	if (const TArray<FAttackLease>* Leases = TokensByTarget.Find(Target))
		for (const FAttackLease& L : *Leases)
			if (L.Attacker.IsValid() && L.ExpiryTime > Now)
				++Count;

	return Count;
}