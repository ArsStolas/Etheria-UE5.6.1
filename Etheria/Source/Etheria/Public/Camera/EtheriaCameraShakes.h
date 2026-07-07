/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EtheriaCameraShakes" - Header
 * Notes: Code-driven camera shakes (no asset required). Tuned in C++ constructors,
 *        but each class can still be subclassed in Blueprint to tweak values.
 */

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "EtheriaCameraShakes.generated.h"

/** Short, snappy shake for regular melee/ranged hits. */
UCLASS()
class ETHERIA_API UEtheriaCameraShake_HitLight : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UEtheriaCameraShake_HitLight(const FObjectInitializer& ObjectInitializer);
};

/** Heavier, longer shake for critical hits. */
UCLASS()
class ETHERIA_API UEtheriaCameraShake_HitHeavy : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UEtheriaCameraShake_HitHeavy(const FObjectInitializer& ObjectInitializer);
};

/** Vertical thud played when the character lands after a big fall. */
UCLASS()
class ETHERIA_API UEtheriaCameraShake_Land : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UEtheriaCameraShake_Land(const FObjectInitializer& ObjectInitializer);
};
