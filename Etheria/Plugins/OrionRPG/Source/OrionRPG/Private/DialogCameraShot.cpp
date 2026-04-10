// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogCameraShot.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_DialogLine.h"
#include <CineCameraActor.h>
#include <CineCameraComponent.h>
#include "GameFramework/PlayerController.h"

UDialogCameraShot::UDialogCameraShot()
{
	bUsePostProcess = false;
	CameraDistance = 100.f;
	FocalLengthOverride = 35.0f;
	ApertureOverride =  2.8f;
	ConeAngle = FVector2f(30.f, 3.f);
	bOverride_CustomNearClippingPlane = true;
	CustomNearClippingPlane = 15.f;

	CropSettings.AspectRatio = 2.39f;
	bConstrainAspectRatio = false;

	Filmback.SensorWidth = 23.76f;
	Filmback.SensorHeight = 13.365f;
	Filmback.SensorAspectRatio = 1.777778f;

	bBlendCameraTransition = false;
	BlendTime = 0.75f; 
	BlendFunc = VTBlend_Cubic; 
	BlendExp = 2.0f;

	LensSettings.MinFocalLength = 4.0f;
	LensSettings.MaxFocalLength = 1000.0f;
	LensSettings.MinFStop = 1.2f;
	LensSettings.MaxFStop = 22.0f;
	LensSettings.SqueezeFactor = 1.0f;
	LensSettings.DiaphragmBladeCount = 7;

}

void UDialogCameraShot::Play(UDialogBuilderGraph* InDialog, AActor* InSpeaker)
{
	if (!GetWorld() ||
        !InDialog)
		return;

	K2_PreCameraSetup();

	ACineCameraActor* Cinecam = InDialog->Cinecam.Get();	

	//Define Track & Focus rules
	bool bTrackEnabled = CameraTrackingRule == EDialogCameraTrackingRule::E_Disabled ? false : true;
	LookAtActor = CameraTrackingRule == EDialogCameraTrackingRule::E_OtherParticipant ? InDialog->CachedParticipants.FindRef(ParticipantToTrack).Get()
        : InSpeaker;
	
	FocusActor = CameraFocusRule == EDialogCameraFocusRule::E_TrackedActor ? LookAtActor
		: CameraFocusRule == EDialogCameraFocusRule::E_Speaker ? InSpeaker 
		: CameraFocusRule == EDialogCameraFocusRule::E_OtherParticipant ? InDialog->CachedParticipants.FindRef(ParticipantToFocus).Get()
		: nullptr;

    // Optionally, focus/track the speaker if LookAtActor is valid
    if (Cinecam && LookAtActor.IsValid())
    {
        // Replace this block in UDialogBuilderGraph::PlayDialogShot
        const FVector RelativeOffset = LookAtActor->GetActorTransform().InverseTransformPosition(InDialog->GetSpeakerBoneLocation(LookAtActor.Get(), bOverrideTrackedBone, OverrideTrackedBone));
        const FVector ActorLocation = LookAtActor->GetActorLocation() + RelativeOffset;
        const FRotator ActorRotation = LookAtActor->GetActorRotation();
        const FVector ForwardVector = ActorRotation.Vector();

		FVector LocationOverride;
		FRotator RotationOverride = FRotator();
		float RandomAngleX;
		float RandomAngleY;
		FRotator Rotation;
		FVector Direction;

		switch (CameraAngleRule)
		{
			case(EDialogCameraAngleRule::E_RandomCone):
				RandomAngleX = FMath::FRandRange(-ConeAngle.X, ConeAngle.X);
				RandomAngleY = FMath::FRandRange(-ConeAngle.Y, ConeAngle.Y);

				// Calculate random direction within cone
				Rotation = FRotator(RandomAngleY, ActorRotation.Yaw + RandomAngleX, 0.f);
				Direction = Rotation.Vector();

				LocationOverride = ActorLocation + Direction * CameraDistance;
				RotationOverride = (ActorLocation - LocationOverride).Rotation();
				break;
            case(EDialogCameraAngleRule::E_Custom):
				// Calculate direction based on angle
				Rotation = FRotator(CameraAngle.Y, ActorRotation.Yaw + CameraAngle.X, 0.f);
				Direction = Rotation.Vector();

				LocationOverride = ActorLocation + Direction * CameraDistance;
				RotationOverride = (ActorLocation - LocationOverride).Rotation();
            break;
		}

        //make sure to generate camera only if (shot get overidden || tracked bone get overidden)
        if ((InDialog->CachedLastShot != this) || 
			(bOverrideTrackedBone))
        {
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (bBlendCameraTransition)
			{

				if (PC)
				{
					//Spawn new cinecam so we can set view target from current camera to this cinecam
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ACineCameraActor* NewCinecam = GetWorld()->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(), SpawnParams);

					// Set the cinecam's transform before blending
					NewCinecam->SetActorLocation(LocationOverride);
					NewCinecam->SetActorRotation(RotationOverride);

					// Smoothly blend to the cinecam as view target
					PC->SetViewTargetWithBlend(
						const_cast<ACineCameraActor*>(NewCinecam),
						BlendTime,
						BlendFunc,
						BlendExp,
						true // bLockOutgoing
					);

					//override cinecam with new one
					InDialog->Cinecam = NewCinecam;
					Cinecam->SetLifeSpan(BlendTime);
					Cinecam = NewCinecam;

					// Wait for BlendTime, then execute lambda
					/*FTimerHandle BlendTimerHandle;
					GetWorld()->GetTimerManager().SetTimer(
						BlendTimerHandle,
						[this, InDialog, NewCinecam, bTrackEnabled, RelativeOffset]()
						{
							NewCinecam->LookatTrackingSettings.bEnableLookAtTracking = bTrackEnabled;
							NewCinecam->LookatTrackingSettings.ActorToTrack = LookAtActor.Get();
							NewCinecam->LookatTrackingSettings.RelativeOffset = RelativeOffset + CameraOffset;
						},
						BlendTime,
						false
					);*/
				}
			}
			else
			{
				// Set the current view target to the cinecam
				if (PC)
				{
					PC->SetViewTarget(const_cast<ACineCameraActor*>(Cinecam));
				}
				Cinecam->SetActorLocation(LocationOverride);
				Cinecam->SetActorRotation(RotationOverride);
				Cinecam->LookatTrackingSettings.bEnableLookAtTracking = bTrackEnabled;
				Cinecam->LookatTrackingSettings.ActorToTrack = LookAtActor.Get();
				Cinecam->LookatTrackingSettings.RelativeOffset = RelativeOffset + CameraOffset;
			}
			

        }

		//Set camera setting
		if (UCineCameraComponent* CinecamComp = Cinecam->GetCineCameraComponent())
		{
			CinecamComp->CropSettings = CropSettings;
			CinecamComp->SetCurrentFocalLength(FocalLengthOverride);
			CinecamComp->SetCurrentAperture(ApertureOverride);
			CinecamComp->SetFilmback(Filmback);
			CinecamComp->SetLensSettings(LensSettings);
			CinecamComp->SetConstraintAspectRatio(bConstrainAspectRatio);
			CinecamComp->bOverride_CustomNearClippingPlane = bOverride_CustomNearClippingPlane;
			CinecamComp->CustomNearClippingPlane = CustomNearClippingPlane;
			if (bUsePostProcess)
			{
				CinecamComp->PostProcessSettings = PostProcessSettings;
			}
			else
			{
				CinecamComp->PostProcessSettings = FPostProcessSettings();
			}


			if (FocusActor.IsValid())
			{
				CinecamComp->FocusSettings.FocusMethod = ECameraFocusMethod::Tracking;
				CinecamComp->FocusSettings.TrackingFocusSettings.RelativeOffset = FocusRelativeOffset;
				CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack = FocusActor.Get();
			}
			else
			{
				CinecamComp->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
			}
		}

		

    }
}
