// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "Sequencer/DialogSection.h"
#include "MovieSceneDialogSection.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SNullWidget.h"
#include "Styling/AppStyle.h"
#include "SequencerSectionPainter.h"

FDialogSection::FDialogSection(UMovieSceneDialogSection& InSectionObject)
	: SectionObject(InSectionObject)
{
}

UMovieSceneSection* FDialogSection::GetSectionObject()
{
	return &SectionObject;
}

TSharedRef<SWidget> FDialogSection::GenerateSectionWidget()
{
	// Returning SNullWidget allows standard sequencer UI bounds calculation
	// and prevents UI track collapse bugs inside 5.6 
	return SNullWidget::NullWidget;
}

int32 FDialogSection::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	return InPainter.PaintSectionBackground();
}

FText FDialogSection::GetSectionTitle() const
{
	if (!SectionObject.DialogLine.Line.IsEmpty())
	{
		return SectionObject.DialogLine.Line;
	}

	return FText::FromString(TEXT("None"));
}