// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/SlateDelegates.h"
#include "IDetailCustomization.h"
#include "Misc/Attribute.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FText;
class IDetailLayoutBuilder;
class SWidget;
class UDialogDefinition;
class UDialogParticipant;
class UDialogSequenceShot;
class UDialogSequenceSlot;
class UMovieSceneDialogSection;

class FDialogSequenceDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:
	// Slot
	TSharedRef<SWidget> GetWidgetForSlotPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu);
	TSharedRef<SWidget> OnGetSlotListMenu();
	void UseSlotDefinition(UDialogDefinition* InDialogDefinition);
	void ClearSlotDefinition();

	// Sequence Shot - ActorToFocus
	TSharedRef<SWidget> GetWidgetForShotActorToFocusPicker(const TAttribute<FText>& InDefinitionText, FOnGetContent GetContentMenu);
	TSharedRef<SWidget> OnGetShotActorToFocusListMenu();
	void UseShotActorToFocusDefinition(UDialogDefinition* InDialogDefinition);
	void ClearShotActorToFocusDefinition();

	// Section - Speaker
	TSharedRef<SWidget> GetWidgetForSpeakerParticipantPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu);
	TSharedRef<SWidget> OnGetSpeakerParticipantListMenu();
	void UseSpeakerParticipantDefinition(UDialogParticipant* InParticipantDefinition);
	void ClearSpeakerParticipantDefinition();

	// Section - Listener
	TSharedRef<SWidget> GetWidgetForListenerParticipantPicker(const TAttribute<FText>& InParticipantNameText, FOnGetContent GetContentMenu);
	TSharedRef<SWidget> OnGetListenerParticipantListMenu();
	void UseListenerParticipantDefinition(UDialogParticipant* InParticipantDefinition);
	void ClearListenerParticipantDefinition();

private:
	FText GetSlotParticipantDisplayText() const;
	FText GetShotActorToFocusDisplayText() const;
	FText GetSectionSpeakerDisplayText() const;
	FText GetSectionListenerDisplayText() const;

private:
	TWeakObjectPtr<UDialogSequenceSlot> DialogSequenceSlot;
	TWeakObjectPtr<UDialogSequenceShot> DialogSequenceShot;
	TWeakObjectPtr<UMovieSceneDialogSection> MovieSceneDialogSection;
};