// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "Sequencer/DialogTrackEditor.h"
#include "Sequencer/DialogSection.h"
#include "MovieSceneDialogSection.h"
#include "MovieSceneDialogTrack.h"
#include "DialogSequence.h"
#include "OrionSetting.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "SequencerUtilities.h"
#include "ScopedTransaction.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DialogTrackEditor"

namespace
{
	int32 CountDialogTrackSections(const UMovieSceneTrack* DialogTrack)
	{
		int32 SectionCount = 0;
		if (!DialogTrack)
		{
			return SectionCount;
		}

		for (const UMovieSceneSection* Section : DialogTrack->GetAllSections())
		{
			if (Section && Section->IsA(UMovieSceneDialogSection::StaticClass()))
			{
				++SectionCount;
			}
		}

		return SectionCount;
	}

	void ShowDialogSectionTrialLimitNotification(const UOrionSetting* Settings)
	{
		const int32 NodeLimit = Settings ? Settings->NodeLimit : 0;
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeLimit"), NodeLimit);

		FNotificationInfo Info(FText::Format(LOCTEXT("DialogSectionTrialLimitWarning", "Trial Version only allows up to {NodeLimit} dialog sections."), Args));
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;

		const FString PurchaseURL = Settings ? Settings->TrialPurchaseURL : FString();
		if (!PurchaseURL.IsEmpty())
		{
			Info.HyperlinkText = LOCTEXT("DialogSectionTrialLimitPurchaseLink", "Purchase full product");
			Info.Hyperlink = FSimpleDelegate::CreateLambda([PurchaseURL]()
			{
				FPlatformProcess::LaunchURL(*PurchaseURL, nullptr, nullptr);
			});
		}

		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}

	bool CanAddDialogTrackSection(const UMovieSceneTrack* DialogTrack)
	{
		const UOrionSetting* Settings = GetDefault<UOrionSetting>();
		if (!Settings || !Settings->bTrialVersion)
		{
			return true;
		}

		if (CountDialogTrackSections(DialogTrack) < FMath::Max(0, Settings->NodeLimit))
		{
			return true;
		}

		ShowDialogSectionTrialLimitNotification(Settings);
		return false;
	}
}

TSharedRef<ISequencerTrackEditor> FDialogTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShared<FDialogTrackEditor>(InSequencer);
}

FDialogTrackEditor::FDialogTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

bool FDialogTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UMovieSceneDialogTrack::StaticClass();
}

bool FDialogTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	return Cast<UDialogSequence>(InSequence) != nullptr;
}

TSharedRef<ISequencerSection> FDialogTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	UMovieSceneDialogSection* DialogSection = Cast<UMovieSceneDialogSection>(&SectionObject);
	check(SupportsType(SectionObject.GetOuter()->GetClass()) && DialogSection != nullptr);

	return MakeShared<FDialogSection>(*DialogSection);
}

void FDialogTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	/*UMovieSceneSequence* RootSequence = GetSequencer()->GetRootMovieSceneSequence();
	if (!Cast<UDialogSequence>(RootSequence))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddDialogTrack", "Dialog"),
		LOCTEXT("AddDialogTrackTooltip", "Add a dialog track."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogTrackEditor::OnAddTrack)));*/
}

TSharedPtr<SWidget> FDialogTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	if (!Track || !SupportsType(Track->GetClass()))
	{
		return nullptr;
	}

	return FSequencerUtilities::MakeAddButton(
		LOCTEXT("AddDialogSectionButton", "Add Dialog"),
		FOnGetContent::CreateSP(this, &FDialogTrackEditor::BuildAddDialogSectionMenu, Track),
		Params.NodeIsHovered,
		GetSequencer());
}

void FDialogTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track)
{
	UMovieSceneDialogTrack* DialogTrack = Cast<UMovieSceneDialogTrack>(Track);
	if (!DialogTrack)
	{
		return;
	}

	class FDialogTrackCustomization : public IDetailCustomization
	{
	public:
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			DetailBuilder.HideCategory("Track");
			DetailBuilder.HideCategory("General");
		}
	};

	auto SubMenuDelegate = [DialogTrack](FMenuBuilder& InMenuBuilder)
	{
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;

		TArray<TWeakObjectPtr<UObject>> Objects;
		Objects.Add(DialogTrack);

		TSharedRef<IDetailsView> DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
		const FOnGetDetailCustomizationInstance CreateInstance = FOnGetDetailCustomizationInstance::CreateLambda([] { return MakeShared<FDialogTrackCustomization>(); });
		DetailsView->RegisterInstancedCustomPropertyLayout(UMovieSceneDialogTrack::StaticClass(), CreateInstance);
		DetailsView->SetObjects(Objects);

		InMenuBuilder.AddWidget(DetailsView, FText::GetEmpty(), true);
	};

	MenuBuilder.AddSubMenu(
		LOCTEXT("DialogTrackProperties", "Properties"),
		LOCTEXT("DialogTrackPropertiesTooltip", "Dialog track properties."),
		FNewMenuDelegate::CreateLambda(SubMenuDelegate),
		false);
}

void FDialogTrackEditor::OnAddTrack()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (!FocusedMovieScene)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddDialogTrack_Transaction", "Add Dialog Track"));
	FocusedMovieScene->Modify();

	UMovieSceneDialogTrack* NewTrack = FocusedMovieScene->AddTrack<UMovieSceneDialogTrack>();
	checkf(NewTrack != nullptr, TEXT("Failed to create new dialog track."));

	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

TSharedRef<SWidget> FDialogTrackEditor::BuildAddDialogSectionMenu(UMovieSceneTrack* DialogTrack)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddDialogSection", "Dialog"),
		LOCTEXT("AddDialogSectionTooltip", "Add a dialog section."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogTrackEditor::OnAddNewSection, DialogTrack)));

	return MenuBuilder.MakeWidget();
}

void FDialogTrackEditor::OnAddNewSection(UMovieSceneTrack* DialogTrack)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (!FocusedMovieScene || !DialogTrack)
	{
		return;
	}

	if (AddNewSection(FocusedMovieScene, DialogTrack))
	{
		GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

bool FDialogTrackEditor::AddNewSection(UMovieScene* MovieScene, UMovieSceneTrack* DialogTrack)
{
	if (!MovieScene || !DialogTrack)
	{
		return false;
	}

	if (!CanAddDialogTrackSection(DialogTrack))
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddDialogSection_Transaction", "Add Dialog Section"));
	DialogTrack->Modify();

	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const FFrameNumber SectionLength = TickResolution.AsFrameNumber(3.0);

	FFrameNumber LastSectionEndTime = MovieScene->GetPlaybackRange().GetLowerBoundValue();
	for (UMovieSceneSection* Section : DialogTrack->GetAllSections())
	{
		if (!Section)
		{
			continue;
		}

		const TRange<FFrameNumber> SectionRange = Section->GetRange();
		if (SectionRange.HasUpperBound())
		{
			LastSectionEndTime = SectionRange.GetUpperBoundValue() > LastSectionEndTime
				? SectionRange.GetUpperBoundValue()
				: LastSectionEndTime;
		}
	}

	const FFrameNumber CurrentFrame = GetSequencer().IsValid()
		? GetSequencer()->GetLocalTime().Time.FloorToFrame()
		: LastSectionEndTime;

	FFrameNumber StartTime = CurrentFrame;
	FFrameNumber EndTime = StartTime + SectionLength;

	// Try to keep the section at the current frame, but shorten it if it overlaps.
	for (UMovieSceneSection* Section : DialogTrack->GetAllSections())
	{
		if (!Section)
		{
			continue;
		}

		const TRange<FFrameNumber> SectionRange = Section->GetRange();
		const TRange<FFrameNumber> CandidateRange(StartTime, TRangeBound<FFrameNumber>::Exclusive(EndTime));

		if (SectionRange.Overlaps(CandidateRange))
		{
			if (SectionRange.HasLowerBound())
			{
				EndTime = FMath::Min(EndTime, SectionRange.GetLowerBoundValue());
			}
		}
	}

	// If the current frame is already too close/inside another section, fall back to the end.
	if (EndTime <= StartTime)
	{
		StartTime = LastSectionEndTime;
		EndTime = StartTime + SectionLength;
	}

	UMovieSceneDialogSection* DialogSection = CastChecked<UMovieSceneDialogSection>(DialogTrack->CreateNewSection());
	DialogSection->SetRange(TRange<FFrameNumber>(StartTime, TRangeBound<FFrameNumber>::Exclusive(EndTime)));
	DialogSection->SetRowIndex(0);

	DialogTrack->AddSection(*DialogSection);
	return true;
}

#undef LOCTEXT_NAMESPACE
