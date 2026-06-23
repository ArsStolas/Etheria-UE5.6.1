// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "MovieSceneDialogSection.h"
#include "DialogSequence.h"
#include "DialogBuilderGraph.h"
#include "DialogDefinition.h"
#include "Audio.h"
#include "MovieScene.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "DialogSection"

namespace
{
static bool IsParticipantInGraph(const UDialogBuilderGraph* Graph, const UDialogParticipant* Participant)
{
	if (!Graph || !Participant)
	{
		return false;
	}

	for (const TObjectPtr<UDialogParticipant>& Entry : Graph->ParticipantDefinitions)
	{
		if (Entry.Get() == Participant)
		{
			return true;
		}
	}

	return false;
}
}

UMovieSceneDialogSection::UMovieSceneDialogSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsActive(true);
	SetIsLocked(false);
}

UDialogSequence* UMovieSceneDialogSection::GetOwningDialogSequence() const
{
	return GetTypedOuter<UDialogSequence>();
}

UDialogBuilderGraph* UMovieSceneDialogSection::GetOwningDialogGraph() const
{
	if (UDialogSequence* DialogSequence = GetOwningDialogSequence())
	{
		return DialogSequence->OwningDialogGraph;
	}
	return nullptr;
}

bool UMovieSceneDialogSection::TryGetDialogSoundDurationFrames(int32& OutDurationFrames) const
{
	OutDurationFrames = 0;

	if (!DialogLine.DialogSound)
	{
		return false;
	}

	const float SoundDuration = DialogLine.DialogSound->GetDuration();
	if (SoundDuration <= 0.f || SoundDuration == INDEFINITELY_LOOPING_DURATION)
	{
		return false;
	}

	const UMovieScene* MovieScene = GetTypedOuter<UMovieScene>();
	if (!MovieScene)
	{
		return false;
	}

	const FFrameTime DurationInFrames = SoundDuration * MovieScene->GetTickResolution();
	OutDurationFrames = FMath::Max(1, DurationInFrames.CeilToFrame().Value);
	return true;
}

void UMovieSceneDialogSection::SyncRangeToDialogSoundDuration()
{
	int32 DurationFrames = 0;
	if (!TryGetDialogSoundDurationFrames(DurationFrames))
	{
		return;
	}

	const FFrameNumber StartFrame = HasStartFrame() ? GetInclusiveStartFrame() : FFrameNumber(0);
	const TRange<FFrameNumber> SoundRange(
		TRangeBound<FFrameNumber>::Inclusive(StartFrame),
		TRangeBound<FFrameNumber>::Exclusive(StartFrame + DurationFrames));

	if (GetRange() != SoundRange)
	{
		SetRange(SoundRange);
	}
}

void UMovieSceneDialogSection::RefreshDialogContent()
{
	UDialogBuilderGraph* DialogGraph = GetOwningDialogGraph();

	// Speaker reference validity
	if (!IsParticipantInGraph(DialogGraph, SpeakerParticipantDefinition))
	{
		if (SpeakerParticipantDefinition)
		{
			SpeakerParticipantDefinition = nullptr;
		}
	}

	// Listener reference validity
	if (!IsParticipantInGraph(DialogGraph, ListenerParticipantDefinition))
	{
		if (ListenerParticipantDefinition)
		{
			ListenerParticipantDefinition = nullptr;
		}
	}

	const FText SpeakerName = SpeakerParticipantDefinition ? SpeakerParticipantDefinition->DisplayName : FText();
	if (!DialogLine.SpeakerName.EqualTo(SpeakerName))
	{
		DialogLine.SpeakerName = SpeakerName;
	}

	// Keep section duration synced to dialog audio duration (if valid).
	SyncRangeToDialogSoundDuration();
}

FOrionDialogLine UMovieSceneDialogSection::GetDialogLine() const
{
	FOrionDialogLine Result = DialogLine;
	return Result;
}

#if WITH_EDITOR
void UMovieSceneDialogSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshDialogContent();
}
#endif

#undef LOCTEXT_NAMESPACE
