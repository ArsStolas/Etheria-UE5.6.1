/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotifyState_InputLock" - Header
 */
#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_InputLock.generated.h"

class UCombatComponent;

UCLASS(meta=(DisplayName="Input Lock Window"))
class ETHERIA_API UAnimNotifyState_InputLock : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="InputLock")
    FName LockId = NAME_None;

    UPROPERTY(EditAnywhere, Category="InputLock")
    bool bBlockJump = true;

    UPROPERTY(EditAnywhere, Category="InputLock")
    bool bBlockCrouch = true;

    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("InputLock");
    }

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

private:
    FName RuntimeLockId;
};