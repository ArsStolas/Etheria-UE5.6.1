/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotify_CombatCue" - Header
 */
#pragma once
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CombatCue.generated.h"

UCLASS()
class ETHERIA_API UAnimNotify_CombatCue : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    FName CueName = "Cue";

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
