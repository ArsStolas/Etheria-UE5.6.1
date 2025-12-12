/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "AnimNotify_ApplyCombatImpulse" - Header
 */
#pragma once
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ApplyCombatImpulse.generated.h"

UENUM(BlueprintType)
enum class EEEImpulseTarget : uint8 { Self=0, Victims=1, Both=2 };

UENUM(BlueprintType)
enum class EEEImpulseDir : uint8 { Up=0, Down=1, Forward=2, Custom=3 };

/**
 * Notifies an impulse to attacker and/or last hit victims.
 */
UCLASS()
class ETHERIA_API UAnimNotify_ApplyCombatImpulse : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Impulse") EEEImpulseTarget Target = EEEImpulseTarget::Both;
    UPROPERTY(EditAnywhere, Category="Impulse") EEEImpulseDir Direction = EEEImpulseDir::Up;
    UPROPERTY(EditAnywhere, Category="Impulse", meta=(ClampMin="0.0")) float Magnitude = 600.f;
    UPROPERTY(EditAnywhere, Category="Impulse") FVector CustomDirection = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Impulse") bool bVictimsUseLaunchCharacter = true;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("CombatImpulse"); }
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};