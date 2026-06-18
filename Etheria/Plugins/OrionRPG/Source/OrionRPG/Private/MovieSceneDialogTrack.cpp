// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "MovieSceneDialogTrack.h"
#include "DialogSequence.h"
#include "DialogBuilderGraph.h"
#include "MovieSceneDialogSection.h"
#include "MovieSceneDialogTemplate.h"

UMovieSceneDialogTrack::UMovieSceneDialogTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	TrackTint = FColor(52, 152, 219);
#endif
}

UDialogSequence* UMovieSceneDialogTrack::GetOwningDialogSequence() const
{
	return GetTypedOuter<UDialogSequence>();
}

UDialogBuilderGraph* UMovieSceneDialogTrack::GetOwningDialogGraph() const
{
	if(UDialogSequence* DialogSequence = GetOwningDialogSequence())
	{
		return DialogSequence->OwningDialogGraph;
	}
	return nullptr;
}

void UMovieSceneDialogTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}

void UMovieSceneDialogTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

bool UMovieSceneDialogTrack::IsEmpty() const
{
	return Sections.Num() == 0;
}

UMovieSceneSection* UMovieSceneDialogTrack::CreateNewSection()
{
	return NewObject<UMovieSceneDialogSection>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UMovieSceneDialogTrack::GetAllSections() const
{
	return Sections;
}

UMovieSceneDialogSection* UMovieSceneDialogTrack::AddDialogSection(
	const FOrionDialogLine& InDialogLine,
	FFrameNumber InStartFrame,
	int32 InDurationFrames)
{
	UMovieSceneDialogSection* NewSection = Cast<UMovieSceneDialogSection>(CreateNewSection());
	if (!NewSection)
	{
		return nullptr;
	}

	NewSection->DialogLine = InDialogLine;

	const int32 ClampedDurationFrames = FMath::Max(1, InDurationFrames);
	NewSection->SetRange(TRange<FFrameNumber>(
		TRangeBound<FFrameNumber>::Inclusive(InStartFrame),
		TRangeBound<FFrameNumber>::Exclusive(InStartFrame + ClampedDurationFrames)));

	// If DialogSound is valid, this overrides manual duration to always match sound length.
	NewSection->SyncRangeToDialogSoundDuration();

	AddSection(*NewSection);

	return NewSection;
}

bool UMovieSceneDialogTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UMovieSceneDialogSection::StaticClass();
}

void UMovieSceneDialogTrack::RemoveAllAnimationData()
{
	Sections.Reset();
}

bool UMovieSceneDialogTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}


bool UMovieSceneDialogTrack::SupportsMultipleRows() const
{
	return false;
}

FMovieSceneEvalTemplatePtr UMovieSceneDialogTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneDialogSectionTemplate(*CastChecked<UMovieSceneDialogSection>(&InSection));
}

#if WITH_EDITORONLY_DATA
FText UMovieSceneDialogTrack::GetDefaultDisplayName() const
{
	return NSLOCTEXT("OrionRPG", "MovieSceneDialogTrackName", "Dialog");
}
#endif