/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimBlueprintLibrary" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SwimComponent.h"
#include "SwimBlueprintLibrary.generated.h"

class ACharacter;

/**
 * Blueprint helpers for the swim system.
 * Designed to be used in AnimBP or any gameplay BP without adding custom C++.
 */
UCLASS()
class ETHERIA_API USwimBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region WaterHelpers
	/** Returns true if Character is currently inside a Water PhysicsVolume (bWaterVolume). */
	UFUNCTION(BlueprintPure, Category="Swim|Helpers")
	static bool IsCharacterInWaterVolume(const ACharacter* Character);

	/**
	 * Floor distance check below the character capsule.
	 * Useful for debugging thresholds and animation transitions.
	 */
	UFUNCTION(BlueprintCallable, Category="Swim|Helpers")
	static bool GetCharacterFloorDistance(const ACharacter* Character, float TraceDepth, TEnumAsByte<ECollisionChannel> TraceChannel, float& OutFloorDistance);
#pragma endregion

#pragma region DirectionHelpers
	/** Camera-based forward direction. Underwater can use camera pitch (3D forward). */
	UFUNCTION(BlueprintPure, Category="Swim|Helpers")
	static FVector GetSwimForwardDirection(const ACharacter* Character, bool bUsePitch);

	/** Camera-based right direction (yaw only). */
	UFUNCTION(BlueprintPure, Category="Swim|Helpers")
	static FVector GetSwimRightDirection(const ACharacter* Character);

	/**
	 * Apply camera-based movement input for swimming.
	 * You can use this instead of letting the component auto-apply cached inputs.
	 */
	UFUNCTION(BlueprintCallable, Category="Swim|Helpers")
	static void ApplySwimMovementInput(ACharacter* Character, float ForwardAxis, float RightAxis, bool bUsePitchForForward);
#pragma endregion

#pragma region ModeHelpers
	/** Converts swim mode enum to a readable name (debug/UI). */
	UFUNCTION(BlueprintPure, Category="Swim|Helpers")
	static FName SwimModeToName(EPlayerSwimMode Mode);
#pragma endregion
};
