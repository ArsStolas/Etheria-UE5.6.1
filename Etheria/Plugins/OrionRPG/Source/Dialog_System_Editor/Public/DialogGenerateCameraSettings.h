#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KeyParams.h"
#include "DialogGenerateCameraSettings.generated.h"

UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogGenerateCameraSettings : public UObject
{
	GENERATED_BODY()

public:
	UDialogGenerateCameraSettings();

	void Initialize();

public:
	UPROPERTY(EditAnywhere, Category = "Sequence")
	EMovieSceneKeyInterpolation KeyInterpolation = EMovieSceneKeyInterpolation::Constant;

	UPROPERTY(BlueprintReadOnly, Category = "Sequence", meta = (InlineEditConditionToggle))
	bool bOverridePlaybackRangeStart;

	UPROPERTY(EditAnywhere, Category = "Sequence", meta = (UIMin = "0.0", ClampMin = "0.0", EditCondition = "bOverridePlaybackRangeStart"))
	float PlaybackRangeStart;

	UPROPERTY(BlueprintReadOnly, Category = "Sequence", meta = (InlineEditConditionToggle))
	bool bOverridePlaybackRangeEnd;

	UPROPERTY(EditAnywhere, Category = "Sequence", meta = (UIMin = "0.0", ClampMin = "0.0", EditCondition = "bOverridePlaybackRangeEnd"))
	float PlaybackRangeEnd;

	/** Randomize camera angle for each preset
	* This will provide random variation for each shot
	*/
	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bRandomizeCameraAngles;

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bRandomizeCameraPresets;

	UPROPERTY(Transient, EditAnywhere, Instanced, Category = "Camera", meta = (EditCondition = "bRandomizeCameraPresets == false", HideEditConditionToggle, EditConditionHides))
	TObjectPtr<class UDialogSequenceShot> ShotToUse;

	UPROPERTY(Transient, EditAnywhere, Instanced, Category = "Camera", meta = (EditCondition = "bRandomizeCameraPresets == true", HideEditConditionToggle, EditConditionHides))
	TArray<TObjectPtr<class UDialogSequenceShot>> DefaultShots;



};