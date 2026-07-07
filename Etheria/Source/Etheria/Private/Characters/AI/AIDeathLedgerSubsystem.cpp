/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "AIDeathLedgerSubsystem - Source"
 */

#include "Characters/AI/AIDeathLedgerSubsystem.h"

#include "GameFramework/Actor.h"

void UAIDeathLedgerSubsystem::RegisterDeath(const AActor* Actor, EAIRespawnCondition Condition, float RespawnDelay)
{
	if (!Actor) return;
	FAIDeathRecord& R = Deaths.FindOrAdd(Actor->GetPathName());
	R.DeathTime = FPlatformTime::Seconds();
	R.Condition = Condition;
	R.RespawnDelay = RespawnDelay;
}

bool UAIDeathLedgerSubsystem::ShouldStayDead(const AActor* Actor)
{
	if (!Actor) return false;
	const FString Key = Actor->GetPathName();
	const FAIDeathRecord* R = Deaths.Find(Key);
	if (!R) return false;

	if (R->Condition == EAIRespawnCondition::OnTimer)
	{
		if (FPlatformTime::Seconds() >= R->DeathTime + R->RespawnDelay)
		{
			Deaths.Remove(Key);
			return false;
		}
	}
	return true;
}

void UAIDeathLedgerSubsystem::ClearDeath(const AActor* Actor)
{
	if (Actor) Deaths.Remove(Actor->GetPathName());
}

void UAIDeathLedgerSubsystem::FlushDeathsForCondition(EAIRespawnCondition Condition)
{
	for (auto It = Deaths.CreateIterator(); It; ++It)
	{
		const EAIRespawnCondition C = It->Value.Condition;
		const bool bMatch = (C == Condition)
			|| (C == EAIRespawnCondition::OnSaveOrDay
				&& (Condition == EAIRespawnCondition::OnSave || Condition == EAIRespawnCondition::OnDayCycle));
		if (bMatch) It.RemoveCurrent();
	}
}

void UAIDeathLedgerSubsystem::ResetLedger()
{
	Deaths.Empty();
}
