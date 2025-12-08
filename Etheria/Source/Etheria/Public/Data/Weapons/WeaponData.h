/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "WeaponData" - Header
 */
#pragma once

#include "Engine/DataAsset.h"
// Forward include to reuse combat structs in the data asset
// We include the header here because this data asset only stores data types declared there.
#include "Components/Combat/CombatComponent.h"
#include "WeaponData.generated.h"

/**
 * Kind of ranged weapon behavior.
 * Used to drive aiming and firing logic (bow vs semi-auto vs full-auto).
 */
UENUM(BlueprintType)
enum class EWeaponRangedType : uint8
{
    None      UMETA(DisplayName="None"),
    Bow       UMETA(DisplayName="Bow (Hold & Release)"),
    SemiAuto  UMETA(DisplayName="Semi-auto"),
    FullAuto  UMETA(DisplayName="Full-auto")
};

/**
 * Ranged-specific configuration for a weapon.
 * This allows the same combat component to handle bows and guns differently.
 */
USTRUCT(BlueprintType)
struct FWeaponRangedConfig
{
    GENERATED_BODY()

    // ---- General ----

    // If false, this weapon is considered melee-only and aiming should be ignored.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged")
    bool bIsRangedWeapon = false;

    // High-level behavior of the ranged weapon (Bow / SemiAuto / FullAuto).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged")
    EWeaponRangedType WeaponKind = EWeaponRangedType::None;

    // ---- Camera / movement while aiming ----

    // Desired arm length when aiming (ADS / bow draw).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Camera")
    float AimArmLength = 220.f;

    // Desired camera FOV when aiming.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Camera")
    float AimFOV = 70.f;

    // Movement speed multiplier while aiming (applied to base walk speed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Camera", meta=(ClampMin="0.1", ClampMax="1.0"))
    float AimMoveSpeedMultiplier = 0.6f;

    // ---- Attack mapping ----

    // Attack group used when hip-firing (no aim). Usually "Light".
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Attacks")
    FName HipFireGroup = FName("Light");

    // AttackId used when firing while aiming (bow shot / ADS shot).
    // This should match an AttackId from the Attacks array.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Attacks")
    FName AimAttackId = FName("AimShot");

    // If true, aim-shot uses the generic charge system (hold & release).
    // For a bow: set this to true and configure Charge on the corresponding attack.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|Charge")
    bool bUseChargeOnAim = false;

    // ---- Full-auto tuning ----

    // Shots per second when holding fire in full-auto mode.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranged|FullAuto",
        meta=(EditCondition="WeaponKind == EWeaponRangedType::FullAuto", EditConditionHides, ClampMin="1.0"))
    float FullAutoRate = 5.f;
};

/**
 * Data asset that defines the full attack and combo configuration for a weapon.
 * This allows swapping weapon behavior without touching the component.
 */
UCLASS(BlueprintType)
class ETHERIA_API UWeaponData : public UDataAsset
{
    GENERATED_BODY()
public:
    // Id for quick filtering or debugging (optional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    FName WeaponId = NAME_None;

    // Attacks and combos to push into the combat component on apply
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Attacks")
    TArray<FAttackSpecConfig> Attacks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combos")
    TArray<FComboSpecConfig> Combos;

    // Ranged configuration (bow / semi-auto / full-auto)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Ranged")
    FWeaponRangedConfig Ranged;

    // Optional tuning overrides
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning")
    bool bOverrideMagnetism = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning", meta=(EditCondition="bOverrideMagnetism"))
    float MagnetismAngleDeg = 35.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning", meta=(EditCondition="bOverrideMagnetism"))
    float MagnetismStrength = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning")
    TEnumAsByte<ECollisionChannel> DamageTraceChannelOverride = ECC_MAX; // ECC_MAX means "do not override"
};
