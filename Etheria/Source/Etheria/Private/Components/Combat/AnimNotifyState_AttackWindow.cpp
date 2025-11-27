/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotifyState_AttackWindow" - Source
 */
#include "Components/Combat/AnimNotifyState_AttackWindow.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotifyState_AttackWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;
    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
        {
            Combat->BeginAttackWindow();
        }
    }
}

void UAnimNotifyState_AttackWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp) return;
    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
        {
            Combat->EndAttackWindow();
        }
    }
}
