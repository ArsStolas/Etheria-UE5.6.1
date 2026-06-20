// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogSequence.h"
#include "DialogBuilderGraph.h"
#include "MovieScene.h"
#include "MovieSceneDialogSection.h"
#include "MovieSceneDialogTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Tracks/MovieSceneAudioTrack.h"
#include "Tracks/MovieSceneEventTrack.h"
#include "Tracks/MovieSceneMaterialParameterCollectionTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Tracks/MovieSceneTimeWarpTrack.h"

UDialogSequence::UDialogSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxFrame = 0;
}

UMovieSceneDialogTrack* UDialogSequence::GetOrCreateDialogTrack()
{
	if (!MovieScene)
	{
		return nullptr;
	}

	if (UMovieSceneDialogTrack* ExistingTrack = MovieScene->FindTrack<UMovieSceneDialogTrack>())
	{
		return ExistingTrack;
	}

	Modify();
	MovieScene->Modify();
	UMovieSceneDialogTrack* NewDialogTrack = MovieScene->AddTrack<UMovieSceneDialogTrack>();
#if WITH_EDITORONLY_DATA
	NewDialogTrack->SetDisplayName(FText::FromString("Dialog"));
#endif
	return NewDialogTrack;
}

const UMovieSceneDialogTrack* UDialogSequence::FindDialogTrack() const
{
	if (!MovieScene)
	{
		return nullptr;
	}

	return MovieScene->FindTrack<UMovieSceneDialogTrack>();
}

void UDialogSequence::RefreshSequence()
{
	DialogTrack = GetOrCreateDialogTrack();
	if (!MovieScene || !DialogTrack)
	{
		return;
	}

	TArray<UMovieSceneSection*> SortedSections = DialogTrack->GetAllSections();

	//Sync duration range to match dialog sound
	for (UMovieSceneSection* Section : SortedSections)
	{
		if (!Section) continue;
		if (UMovieSceneDialogSection* DialogSection = Cast<UMovieSceneDialogSection>(Section))
		{
			DialogSection->SyncRangeToDialogSoundDuration();
		}
	}


	//sort sections by start time, and remove any null entries
	SortedSections.RemoveAll([](UMovieSceneSection* Section)
		{
			return Section == nullptr;
		});

	SortedSections.Sort([](const UMovieSceneSection& A, const UMovieSceneSection& B)
		{
			const TRange<FFrameNumber> RangeA = A.GetRange();
			const TRange<FFrameNumber> RangeB = B.GetRange();

			const FFrameNumber StartA = RangeA.HasLowerBound() ? RangeA.GetLowerBoundValue() : FFrameNumber(0);
			const FFrameNumber StartB = RangeB.HasLowerBound() ? RangeB.GetLowerBoundValue() : FFrameNumber(0);

			return StartA < StartB;
		});

	bool bHasSections = false;
	FFrameNumber MinFrame = 0;
	MaxFrame = 0;
	FFrameNumber PreviousEndFrame = 0;

	for (UMovieSceneSection* Section : SortedSections)
	{
		if (!Section) continue;

		const TRange<FFrameNumber> SectionRange = Section->GetRange();
		if (!SectionRange.HasLowerBound() || !SectionRange.HasUpperBound())
		{
			continue;
		}

		FFrameNumber SectionStart = SectionRange.GetLowerBoundValue();
		FFrameNumber SectionEnd = SectionRange.GetUpperBoundValue();

		if (bHasSections && SectionStart < PreviousEndFrame)
		{
			Section->Modify();

			// Fully overlapped: move the section forward so it remains valid.
			const FFrameNumber SectionLength = FMath::Max<FFrameNumber>(1, SectionEnd - SectionStart);
			SectionStart = PreviousEndFrame;
			SectionEnd = SectionStart + SectionLength;
			Section->SetRange(TRange<FFrameNumber>(SectionStart, SectionEnd));
		}

		if (!bHasSections)
		{
			MinFrame = SectionStart;
			MaxFrame = SectionEnd;
			bHasSections = true;
		}
		else
		{
			MinFrame = SectionStart < MinFrame ? SectionStart : MinFrame;
			MaxFrame = MaxFrame < SectionEnd ? SectionEnd : MaxFrame;
		}

		PreviousEndFrame = SectionEnd;
	}

	if (bHasSections)
	{
		const TRange<FFrameNumber> PlaybackRange(0, MaxFrame);
		MovieScene->SetPlaybackRange(PlaybackRange);

		if (UMovieSceneTrack* CameraCutTrack = MovieScene->GetCameraCutTrack())
		{
			for (UMovieSceneSection* Section : CameraCutTrack->GetAllSections())
			{
				if (!Section)
				{
					continue;
				}

				const TRange<FFrameNumber> SectionRange = Section->GetRange();
				if (!SectionRange.HasUpperBound() || SectionRange.GetUpperBoundValue() != MaxFrame)
				{
					Section->SetRange(PlaybackRange);
				}
			}
		}
	}

#if WITH_EDITORONLY_DATA
	MovieScene->SetPlaybackRangeLocked(true);
#endif
}



#if WITH_EDITOR
UDialogSequence* UDialogSequence::GetNullDialogSequence()
{
	static UDialogSequence* NullSequence = nullptr;

	if (!NullSequence)
	{
		NullSequence = NewObject<UDialogSequence>(GetTransientPackage(), NAME_None);
		NullSequence->AddToRoot();
		NullSequence->MovieScene = NewObject<UMovieScene>(NullSequence, FName("No Animation"));
		NullSequence->MovieScene->AddToRoot();

		NullSequence->MovieScene->SetDisplayRate(FFrameRate(20, 1));
	}

	return NullSequence;
}
#endif

