/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemAttackNotifies - Source"
 */

#include "Characters/AI/Boss/GolemAttackNotifies.h"

#include "Characters/AI/Boss/GolemBossComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UGolemBossComponent* FindGolem(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UGolemBossComponent>() : nullptr;
	}
}

void UAnimNotify_GolemStrike::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (UGolemBossComponent* Golem = FindGolem(MeshComp)) Golem->TriggerStrikeNow();
}

void UAnimNotify_GolemThrowRock::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (UGolemBossComponent* Golem = FindGolem(MeshComp)) Golem->LaunchRockNow();
}

void UAnimNotify_GolemEndAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (UGolemBossComponent* Golem = FindGolem(MeshComp)) Golem->EndAttackNow();
}

void UAnimNotify_GolemCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (UGolemBossComponent* Golem = FindGolem(MeshComp)) Golem->FireAnimCue(CueTag);
}
