// Copyright 2025 Ivan Chandra. All Rights Reserved.
#include "DialogSequenceDetails.h"

#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderEditor.h"
#include "DialogDefinition.h"
#include "DialogSequenceShot.h"
#include "DialogStage.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Internationalization/Text.h"
#include "Layout/Margin.h"
#include "MovieSceneDialogSection.h"
#include "PropertyHandle.h"
#include "Templates/Casts.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "DialogSequenceDetails"

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
static bool IsDialogDefinitionInGraph(const UDialogBuilderGraph* Graph, const UDialogDefinition* Definition)
{
	if (!Graph || !Definition)
	{
		return false;
	}

	for (const TObjectPtr<UDialogParticipant>& Entry : Graph->ParticipantDefinitions)
	{
		if (Entry.Get() == Definition)
		{
			return true;
		}
	}

	for (const TObjectPtr<UDialogProp>& Entry : Graph->PropDefinitions)
	{
		if (Entry.Get() == Definition)
		{
			return true;
		}
	}

	return false;
}

static FText GetDialogDefinitionText(const UDialogDefinition* Definition)
{
	if (!Definition)
	{
		return LOCTEXT("NoneText", "None");
	}

	return !Definition->DisplayName.IsEmpty()
		? Definition->DisplayName
		: FText::FromString(Definition->Tag.ToString());
}

static FText GetParticipantText(const UDialogParticipant* Participant)
{
	if (!Participant)
	{
		return LOCTEXT("NoneText", "None");
	}

	return !Participant->DisplayName.IsEmpty()
		? Participant->DisplayName
		: FText::FromString(Participant->Tag.ToString());
}
}

TSharedRef<IDetailCustomization> FDialogSequenceDetails::MakeInstance()
{
	return MakeShareable(new FDialogSequenceDetails);
}

void FDialogSequenceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DialogSequenceSlot.Reset();
	DialogSequenceShot.Reset();
	MovieSceneDialogSection.Reset();

	const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailBuilder.GetSelectedObjects();
	for (const TWeakObjectPtr<UObject>& CurrentObject : SelectedObjects)
	{
		if (!CurrentObject.IsValid())
		{
			continue;
		}

		if (!DialogSequenceSlot.IsValid())
		{
			if (UDialogSequenceSlot* SlotPtr = Cast<UDialogSequenceSlot>(CurrentObject.Get()))
			{
				DialogSequenceSlot = SlotPtr;
			}
		}

		if (!DialogSequenceShot.IsValid())
		{
			if (UDialogSequenceShot* SequenceShotPtr = Cast<UDialogSequenceShot>(CurrentObject.Get()))
			{
				DialogSequenceShot = SequenceShotPtr;
			}
		}

		if (!MovieSceneDialogSection.IsValid())
		{
			if (UMovieSceneDialogSection* SectionPtr = Cast<UMovieSceneDialogSection>(CurrentObject.Get()))
			{
				MovieSceneDialogSection = SectionPtr;
			}
		}

		if (CurrentObject.Get()->IsA<UDialogPlayerParticipant>() || CurrentObject.Get()->IsA<UDialogLight>())
		{
			if (TSharedPtr<IPropertyHandle> TagHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogDefinition, Tag)))
			{
				TagHandle->MarkHiddenByCustomization();
			}
		}
	}

	if (SelectedObjects.Num() != 1)
	{
		return;
	}

	// Slot customization
	if (UDialogSequenceSlot* Slot = DialogSequenceSlot.Get())
	{
		if (Slot->IsA<UDialogSequenceSlot_Light>())
		{
			return;
		}
		if (!IsDialogDefinitionInGraph(Slot->OwningDialogGraph, Slot->DialogDefinition))
		{
			Slot->Modify();
			Slot->DialogDefinition = nullptr;
		}

		IDetailCategoryBuilder& DetailCategory = DetailBuilder.EditCategory("Detail", LOCTEXT("DetailCategoryTitle", "Detail"));

	
		DetailCategory.AddCustomRow(LOCTEXT("ActorToBindLabel", "Actor To Bind"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ActorToBindText", "Actor To Bind"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForSlotPicker(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FDialogSequenceDetails::GetSlotParticipantDisplayText)),
					FOnGetContent::CreateSP(this, &FDialogSequenceDetails::OnGetSlotListMenu))
			];

		DetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceSlot, SlotLocation)).DisplayName(LOCTEXT("SlotLocationLabel", "Slot Location"));
		DetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceSlot, SlotRotation)).DisplayName(LOCTEXT("SlotRotationLabel", "Slot Rotation"));
		return;
	}

	// Sequence shot customization
	if (UDialogSequenceShot* SequenceShot = DialogSequenceShot.Get())
	{
		if (!IsDialogDefinitionInGraph(SequenceShot->OwningDialogGraph, SequenceShot->ActorToFocus))
		{
			SequenceShot->Modify();
			SequenceShot->ActorToFocus = nullptr;
		}

		IDetailCategoryBuilder& ShotCategory = DetailBuilder.EditCategory("Track & Focus Settings", LOCTEXT("ShotTrackFocusCategoryTitle", "Track & Focus Settings"));

		TSharedPtr<IPropertyHandle> ActorToFocusHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, ActorToFocus));
		if (ActorToFocusHandle.IsValid())
		{
			ActorToFocusHandle->MarkHiddenByCustomization();
		}

		ShotCategory.AddCustomRow(LOCTEXT("ShotActorToFocusRowLabel", "Target Actor"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ShotActorToFocusText", "Target Actor"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForShotActorToFocusPicker(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FDialogSequenceDetails::GetShotActorToFocusDisplayText)),
					FOnGetContent::CreateSP(this, &FDialogSequenceDetails::OnGetShotActorToFocusListMenu))
			];

		const TSharedRef<IPropertyHandle> DrawTargetFocusHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, bDrawTargetFocus));

		const TSharedRef<IPropertyHandle> TrackedBoneHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, TrackedBone));

		const TSharedRef<IPropertyHandle> FocusCameraOnActorHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, bFocusCameraOnActor));


		ShotCategory.AddProperty(DrawTargetFocusHandle);
		ShotCategory.AddProperty(FocusCameraOnActorHandle);
		ShotCategory.AddProperty(TrackedBoneHandle);

		IDetailCategoryBuilder& PlacementCategory = DetailBuilder.EditCategory("Camera Placement", LOCTEXT("CameraPlacementCategoryTitle", "Camera Placement"));


		const TSharedRef<IPropertyHandle> CameraDistanceHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, CameraDistance));

		const TSharedRef<IPropertyHandle> CameraOffsetHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, CameraOffset));

		const TSharedRef<IPropertyHandle> TargetOffsetHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogSequenceShot, TargetOffset));


		PlacementCategory.AddProperty(CameraDistanceHandle);
		PlacementCategory.AddProperty(CameraOffsetHandle);
		PlacementCategory.AddProperty(TargetOffsetHandle);

		IDetailCategoryBuilder& CameraSettingsCategory = DetailBuilder.EditCategory("Camera Default Settings", LOCTEXT("CameraDefaultSettingsCategoryTitle", "Camera Default Settings"));

		UDialogBuilderGraph* DialogGraph = SequenceShot->OwningDialogGraph;

		if (DialogGraph)
		{
			TArray<UObject*> ExternalObjects;
			ExternalObjects.Add(DialogGraph);


			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, FocusMethod));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, FocalLength));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, Aperture));


			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, CustomNearClippingPlane));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, CropSettings));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, Filmback));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, LensSettings));

			CameraSettingsCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, PostProcessSettings));
		}

		
		return;
	}

	// Section customization
	if (UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get())
	{
		Section->RefreshDialogContent();

		IDetailCategoryBuilder& SectionCategory = DetailBuilder.EditCategory("Section", LOCTEXT("SectionCategoryTitle", "Section"));

		TSharedPtr<IPropertyHandle> DialogLineHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMovieSceneDialogSection, DialogLine));
		if (DialogLineHandle.IsValid())
		{
			DialogLineHandle->MarkHiddenByCustomization();
		}

		TSharedPtr<IPropertyHandle> SpeakerRefHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMovieSceneDialogSection, SpeakerParticipantDefinition));
		if (SpeakerRefHandle.IsValid())
		{
			SpeakerRefHandle->MarkHiddenByCustomization();
		}

		TSharedPtr<IPropertyHandle> ListenerRefHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMovieSceneDialogSection, ListenerParticipantDefinition));
		if (ListenerRefHandle.IsValid())
		{
			ListenerRefHandle->MarkHiddenByCustomization();
		}

		SectionCategory.AddCustomRow(LOCTEXT("SpeakerParticipantRowLabel", "Speaker Participant"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("SpeakerParticipantText", "Speaker"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForSpeakerParticipantPicker(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FDialogSequenceDetails::GetSectionSpeakerDisplayText)),
					FOnGetContent::CreateSP(this, &FDialogSequenceDetails::OnGetSpeakerParticipantListMenu))
			];

		SectionCategory.AddCustomRow(LOCTEXT("ListenerParticipantRowLabel", "Listener Participant"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ListenerParticipantText", "Listener"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForListenerParticipantPicker(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FDialogSequenceDetails::GetSectionListenerDisplayText)),
					FOnGetContent::CreateSP(this, &FDialogSequenceDetails::OnGetListenerParticipantListMenu))
			];

		if (DialogLineHandle.IsValid())
		{
			uint32 NumChildren = 0;
			DialogLineHandle->GetNumChildren(NumChildren);

			for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
			{
				TSharedPtr<IPropertyHandle> ChildHandle = DialogLineHandle->GetChildHandle(ChildIndex);
				if (!ChildHandle.IsValid() || !ChildHandle->GetProperty())
				{
					continue;
				}

				SectionCategory.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}
}

FText FDialogSequenceDetails::GetSlotParticipantDisplayText() const
{
	const UDialogSequenceSlot* Slot = DialogSequenceSlot.Get();
	if (!Slot || !Slot->OwningDialogGraph)
	{
		return LOCTEXT("SlotNone", "None");
	}

	const UDialogDefinition* Selected = Slot->DialogDefinition;
	return IsDialogDefinitionInGraph(Slot->OwningDialogGraph, Selected) ? GetDialogDefinitionText(Selected) : LOCTEXT("SlotNoneMissing", "None");
}

FText FDialogSequenceDetails::GetSectionSpeakerDisplayText() const
{
	const UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get();
	const UDialogBuilderGraph* Graph = Section ? Section->GetOwningDialogGraph() : nullptr;
	return IsParticipantInGraph(Graph, Section ? Section->SpeakerParticipantDefinition.Get() : nullptr)
		? GetParticipantText(Section->SpeakerParticipantDefinition.Get())
		: LOCTEXT("SpeakerNone", "None");
}

FText FDialogSequenceDetails::GetSectionListenerDisplayText() const
{
	const UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get();
	const UDialogBuilderGraph* Graph = Section ? Section->GetOwningDialogGraph() : nullptr;
	return IsParticipantInGraph(Graph, Section ? Section->ListenerParticipantDefinition.Get() : nullptr)
		? GetParticipantText(Section->ListenerParticipantDefinition.Get())
		: LOCTEXT("ListenerNone", "None");
}

//sequence shot focus
FText FDialogSequenceDetails::GetShotActorToFocusDisplayText() const
{
	const UDialogSequenceShot* SequenceShot = DialogSequenceShot.Get();
	if (!SequenceShot || !SequenceShot->OwningDialogGraph)
	{
		return LOCTEXT("ShotActorToFocusNone", "None");
	}

	const UDialogDefinition* Selected = SequenceShot->ActorToFocus;
	return IsDialogDefinitionInGraph(SequenceShot->OwningDialogGraph, Selected)
		? GetDialogDefinitionText(Selected)
		: LOCTEXT("ShotActorToFocusNoneMissing", "None");
}

TSharedRef<SWidget> FDialogSequenceDetails::GetWidgetForShotActorToFocusPicker(const TAttribute<FText>& InDefinitionText, FOnGetContent GetContentMenu)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([InDefinitionText]() { return InDefinitionText.Get(); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

TSharedRef<SWidget> FDialogSequenceDetails::OnGetShotActorToFocusListMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("ShotActorToFocusParticipants", LOCTEXT("ShotActorToFocusParticipantsSection", "Actor To Focus"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ShotActorToFocusNoneEntry", "None"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::ClearShotActorToFocusDefinition)));

	const UDialogSequenceShot* SequenceShot = DialogSequenceShot.Get();
	const UDialogBuilderGraph* Graph = SequenceShot ? SequenceShot->OwningDialogGraph : nullptr;

	UDialogBuilderNode_DialogSequence* SequenceNode = Graph ? Graph->CurrentEditingSequenceNode : nullptr;
	UDialogStage* CurrentStage = SequenceNode ? SequenceNode->DialogStage: nullptr;


	if (CurrentStage)
	{
		for(auto& Slot : CurrentStage->Slots)
		{
			if (Slot && Slot->DialogDefinition)
			{
				MenuBuilder.AddMenuEntry(
					GetDialogDefinitionText(Slot->DialogDefinition.Get()),
					FText::FromString(Slot->DialogDefinition->Tag.ToString()),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::UseShotActorToFocusDefinition, static_cast<UDialogDefinition*>(Slot->DialogDefinition.Get()))));
			}
		}
		
		
	}
	MenuBuilder.EndSection();

	

	return MenuBuilder.MakeWidget();
}

void FDialogSequenceDetails::UseShotActorToFocusDefinition(UDialogDefinition* InDialogDefinition)
{
	if (!DialogSequenceShot.IsValid())
	{
		return;
	}

	UDialogSequenceShot* SequenceShot = DialogSequenceShot.Get();
	if (!InDialogDefinition || !IsDialogDefinitionInGraph(SequenceShot->OwningDialogGraph, InDialogDefinition))
	{
		ClearShotActorToFocusDefinition();
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetShotActorToFocus", "Set Shot Actor To Focus"));
	SequenceShot->Modify();
	SequenceShot->ActorToFocus = InDialogDefinition;

	if (SequenceShot->OwningDialogGraph->DialogGraphPages.IsValidIndex(0))
	{
		UDialogBuilderEdGraph* DialogGraph = SequenceShot->OwningDialogGraph->DialogGraphPages.IsValidIndex(0)
			? Cast<UDialogBuilderEdGraph>(SequenceShot->OwningDialogGraph->DialogGraphPages[0])
			: nullptr;
		FDialogBuilderEditor* DialogBuilderEditor = DialogGraph ? DialogGraph->DialogEditorPtr.Pin().Get() : nullptr;
		if (DialogBuilderEditor)
		{
			DialogBuilderEditor->ApplyCameraPreset(SequenceShot);
		}
	}
}

void FDialogSequenceDetails::ClearShotActorToFocusDefinition()
{
	if (UDialogSequenceShot* SequenceShot = DialogSequenceShot.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("ClearShotActorToFocus", "Clear Shot Actor To Focus"));
		SequenceShot->Modify();
		SequenceShot->ActorToFocus = nullptr;
	}
}

// Slot
TSharedRef<SWidget> FDialogSequenceDetails::GetWidgetForSlotPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([InParticipantNameText]() { return InParticipantNameText.Get(); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

TSharedRef<SWidget> FDialogSequenceDetails::OnGetSlotListMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("SlotCommon", LOCTEXT("SlotCommonSection", "Slot"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SlotParticipantNone", "None"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::ClearSlotDefinition)));
	MenuBuilder.EndSection();

	const UDialogSequenceSlot* Slot = DialogSequenceSlot.Get();
	const UDialogBuilderGraph* Graph = Slot ? Slot->OwningDialogGraph : nullptr;
	if (!Slot || !Graph)
	{
		return MenuBuilder.MakeWidget();
	}

	

	// Regular slot => participants + props
	if (Slot->IsA<UDialogSequenceSlot>())
	{
		MenuBuilder.BeginSection("SlotParticipants", LOCTEXT("SlotParticipantsSection", "Participant Definitions"));
		for (const TObjectPtr<UDialogParticipant>& Participant : Graph->ParticipantDefinitions)
		{
			if (!Participant)
			{
				continue;
			}

			MenuBuilder.AddMenuEntry(
				GetDialogDefinitionText(Participant.Get()),
				FText::FromString(Participant->Tag.ToString()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::UseSlotDefinition, static_cast<UDialogDefinition*>(Participant.Get()))));
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("SlotProps", LOCTEXT("SlotPropsSection", "Prop Definitions"));
		for (const TObjectPtr<UDialogProp>& Prop : Graph->PropDefinitions)
		{
			if (!Prop)
			{
				continue;
			}

			MenuBuilder.AddMenuEntry(
				GetDialogDefinitionText(Prop.Get()),
				FText::FromString(Prop->Tag.ToString()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::UseSlotDefinition, static_cast<UDialogDefinition*>(Prop.Get()))));
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void FDialogSequenceDetails::UseSlotDefinition(UDialogDefinition* InDialogDefinition)
{
	if (!InDialogDefinition || !DialogSequenceSlot.IsValid())
	{
		return;
	}

	UDialogSequenceSlot* Slot = DialogSequenceSlot.Get();
	if (!IsDialogDefinitionInGraph(Slot->OwningDialogGraph, InDialogDefinition))
	{
		ClearSlotDefinition();
		return;
	}

	if (UDialogStage* OwningStage = Slot->GetOwningDialogStage())
	{
		for (const TObjectPtr<UDialogSequenceSlot>& OtherSlot : OwningStage->Slots)
		{
			if (!OtherSlot || OtherSlot.Get() == Slot)
			{
				continue;
			}

			if (OtherSlot->DialogDefinition.Get() == InDialogDefinition)
			{
				FNotificationInfo Info(LOCTEXT("DuplicateSlotDefinitionWarning", "This dialog definition is already assigned to another slot in this stage."));
				Info.ExpireDuration = 3.0f;
				Info.bUseLargeFont = false;

				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if (Notification.IsValid())
				{
					Notification->SetCompletionState(SNotificationItem::CS_Fail);
				}

				return;
			}
		}
	}

	const FScopedTransaction Transaction(LOCTEXT("SetSlotDefinition", "Set Slot Definition"));
	Slot->SetDialogDefinition(InDialogDefinition);
}

void FDialogSequenceDetails::ClearSlotDefinition()
{
	if (UDialogSequenceSlot* Slot = DialogSequenceSlot.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("ClearSlotDefinition", "Clear Slot Definition"));
		Slot->SetDialogDefinition(nullptr);
	}
}

// Speaker
TSharedRef<SWidget> FDialogSequenceDetails::GetWidgetForSpeakerParticipantPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([InParticipantNameText]() { return InParticipantNameText.Get(); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

TSharedRef<SWidget> FDialogSequenceDetails::OnGetSpeakerParticipantListMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("SpeakerParticipants", LOCTEXT("SpeakerParticipantsSection", "Participant Definitions"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SpeakerParticipantNone", "None"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::ClearSpeakerParticipantDefinition)));

	if (const UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get())
	{
		if (const UDialogBuilderGraph* Graph = Section->GetOwningDialogGraph())
		{
			for (const TObjectPtr<UDialogParticipant>& Participant : Graph->ParticipantDefinitions)
			{
				if (!Participant)
				{
					continue;
				}

				MenuBuilder.AddMenuEntry(
					GetParticipantText(Participant.Get()),
					FText::FromString(Participant->Tag.ToString()),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::UseSpeakerParticipantDefinition, Participant.Get())));
			}
		}
	}

	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void FDialogSequenceDetails::UseSpeakerParticipantDefinition(UDialogParticipant* InParticipantDefinition)
{
	if (!InParticipantDefinition || !MovieSceneDialogSection.IsValid())
	{
		return;
	}

	UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get();
	if (!IsParticipantInGraph(Section->GetOwningDialogGraph(), InParticipantDefinition))
	{
		ClearSpeakerParticipantDefinition();
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetSpeakerParticipant", "Set Speaker Participant"));
	Section->Modify();
	Section->SpeakerParticipantDefinition = InParticipantDefinition;
	Section->RefreshDialogContent();
}

void FDialogSequenceDetails::ClearSpeakerParticipantDefinition()
{
	if (UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("ClearSpeakerParticipant", "Clear Speaker Participant"));
		Section->Modify();
		Section->SpeakerParticipantDefinition = nullptr;
		Section->RefreshDialogContent();
	}
}

// Listener
TSharedRef<SWidget> FDialogSequenceDetails::GetWidgetForListenerParticipantPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([InParticipantNameText]() { return InParticipantNameText.Get(); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

TSharedRef<SWidget> FDialogSequenceDetails::OnGetListenerParticipantListMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("ListenerParticipants", LOCTEXT("ListenerParticipantsSection", "Participant Definitions"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ListenerParticipantNone", "None"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::ClearListenerParticipantDefinition)));

	if (const UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get())
	{
		if (const UDialogBuilderGraph* Graph = Section->GetOwningDialogGraph())
		{
			for (const TObjectPtr<UDialogParticipant>& Participant : Graph->ParticipantDefinitions)
			{
				if (!Participant)
				{
					continue;
				}

				MenuBuilder.AddMenuEntry(
					GetParticipantText(Participant.Get()),
					FText::FromString(Participant->Tag.ToString()),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FDialogSequenceDetails::UseListenerParticipantDefinition, Participant.Get())));
			}
		}
	}

	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void FDialogSequenceDetails::UseListenerParticipantDefinition(UDialogParticipant* InParticipantDefinition)
{
	if (!InParticipantDefinition || !MovieSceneDialogSection.IsValid())
	{
		return;
	}

	UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get();
	if (!IsParticipantInGraph(Section->GetOwningDialogGraph(), InParticipantDefinition))
	{
		ClearListenerParticipantDefinition();
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetListenerParticipant", "Set Listener Participant"));
	Section->Modify();
	Section->ListenerParticipantDefinition = InParticipantDefinition;
	Section->RefreshDialogContent();
}

void FDialogSequenceDetails::ClearListenerParticipantDefinition()
{
	if (UMovieSceneDialogSection* Section = MovieSceneDialogSection.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("ClearListenerParticipant", "Clear Listener Participant"));
		Section->Modify();
		Section->ListenerParticipantDefinition = nullptr;
		Section->RefreshDialogContent();
	}
}

#undef LOCTEXT_NAMESPACE
