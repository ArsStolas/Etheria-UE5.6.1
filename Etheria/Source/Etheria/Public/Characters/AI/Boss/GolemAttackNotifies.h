/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemAttackNotifies - Header"
 * Notes: AnimNotifies that drive the Golem's custom attacks straight from its montages (frame-perfect),
 *        instead of relying only on the per-attack WindupDuration/RecoveryDuration timers. Drop them on
 *        an attack montage; each finds the UGolemBossComponent on the animated actor — no BP wiring.
 *          - Golem Throw Rock : the frame the hand lets go      -> LaunchRockNow()
 *          - Golem Strike     : the impact frame                -> TriggerStrikeNow()
 *          - Golem End Attack : the last frame of the montage   -> EndAttackNow()
 *          - Golem Cue        : any other marker (anim start, anticipation, enrage roar, footstep…) carrying
 *                               a designer Tag -> FireAnimCue(Tag) -> the OnGolemAnimCue dispatcher (skin in BP).
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GolemAttackNotifies.generated.h"

/** Impact frame: fire the strike now (damage + OnGolemStrike), regardless of WindupDuration. */
UCLASS(meta = (DisplayName = "Golem Strike (impact)"))
class ETHERIA_API UAnimNotify_GolemStrike : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Golem Strike"); }
};

/** Throw-release frame: detach the rock held in the hand and throw it toward the impact. */
UCLASS(meta = (DisplayName = "Golem Throw Rock (release)"))
class ETHERIA_API UAnimNotify_GolemThrowRock : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Golem Throw Rock"); }
};

/** Last frame: end the attack now (skip the remaining recovery timer). */
UCLASS(meta = (DisplayName = "Golem End Attack"))
class ETHERIA_API UAnimNotify_GolemEndAttack : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Golem End Attack"); }
};

/** Generic anim marker: broadcasts OnGolemAnimCue(CueTag) so BP can react (anim start, anticipation, enrage roar, footstep…). */
UCLASS(meta = (DisplayName = "Golem Cue (marker)"))
class ETHERIA_API UAnimNotify_GolemCue : public UAnimNotify
{
	GENERATED_BODY()
public:
	/** Designer tag identifying this cue (e.g. "WindupStart", "Anticipation", "Enrage", "Footstep"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem") FName CueTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override
	{
		return CueTag.IsNone() ? TEXT("Golem Cue") : FString::Printf(TEXT("Golem Cue: %s"), *CueTag.ToString());
	}
};
