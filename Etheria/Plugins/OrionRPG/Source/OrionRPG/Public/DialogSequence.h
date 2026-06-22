// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LevelSequence.h"
#include "DialogData.h"
#include "DialogSequence.generated.h"

class UDialogBuilderGraph;
class UMovieSceneDialogSection;
class UMovieSceneDialogTrack;
struct FDialogLineData;
struct FGameplayTag;

UCLASS(BlueprintType)
class ORIONRPG_API UDialogSequence : public ULevelSequence
{
	GENERATED_BODY()

public:
	UDialogSequence(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UMovieSceneDialogTrack* GetOrCreateDialogTrack();
	const UMovieSceneDialogTrack* FindDialogTrack() const;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog Sequence")
	TObjectPtr<UDialogBuilderGraph> OwningDialogGraph;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog Sequence")
	TObjectPtr<UMovieSceneDialogTrack> DialogTrack;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog Sequence")
	FFrameNumber MaxFrame;
public:
	void RefreshSequence();


#if WITH_EDITOR
	/**
	 * Get a Dialog sequence.
	 *
	 * @return Placeholder sequence.
	 */
	static UDialogSequence* GetNullDialogSequence();
#endif
#if WITH_EDITOR
	//virtual ETrackSupport IsTrackSupportedImpl(TSubclassOf<class UMovieSceneTrack> InTrackClass) const override;
#endif
};