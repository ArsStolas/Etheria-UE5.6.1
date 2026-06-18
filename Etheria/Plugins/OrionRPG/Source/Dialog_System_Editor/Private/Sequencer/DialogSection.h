// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISequencerSection.h"

class UMovieSceneDialogSection;

class FDialogSection
	: public ISequencerSection
	, public TSharedFromThis<FDialogSection>
{
public:
	explicit FDialogSection(UMovieSceneDialogSection& InSectionObject);

public:
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual TSharedRef<SWidget> GenerateSectionWidget() override;
	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual FText GetSectionTitle() const override;

private:
	UMovieSceneDialogSection& SectionObject;
};