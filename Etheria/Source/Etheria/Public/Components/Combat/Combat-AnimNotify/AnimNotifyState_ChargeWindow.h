/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotifyState_ChargeWindow" - Header
 */
#pragma once
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ChargeWindow.generated.h"

/**
 * Drives charge begin, progress, and release.
 * The component integrates time from this state to compute charge level and telegraph scaling.
 */
UCLASS()
class ETHERIA_API UAnimNotifyState_ChargeWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    // Optional attack id to bind this charge to. If None, the last requested attack will be charged.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge")
    FName AttackId = NAME_None;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
