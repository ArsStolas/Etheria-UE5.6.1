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
#include "QuestBuilder_EditorStyle.h"


class UQuestBuilderNode;
class FText;
class IDetailCategoryBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;
class STextEntryPopup;
class SWidget;
class UQuest;
class UBlendProfile;
class UEdGraph;

class FQuestDetails : public IDetailCustomization
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
	void BecomeSharedWith(UQuest* NewQuest);

private:
	TWeakObjectPtr<UQuest> Quest;
	TWeakObjectPtr<UQuestBuilderNode> QuestNode;
	TSharedPtr<STextEntryPopup> TextEntryWidget;
};