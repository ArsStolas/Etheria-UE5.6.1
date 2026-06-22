// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include <MovieSceneSequencePlayer.h>
#include <CineCameraSettings.h>
#include "LevelSequencePlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "DialogSequenceShot.generated.h"


UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, AutoExpandCategories = "Detail")
class ORIONRPG_API UDialogSequenceShot : public UObject {
	GENERATED_BODY()

public:
	UDialogSequenceShot();

	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	TObjectPtr<class UDialogBuilderGraph> OwningDialogGraph;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings")
	bool bDrawTargetFocus = false;

	UPROPERTY(BlueprintReadWrite, Category = "Track & Focus Settings", meta = (Categories = "Dialog.Participant"))
	class UDialogDefinition* ActorToFocus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track & Focus Settings")
	bool bFocusCameraOnActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Track & Focus Settings")
	FName TrackedBone;

	/**Cinecam distance from the track location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraDistance;

	/**Offset for camera shot*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FVector CameraOffset;

	/**Offset for target the camera focus on*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FVector TargetOffset;


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

