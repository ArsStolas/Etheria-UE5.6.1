/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
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

void UAICombatDirectorSubsystem::PruneAngleClaims(TArray<FAngleClaim>& Claims, float Now)
{
	Claims.RemoveAll([Now](const FAngleClaim& C)
	{
		return !C.Attacker.IsValid() || C.ExpiryTime <= Now;
	});
}

void UAICombatDirectorSubsystem::SweepStaleEntries()
{
	const float Now = NowSeconds();
	if (Now - LastSweepTime < 5.f) return;
	LastSweepTime = Now;
	for (auto It = TokensByTarget.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
	for (auto It = LastAttackStartByTarget.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
	for (auto It = LastHitConnectByTarget.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
	for (auto It = AnglesByTarget.CreateIterator(); It; ++It)
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

bool UAICombatDirectorSubsystem::IsAttackWindowOpen(AActor* Target, float MinInterval, float HitGrace) const
{
	if (!Target) return true;
	const float Now = NowSeconds();
	if (MinInterval > 0.f)
		if (const float* Last = LastAttackStartByTarget.Find(Target))
			if ((Now - *Last) < MinInterval)
				return false;
	if (HitGrace > 0.f)
		if (const float* LastHit = LastHitConnectByTarget.Find(Target))
			if ((Now - *LastHit) < HitGrace)
				return false;
	return true;
}

void UAICombatDirectorSubsystem::NotifyAttackStarted(AActor* Target)
{
	if (Target) LastAttackStartByTarget.Add(Target, NowSeconds());
}

void UAICombatDirectorSubsystem::NotifyAttackConnected(AActor* Target)
{
	if (Target) LastHitConnectByTarget.Add(Target, NowSeconds());
}

float UAICombatDirectorSubsystem::ReserveAttackAngle(AActor* Target, AActor* Attacker, float PreferredAngle, float MinSeparation, float LeaseDuration)
{
	if (!Target || !Attacker) return PreferredAngle;

	const float Now = NowSeconds();
	const float Lease = FMath::Max(LeaseDuration, 0.1f);

	TArray<FAngleClaim>& Claims = AnglesByTarget.FindOrAdd(Target);
	PruneAngleClaims(Claims, Now);

	TArray<float> Others;
	Others.Reserve(Claims.Num());
	for (const FAngleClaim& C : Claims)
		if (C.Attacker.Get() != Attacker)
			Others.Add(C.Angle);

	FAngleClaim* Mine = Claims.FindByPredicate([Attacker](const FAngleClaim& C){ return C.Attacker.Get() == Attacker; });

	auto MinClearOf = [&Others](float Ang) -> float
	{
		float MinClear = PI;
		for (const float OA : Others)
			MinClear = FMath::Min(MinClear, FMath::Abs(FMath::FindDeltaAngleRadians(Ang, OA)));
		return MinClear;
	};

	float Granted;
	if (Others.Num() == 0)
	{

		Granted = PreferredAngle;
	}
	else if (Mine && MinClearOf(Mine->Angle) >= MinSeparation)
	{

		Granted = (MinClearOf(PreferredAngle) >= MinSeparation) ? PreferredAngle : Mine->Angle;
	}
	else
	{

		TArray<float> Sorted = Others;
		Sorted.Sort();
		const int32 N = Sorted.Num();
		float BestMid = PreferredAngle;
		float BestGap = -1.f;
		for (int32 i = 0; i < N; ++i)
		{
			const float A = Sorted[i];
			const float B = Sorted[(i + 1) % N];
			float Gap = B - A;
			if (Gap <= 0.f) Gap += 2.f * PI;
			if (Gap > BestGap) { BestGap = Gap; BestMid = A + Gap * 0.5f; }
		}
		Granted = FMath::UnwindRadians(BestMid);
	}

	if (Mine)
	{
		Mine->Angle = Granted;
		Mine->ExpiryTime = Now + Lease;
	}
	else
	{
		FAngleClaim NewClaim;
		NewClaim.Attacker = Attacker;
		NewClaim.Angle = Granted;
		NewClaim.ExpiryTime = Now + Lease;
		Claims.Add(NewClaim);
	}

	return Granted;
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
