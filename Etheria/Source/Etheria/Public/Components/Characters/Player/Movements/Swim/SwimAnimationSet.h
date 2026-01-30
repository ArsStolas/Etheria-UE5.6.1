/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimAnimationSet" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SwimAnimationSet.generated.h"

class UAnimMontage;
class UBlendSpace;

/**
 * DataAsset used to configure swim animations from the Editor.
 * 
 * You can either:
 * - Use BlendSpaces (SurfaceBlendSpace / UnderwaterBlendSpace) for locomotion, OR
 * - Use Directional Montages (Idle/Forward/Backward/Left/Right + Sprint variants) and let the AnimInstance auto-play them.
 */
UCLASS(BlueprintType)
class ETHERIA_API USwimAnimationSet : public UDataAsset
{
	GENERATED_BODY()

public:
	// ============================================================
	// BlendSpaces (optional)
	// ============================================================
	/** Surface swim BlendSpace (optional). If set, montages are not auto-played for surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|BlendSpaces", meta=(ToolTip="Optional surface swim BlendSpace. If set, directional montages won't auto-play for surface."))
	TObjectPtr<UBlendSpace> SurfaceBlendSpace = nullptr;

	/** Underwater swim BlendSpace (optional). If set, montages are not auto-played for underwater. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|BlendSpaces", meta=(ToolTip="Optional underwater swim BlendSpace. If set, directional montages won't auto-play for underwater."))
	TObjectPtr<UBlendSpace> UnderwaterBlendSpace = nullptr;

	// ============================================================
	// Surface Directional Montages (optional)
	// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface idle loop montage."))
	TObjectPtr<UAnimMontage> SurfaceIdleMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface forward swim loop montage."))
	TObjectPtr<UAnimMontage> SurfaceForwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface backward swim loop montage."))
	TObjectPtr<UAnimMontage> SurfaceBackwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface left strafe swim loop montage."))
	TObjectPtr<UAnimMontage> SurfaceLeftMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface right strafe swim loop montage."))
	TObjectPtr<UAnimMontage> SurfaceRightMontage = nullptr;

	// Sprint variants (optional)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface sprint forward loop montage (optional)."))
	TObjectPtr<UAnimMontage> SurfaceSprintForwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface sprint backward loop montage (optional)."))
	TObjectPtr<UAnimMontage> SurfaceSprintBackwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface sprint left loop montage (optional)."))
	TObjectPtr<UAnimMontage> SurfaceSprintLeftMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Surface|Montages", meta=(ToolTip="Surface sprint right loop montage (optional)."))
	TObjectPtr<UAnimMontage> SurfaceSprintRightMontage = nullptr;

	// ============================================================
	// Underwater Directional Montages (optional)
	// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater idle loop montage."))
	TObjectPtr<UAnimMontage> UnderwaterIdleMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater forward swim loop montage."))
	TObjectPtr<UAnimMontage> UnderwaterForwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater backward swim loop montage."))
	TObjectPtr<UAnimMontage> UnderwaterBackwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater left strafe swim loop montage."))
	TObjectPtr<UAnimMontage> UnderwaterLeftMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater right strafe swim loop montage."))
	TObjectPtr<UAnimMontage> UnderwaterRightMontage = nullptr;

	// Sprint variants (optional)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater sprint forward loop montage (optional)."))
	TObjectPtr<UAnimMontage> UnderwaterSprintForwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater sprint backward loop montage (optional)."))
	TObjectPtr<UAnimMontage> UnderwaterSprintBackwardMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater sprint left loop montage (optional)."))
	TObjectPtr<UAnimMontage> UnderwaterSprintLeftMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Underwater|Montages", meta=(ToolTip="Underwater sprint right loop montage (optional)."))
	TObjectPtr<UAnimMontage> UnderwaterSprintRightMontage = nullptr;

	// ============================================================
	// Transitions (optional)
	// ============================================================
	/** Played when diving from Surface to Underwater (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Transitions", meta=(ToolTip="Optional montage played when diving from surface to underwater."))
	TObjectPtr<UAnimMontage> DiveFromSurfaceMontage = nullptr;

	/** Played when rising from Underwater to Surface (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Transitions", meta=(ToolTip="Optional montage played when rising from underwater to surface."))
	TObjectPtr<UAnimMontage> RiseToSurfaceMontage = nullptr;
};
