/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "WeaponData" - Header
 */
#pragma once

#include "Engine/DataAsset.h"
// Forward include to reuse combat structs in the data asset
// We include the header here because this data asset only stores data types declared there.
#include "Components/Combat/CombatComponent.h"
#include "WeaponData.generated.h"

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
    TArray<FEEAttackSpec> Attacks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combos")
    TArray<FEEComboSpec> Combos;

    // Optional tuning overrides
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning")
    TEnumAsByte<ECollisionChannel> DamageTraceChannelOverride = ECC_MAX; // ECC_MAX means "do not override"

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Tuning")
    bool bOverrideMagnetism = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bOverrideMagnetism"), Category="Weapon|Tuning")
    float MagnetismAngleDeg = 35.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bOverrideMagnetism"), Category="Weapon|Tuning")
    float MagnetismStrength = 0.55f;
};