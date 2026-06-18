// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "MovieSceneNameableTrack.h"
#include "DialogData.h"
#include "MovieSceneDialogTrack.generated.h"

class UMovieSceneDialogSection;
class UMovieSceneSection;
class UDialogDefinition;
struct FGameplayTag;

UCLASS()
class ORIONRPG_API UMovieSceneDialogTrack
	: public UMovieSceneNameableTrack
	, public IMovieSceneTrackTemplateProducer
{
	GENERATED_BODY()

public:
	UMovieSceneDialogTrack(const FObjectInitializer& ObjectInitializer);

	class  UDialogSequence* GetOwningDialogSequence() const;
	class UDialogBuilderGraph* GetOwningDialogGraph() const;

	static uint16 GetEvaluationPriority() { return UMovieSceneSpawnTrack::GetEvaluationPriority() + 120; }

	UMovieSceneDialogSection* AddDialogSection(
		const FOrionDialogLine& InDialogLine,
		FFrameNumber InStartFrame,
		int32 InDurationFrames = 1);

public:
	virtual bool IsEmpty() const override;
	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual void RemoveAllAnimationData() override;
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual bool SupportsMultipleRows() const override;

	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif

private:
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;
};