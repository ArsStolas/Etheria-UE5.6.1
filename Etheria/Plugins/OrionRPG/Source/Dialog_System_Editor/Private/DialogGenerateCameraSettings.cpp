// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogGenerateCameraSettings.h"
#include "DialogSequenceShot.h"
#include "DialogBuilderSetting.h"

UDialogGenerateCameraSettings::UDialogGenerateCameraSettings()
{
	bRandomizeCameraPresets = true;
	bRandomizeCameraAngles = true;
	bOverridePlaybackRangeStart = false;
	bOverridePlaybackRangeEnd = false;
}


void UDialogGenerateCameraSettings::Initialize()
{
	if (DefaultShots.Num() == 0)
	{
		UDialogBuilderSetting* DialogBuilderSetting = GetMutableDefault<UDialogBuilderSetting>();

		for (auto& Shot : DialogBuilderSetting->DefaultCameraPresetsToGenerate)
		{
			if (Shot)
			{
				DefaultShots.AddUnique(NewObject<UDialogSequenceShot>(this, Shot));
			}
		}
	}
}