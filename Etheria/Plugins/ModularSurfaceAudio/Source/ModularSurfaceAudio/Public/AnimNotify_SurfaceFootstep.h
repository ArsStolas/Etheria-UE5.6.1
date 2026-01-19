/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UAnimNotify_SurfaceFootstep" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SurfaceAudioTypes.h"
#include "AnimNotify_SurfaceFootstep.generated.h"

UCLASS(meta=(DisplayName="Surface Footstep"))
class MODULARSURFACEAUDIO_API UAnimNotify_SurfaceFootstep : public UAnimNotify
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
    EFootstepFoot Foot = EFootstepFoot::Left;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
    EFootstepGait Gait = EFootstepGait::Walk;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
    FName FootSocketName = "foot_l";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
    bool bTryIKProvider = false;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
