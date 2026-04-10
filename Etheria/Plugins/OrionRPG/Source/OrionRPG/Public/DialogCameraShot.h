// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include <MovieSceneSequencePlayer.h>
#include <CineCameraSettings.h>
#include "LevelSequencePlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "DialogCameraShot.generated.h"

UENUM(BlueprintType)
enum class EDialogCameraAngleRule : uint8
{
	/**Generate random angle in a cone*/
	E_RandomCone				UMETA(DisplayName = "Random Cone"),
	/**Custom angle*/
	E_Custom 					UMETA(DisplayName = "Custom Angle")
};

UENUM(BlueprintType)
enum class EDialogCameraFocusRule : uint8
{
	/**disable focus*/
	E_Disabled				UMETA(DisplayName = "Disabled"),
	/**Try to focus the current tracked actor, will fall back to disabled state if there is no tracked actor*/
	E_TrackedActor			UMETA(DisplayName = "Tracked Actor"),
	/**focus the speaker*/
	E_Speaker				UMETA(DisplayName = "Speaker"),
	/**focus other participant*/
	E_OtherParticipant 		UMETA(DisplayName = "Other Participant")
};


UENUM(BlueprintType)
enum class EDialogCameraTrackingRule : uint8
{
	/**disable tracking*/
	E_Disabled				UMETA(DisplayName = "Disabled"),
	/**Track the speaker*/
	E_Speaker				UMETA(DisplayName = "Speaker"),
	/**Track other participant*/
	E_OtherParticipant 		UMETA(DisplayName = "Other Participant")
};

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, AutoExpandCategories = "Detail")
class ORIONRPG_API UDialogCameraShot : public UObject {
	GENERATED_BODY()

public:
	UDialogCameraShot();


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Angle Rule")
	EDialogCameraAngleRule CameraAngleRule = EDialogCameraAngleRule::E_RandomCone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Angle Rule", meta = (EditCondition = "CameraAngleRule == EDialogCameraAngleRule::E_RandomCone", HideEditConditionToggle, EditConditionHides))
	FVector2f ConeAngle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Angle Rule", meta = (EditCondition = "CameraAngleRule == EDialogCameraAngleRule::E_Custom", HideEditConditionToggle, EditConditionHides))
	FVector2f CameraAngle;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Transition")
	bool bBlendCameraTransition;

	/**time taken to blend*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Transition", meta = (EditCondition = "bBlendCameraTransition == true", HideEditConditionToggle, EditConditionHides))
	float BlendTime;

	/** Cubic, Linear etc functions for blending*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Transition", meta = (EditCondition = "bBlendCameraTransition == true", HideEditConditionToggle, EditConditionHides))
	TEnumAsByte<EViewTargetBlendFunction> BlendFunc;

	/**Exponent, used by certain blend functions to control the shape of the curve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Transition", meta = (EditCondition = "bBlendCameraTransition == true", HideEditConditionToggle, EditConditionHides))
	float BlendExp;

	UPROPERTY(EditAnywhere, Category = "Override", meta = (InlineEditConditionToggle))
	bool bOverrideTrackedBone = false;

	/*
	* Override the default bone to track.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Track & Focus Settings", meta = (EditCondition = "bOverrideTrackedBone"))
	FName OverrideTrackedBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings")
	EDialogCameraTrackingRule CameraTrackingRule = EDialogCameraTrackingRule::E_Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings", meta = (EditCondition = "CameraTrackingRule == EDialogCameraTrackingRule::E_OtherParticipant", HideEditConditionToggle, EditConditionHides, Categories = "Dialog.Participant"))
	FGameplayTag ParticipantToTrack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings")
	EDialogCameraFocusRule CameraFocusRule = EDialogCameraFocusRule::E_TrackedActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings", meta = (EditCondition = "CameraFocusRule == EDialogCameraFocusRule::E_OtherParticipant", HideEditConditionToggle, EditConditionHides, Categories = "Dialog.Participant"))
	FGameplayTag ParticipantToFocus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings", meta = (EditCondition = "CameraFocusRule != EDialogCameraFocusRule::E_Disabled", HideEditConditionToggle, EditConditionHides))
	FVector FocusRelativeOffset;

	/**Cinecam distance from the track location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FVector CameraOffset;

	/** Current focal length of the camera (i.e. controls FoV, zoom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float FocalLengthOverride;

	/** Current aperture, in terms of f-stop (e.g. 2.8 for f/2.8) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float ApertureOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Camera Settings", meta = (InlineEditConditionToggle))
	bool bOverride_CustomNearClippingPlane;

	/** Set bOverride_CustomNearClippingPlane to true if you want to use a custom clipping plane instead of GNearClippingPlane. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (UIMin = "0.00001", ClampMin = "0.00001", editcondition = "bOverride_CustomNearClippingPlane"))
	float CustomNearClippingPlane;

	/** If bConstrainAspectRatio is true, black bars will be added if the destination view has a different aspect ratio than this camera requested. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (InlineEditConditionToggle))
	bool bConstrainAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (EditCondition = "bConstrainAspectRatio"))
	struct FPlateCropSettings CropSettings;



	/** Controls the filmback of the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FCameraFilmbackSettings Filmback;

	/** Controls the camera's lens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FCameraLensSettings LensSettings;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle), Category = "Camera Settings")
	bool bUsePostProcess = false;

	/**Camera post process override*/
	UPROPERTY(Interp, BlueprintReadWrite, Category = "Camera Settings", meta = (EditCondition = "bUsePostProcess"))
	struct FPostProcessSettings PostProcessSettings;

	UPROPERTY(BlueprintReadOnly, Category = "Sequence")
	TWeakObjectPtr<class AActor> LookAtActor;

	UPROPERTY(BlueprintReadOnly, Category = "Sequence")
	TWeakObjectPtr<class AActor> FocusActor;

public:
	UFUNCTION(BlueprintCallable, Category = "DialogCameraShot")
	virtual void Play(class UDialogBuilderGraph* InDialog, AActor* InSpeaker);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "PreCameraSetup", Category = "Event")
	void K2_PreCameraSetup();

protected:
	// Allows the Object to get a valid UWorld from it's outer.
	virtual UWorld* GetWorld() const override
	{
		if (HasAllFlags(RF_ClassDefaultObject))
		{
			// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
			return nullptr;
		}

		UObject* Outer = GetOuter();

		while (Outer)
		{
			UWorld* World = Outer->GetWorld();
			if (World)
			{
				return World;
			}

			Outer = Outer->GetOuter();
		}

		return nullptr;
	}

};

