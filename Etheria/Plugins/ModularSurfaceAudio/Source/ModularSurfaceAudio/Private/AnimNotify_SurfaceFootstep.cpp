/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UAnimNotify_SurfaceFootstep" - Source
 */

#include "AnimNotify_SurfaceFootstep.h"

#include "SurfaceAudioComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotify_SurfaceFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp)
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    USurfaceAudioComponent* Comp = Owner->FindComponentByClass<USurfaceAudioComponent>();
    if (!Comp)
    {
        return;
    }

    Comp->PlayFootstepFromNotify(Foot, Gait, FootSocketName, bTryIKProvider);
}
