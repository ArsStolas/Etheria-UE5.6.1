// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Event/OrionEvent.h"
#include "PlaySound.generated.h"

UENUM(BlueprintType)
enum class EPlaySoundType : uint8
{
	E_SoundLocation		UMETA(DisplayName = "Play Sound At Location"),
	E_Sound2D	UMETA(DisplayName = "Play Sound 2D"),
};

/**
 * Play sound from sound base asset. 
 * You can choose to spawn sound in 2d or at location.
 */
UCLASS()
class ORIONRPG_API UPlaySound : public UOrionEvent
{
	GENERATED_BODY()
public:
	UPlaySound();

	// Sound type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	EPlaySoundType SoundType;

	// Sound to play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<class USoundBase> SoundToPlay;

	// Volume multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	float VolumeMultiplier;

	// Pitch multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	float PitchMultiplier;

	//Play Sound at location of the attached pawn + offset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "SoundType == EPlaySoundType::E_SoundLocation", HideEditConditionToggle, EditConditionHides))
	bool bPlayAtAttachedPawnLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "SoundType == EPlaySoundType::E_SoundLocation && bPlayAtAttachedPawnLocation == true", HideEditConditionToggle, EditConditionHides))
	FVector Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "SoundType == EPlaySoundType::E_SoundLocation && bPlayAtAttachedPawnLocation == false", HideEditConditionToggle, EditConditionHides))
	FVector Location;

	virtual void BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn) override;
	virtual FString GetNodeDisplayText_Implementation() const override;
};
