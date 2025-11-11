/**
* Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UAnimNotifyState_InputLock" - Source
 */
#include "Components/Combat/AnimNotifyState_InputLock.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"

static FName MakeFallbackLockId(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    const FString OwnerName = MeshComp && MeshComp->GetOwner() ? MeshComp->GetOwner()->GetName() : TEXT("Owner");
    const FString AnimName  = Animation ? Animation->GetName() : TEXT("Anim");
    return FName(*FString::Printf(TEXT("InputLock_%s_%s"), *OwnerName, *AnimName));
}

void UAnimNotifyState_InputLock::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
    if (!MeshComp) return;
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
    {
        RuntimeLockId = (LockId != NAME_None) ? LockId : MakeFallbackLockId(MeshComp, Animation);
        Combat->PushInputLock(RuntimeLockId, bBlockJump, bBlockCrouch);
    }
}

void UAnimNotifyState_InputLock::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
    {
        const FName IdToPop = (RuntimeLockId != NAME_None) ? RuntimeLockId : (LockId != NAME_None ? LockId : MakeFallbackLockId(MeshComp, Animation));
        Combat->PopInputLock(IdToPop);
        RuntimeLockId = NAME_None;
    }
}