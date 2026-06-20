// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "Framework/SlateDelegates.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "Misc/Attribute.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "DialogBuilder_EditorStyle.h"

class FText;
class IDetailCategoryBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;
class STextEntryPopup;
class SWidget;
class UDialogBuilderNode_DialogLine;
class UDialogBuilderNode_DialogSequence;
class UDialogBuilderNode_PlayerChoice;
class UDialogBuilderNode_PlayerLine;
class UBlendProfile;
class UEdGraph;

class FDialogNodeDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:

	TSharedRef<SWidget> GetWidgetForInlineShareMenu(const TAttribute<FText>& InSharedNameText, const TAttribute<bool>& bInIsCurrentlyShared, FOnClicked PromoteClick, FOnClicked DemoteClick, FOnGetContent GetContentMenu);

	FReply OnPromoteToSharedClick();
	void PromoteToShared(const FText& NewTransitionName, ETextCommit::Type CommitInfo);
	FReply OnUnshareClick();
	TSharedRef<SWidget> OnGetShareableNodesMenu();
	void BecomeSharedWith(UDialogBuilderNode_DialogLine* NewNode);

	TSharedRef<SWidget> GetWidgetForDialogStagePicker(const TAttribute<FText>& InDialogStageNameText, FOnClicked PromoteClick, FOnGetContent GetContentMenu);
	TSharedRef<SWidget> OnGetDialogStageListMenu();
	void UseDialogStage(class UDialogStage* InDialogStage);

private:
	TWeakObjectPtr<UDialogBuilderNode_DialogSequence> DialogSequenceNode;
	TWeakObjectPtr<UDialogBuilderNode_DialogLine> DialogLineNode;
	TWeakObjectPtr<UDialogBuilderNode_PlayerLine> PlayerLineNode;
	TWeakObjectPtr<UDialogBuilderNode_PlayerChoice> PlayerChoiceNode;
	TSharedPtr<STextEntryPopup> TextEntryWidget;
};