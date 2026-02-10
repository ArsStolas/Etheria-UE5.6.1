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
enum class EEEImpulseTarget : uint8
{
    Self    = 0,
    Victims = 1,
    Both    = 2
};

UENUM(BlueprintType)
enum class EEEImpulseDir : uint8
{
    Up = 0,
    Down = 1,

    Forward = 2,
    Backward = 3,
    Right = 4,
    Left = 5,

    ForwardRight = 6,
    ForwardLeft = 7,
    BackwardRight = 8,
    BackwardLeft = 9,

    /** Uses movement input direction (last/pending input), optional 8-way quantize. */
    Input = 10,

    Custom = 11
};

UENUM(BlueprintType)
enum class EEEImpulseInputBasis : uint8
{
    /** Quantization/orientation based on actor forward/right */
    Actor = 0,

    /** Quantization/orientation based on Controller yaw (camera yaw) */
    ControllerYaw = 1
};

/**
 * Notifies an impulse to attacker and/or last hit victims.
 */
UCLASS()
class ETHERIA_API UAnimNotify_ApplyCombatImpulse : public UAnimNotify
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Impulse")
    EEEImpulseTarget Target = EEEImpulseTarget::Both;

    UPROPERTY(EditAnywhere, Category="Impulse")
    EEEImpulseDir Direction = EEEImpulseDir::Up;

    UPROPERTY(EditAnywhere, Category="Impulse", meta=(ClampMin="0.0"))
    float Magnitude = 600.f;

    UPROPERTY(EditAnywhere, Category="Impulse", meta=(EditCondition="Direction == EEEImpulseDir::Custom"))
    FVector CustomDirection = FVector::ZeroVector;

    /** If true, removes Z from direction for non vertical modes (great for ground dodges). */
    UPROPERTY(EditAnywhere, Category="Impulse|Advanced")
    bool bProjectToXYPlane = false;

    /** LaunchCharacter overrides */
    UPROPERTY(EditAnywhere, Category="Impulse|Launch")
    bool bOverrideXY = true;

    UPROPERTY(EditAnywhere, Category="Impulse|Launch")
    bool bOverrideZ = true;

    /** Victim delivery method */
    UPROPERTY(EditAnywhere, Category="Impulse|Victims")
    bool bVictimsUseLaunchCharacter = true;

    /** Input options (only used when Direction == Input) */
    UPROPERTY(EditAnywhere, Category="Impulse|Input", meta=(EditCondition="Direction == EEEImpulseDir::Input"))
    EEEImpulseInputBasis InputBasis = EEEImpulseInputBasis::ControllerYaw;

    UPROPERTY(EditAnywhere, Category="Impulse|Input", meta=(EditCondition="Direction == EEEImpulseDir::Input"))
    bool bQuantizeInputTo8Directions = true;

    UPROPERTY(EditAnywhere, Category="Impulse|Input", meta=(EditCondition="Direction == EEEImpulseDir::Input", ClampMin="0.0"))
    float InputDeadzone = 0.15f;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("CombatImpulse"); }
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
