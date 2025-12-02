/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotifyState_AttackWindow" - Header
 */
#pragma once
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AttackWindow.generated.h"

class UCombatComponent;

UCLASS()
class ETHERIA_API UAnimNotifyState_AttackWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
