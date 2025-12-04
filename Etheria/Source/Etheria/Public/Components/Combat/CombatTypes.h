/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatTypes - Header"
 * Notes: Shared combat enums/structs/delegates for CombatComponent.
 */

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

class UAnimMontage;
class AActor;

#pragma region STRUCT & ENUM
/** Direction used for directional dodge selection. */
UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
    Forward  UMETA(DisplayName="Forward"),
    Backward UMETA(DisplayName="Backward"),
    Left     UMETA(DisplayName="Left"),
    Right    UMETA(DisplayName="Right")
};

/* Basic enums */
UENUM(BlueprintType) enum class EStance : uint8 { Both=0, GroundOnly=1, AirOnly=2 };
UENUM(BlueprintType) enum class EAttackType : uint8 { Melee=0, Ranged=1, AoE=2 };
UENUM(BlueprintType) enum class ETraceShape : uint8 { Line=0, Sphere=1, Capsule=2 };
UENUM(BlueprintType) enum class EPerfectKind : uint8 { None=0, Parry=1, Dodge=2 };
UENUM(BlueprintType) enum class ECombatCuePhase : uint8 { Start=0, Impact=1, End=2 };
UENUM(BlueprintType) enum class EChargeShape : uint8 { None=0, Radial=1, FrontalRect=2 };
UENUM(BlueprintType) enum class ERangedMode : uint8 { None=0, Line=1 };

/* Charge structs */
USTRUCT(BlueprintType)
struct FChargeLevelConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float Time = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float DamageMultiplier = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float RangeMultiplier = 1.2f;
};

USTRUCT(BlueprintType)
struct FChargeSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bChargeable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") EChargeShape Shape = EChargeShape::Radial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bShowTelegraph = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxRadius = 400.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxLength = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxWidth = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<FChargeLevelConfig> Levels;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<UAnimMontage*> ReleaseMontages;
};

/* Attack spec */
USTRUCT(BlueprintType)
struct FAttackSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName Group = NAME_None; // "Light", "Heavy"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") EStance Stance = EStance::GroundOnly;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") EAttackType AttackType = EAttackType::Melee;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float BaseDamage = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritChance = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritMultiplier = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitStartDelay = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitWindow = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float Cooldown = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") ETraceShape TraceShape = ETraceShape::Sphere;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Range = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Radius = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float CapsuleHalfHeight = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") UAnimMontage* Montage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") FName MontageSection = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") FChargeSpecConfig Charge;
};

/* Combo spec */
USTRUCT(BlueprintType)
struct FComboStepConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float DamageOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritChanceOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritMultiplierOverride = -1.f;
};

USTRUCT(BlueprintType)
struct FComboSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName ComboId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") TArray<FComboStepConfig> Steps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta=(ClampMin="0.1")) float ResetDelay = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta=(ClampMin="0.0")) float Cooldown = 0.0f;
};
#pragma endregion

/* Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnCue, FName, CueName, ECombatCuePhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnAttackEvent, FName, AttackId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitCritEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnPerfectEvent, EPerfectKind, PerfectKind);
