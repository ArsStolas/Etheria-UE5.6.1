// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogNodeDetails.h"

#include "DialogBuilderEdNode_DialogLine.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderEdNode_PlayerLine.h"
#include "DialogBuilderNode_PlayerLine.h"
#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "Containers/Map.h"
#include "Delegates/Delegate.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformCrt.h"
#include "HAL/PlatformMath.h"
#include "IDetailPropertyRow.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/Margin.h"
#include "Layout/WidgetPath.h"
#include "Math/Color.h"
#include "Misc/AssertionMacros.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "SKismetLinearExpression.h"
#include "SlateOptMacros.h"
#include "SlotBase.h"
#include "Styling/AppStyle.h"
#include "Templates/Casts.h"
#include "Templates/SubclassOf.h"
#include "Textures/SlateIcon.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "DetailsCustomization"

/////////////////////////////////////////////////////////////////////////

TSharedRef<IDetailCustomization> FDialogNodeDetails::MakeInstance()
{
	return MakeShareable(new FDialogNodeDetails);
}



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FDialogNodeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get a handle to the node we're viewing
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetSelectedObjects();
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			if (UDialogBuilderNode_DialogLine* DialogLineNodePtr = Cast<UDialogBuilderNode_DialogLine>(CurrentObject.Get()))
			{
				if (!DialogLineNode.IsValid())
				{
					DialogLineNode = DialogLineNodePtr;
				}

			}
			if (UDialogBuilderNode_PlayerLine* PlayerLineNodePtr = Cast<UDialogBuilderNode_PlayerLine>(CurrentObject.Get()))
			{
				if (!PlayerLineNode.IsValid())
				{
					PlayerLineNode = PlayerLineNodePtr;
				}

			}
		}
	}
	IDetailCategoryBuilder& OptionSelectionCategory = DetailBuilder.EditCategory("Selection Rule", LOCTEXT("OptionSelectionCategoryLabel", "Selection Rule"));
	OptionSelectionCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, SelectingChoiceShotOverride)).DisplayName(LOCTEXT("SelectingChoiceShotOverrideLabel", "Selecting Shot Override"));
	OptionSelectionCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, SelectionTimeLimit)).DisplayName(LOCTEXT("SelectionTimeLimitLabel", "Selection Time Limit"));
	OptionSelectionCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, TimeLimit)).DisplayName(LOCTEXT("TimeLimitLabel", "Time Limit"));

	

	IDetailCategoryBuilder& DialogParticipantCategory = DetailBuilder.EditCategory("Participant", LOCTEXT("ParticipantCategoryTitle", "Participant Settings"));
	if (PlayerLineNode == NULL && DialogLineNode != NULL && SelectedObjects.Num() == 1)
	{
		UDialogBuilderNode_DialogLine* DialogLineNodeInstance = DialogLineNode.Get();
		// The sharing option for the crossfade settings
		DialogParticipantCategory.AddCustomRow(LOCTEXT("ParticipantSettingsSharingLabel", "Participant Sharing"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ParticipantSharingLabel", "Participant Sharing"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("UseSharedParticipant_ToolTip", "Participant Sharing let you access and share other participant data.\n Convenient when you need to input the same participant multiple times."))
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForInlineShareMenu(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([DialogLineNodeInstance]() { return FText::FromString(DialogLineNodeInstance->SharedParticipantName); })),
					TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([DialogLineNodeInstance]() { return DialogLineNodeInstance->bSharedParticipant; })),
					FOnClicked::CreateSP(this, &FDialogNodeDetails::OnPromoteToSharedClick),
					FOnClicked::CreateSP(this, &FDialogNodeDetails::OnUnshareClick),
					FOnGetContent::CreateSP(this, &FDialogNodeDetails::OnGetShareableNodesMenu))
			]; 

		TSharedRef<IPropertyHandle> ParticipantInfoHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, ParticipantInfo));
		ParticipantInfoHandle->MarkHiddenByCustomization(); // Hide the root property row
		uint32 NumChildren = 0;
		ParticipantInfoHandle->GetNumChildren(NumChildren);
		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = ParticipantInfoHandle->GetChildHandle(ChildIndex);
			if (ChildHandle.IsValid())
			{
				DialogParticipantCategory.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}


	

	IDetailCategoryBuilder& DialogNodeDetail = DetailBuilder.EditCategory("Detail", LOCTEXT("DetailTitle", "Detail"));
	DialogNodeDetail.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, ID)).DisplayName(LOCTEXT("IDLabel", "ID"));

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> FDialogNodeDetails::GetWidgetForInlineShareMenu(const TAttribute<FText>& InSharedNameText, const TAttribute<bool>& bInIsCurrentlyShared, FOnClicked PromoteClick, FOnClicked DemoteClick, FOnGetContent GetContentMenu)
{
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ToolTipText(LOCTEXT("UseSharedParticipant_ToolTip", "Use Shared Participant"))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([bInIsCurrentlyShared, InSharedNameText]() { return bInIsCurrentlyShared.Get() ? InSharedNameText.Get() : LOCTEXT("SharedTransition", "Use Shared"); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(3.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([bInIsCurrentlyShared, DemoteClick, PromoteClick]() { return bInIsCurrentlyShared.Get() ? DemoteClick.Execute() : PromoteClick.Execute(); })
				.Text_Lambda([bInIsCurrentlyShared]() { return bInIsCurrentlyShared.Get() ? LOCTEXT("UnshareLabel", "Unshare") : LOCTEXT("ShareLabel", "New Shared Participant"); })
				.TextStyle(&FAppStyle::Get(), TEXT("TinyText"))
		];
}

/** RuleShare = true if we are sharing the rules of this participant (else we are implied to be sharing the participant settings) */
FReply FDialogNodeDetails::OnPromoteToSharedClick()
{
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (Parent.IsValid())
	{
		// Show dialog to enter new track name
		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label(LOCTEXT("PromoteDialogLineNodeToSharedLabel", "Shared Participant Name"))
			.OnTextCommitted(this, &FDialogNodeDetails::PromoteToShared);

		// Show dialog to enter new event name
		FSlateApplication::Get().PushMenu(
			Parent.ToSharedRef(),
			FWidgetPath(),
			TextEntry,
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
		);
		TextEntryWidget = TextEntry;
	}

	return FReply::Handled();
}

void FDialogNodeDetails::PromoteToShared(const FText& NewParticipantSettingName, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (DialogLineNode.Get())
		{
			DialogLineNode.Get()->MakeParticipantShareable(NewParticipantSettingName.ToString());
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

FReply FDialogNodeDetails::OnUnshareClick()
{
	if (DialogLineNode.Get())
	{
		DialogLineNode.Get()->UnshareParticipant();
	}

	return FReply::Handled();
}


TSharedRef<SWidget> FDialogNodeDetails::OnGetShareableNodesMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FText SectionText;

	SectionText = LOCTEXT("PickSharedParticipantSettings", "Shared Settings");
	

	MenuBuilder.BeginSection("ParticipantSharableNodes", SectionText);

	if (UDialogBuilderNode_DialogLine* DialogLineNodeInstance = DialogLineNode.Get())
	{
		const UDialogBuilderGraph* CurrentDialogGraph = DialogLineNode.Get()->GetOwningDialogGraph();

		// Collect all unique shared participant and group them by their name.
		TMultiMap<FString, UDialogBuilderNode_DialogLine*> SharedParticipants;

		
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		TArray<FAssetData> DialogGraphDataArray;
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UDialogBuilderGraph::StaticClass()), DialogGraphDataArray);

		/*for (const FAssetData& AssetData : DialogGraphDataArray)
		{
			UDialogBuilderGraph* DialogGraphAsset = Cast<UDialogBuilderGraph>(AssetData.GetAsset());
			if (DialogGraphAsset)
			{
				for (auto& Node : DialogGraphAsset->AllNodes)
				{
					UDialogBuilderNode_DialogLine* DialogLineNodeInst = Node ? Cast<UDialogBuilderNode_DialogLine>(Node) : nullptr;
					if (DialogLineNodeInst)
					{
						if (!DialogLineNodeInst->SharedParticipantName.IsEmpty())
						{
							SharedParticipants.Add(DialogLineNodeInst->SharedParticipantName, DialogLineNodeInst);
						}
					}
				}
			}
		}*/

		if (CurrentDialogGraph)
		{
			for (auto& Node : CurrentDialogGraph->AllNodes)
			{
				UDialogBuilderNode_DialogLine* DialogLineNodeInst = Node ? Cast<UDialogBuilderNode_DialogLine>(Node) : nullptr;
				if (DialogLineNodeInst)
				{
					if (!DialogLineNodeInst->SharedParticipantName.IsEmpty())
					{
						SharedParticipants.Add(DialogLineNodeInst->SharedParticipantName, DialogLineNodeInst);
					}
				}
			}
		}

		// Get the unique shared participant names
		TSet<FString> SharedParticipantKeys;
		SharedParticipants.GetKeys(SharedParticipantKeys);

		//Iterate through the unique shared participant names and list all the places where they are referenced in the tooltip.
		TArray<UDialogBuilderNode_DialogLine*> UsedIn;
		for (const FString& Key : SharedParticipantKeys)
		{
			UsedIn.Reset();
			SharedParticipants.MultiFind(Key, UsedIn, true);
			if (UsedIn.IsEmpty())
			{
				continue;
			}

			FTextBuilder ToolTipBuilder;
			ToolTipBuilder.AppendLine(LOCTEXT("DialogLineUsedBy", "Used by:"));
			TArray<FString> UsedInGraph;
			for (const UDialogBuilderNode_DialogLine* UsedInDialogGraph : UsedIn)
			{
				if(UsedInGraph.Contains(UsedInDialogGraph->GetOwningDialogGraph()->GetName()))
					continue;

				ToolTipBuilder.AppendLine(UsedInDialogGraph->GetOwningDialogGraph()->GetName());
				UsedInGraph.AddUnique(UsedInDialogGraph->GetOwningDialogGraph()->GetName());
			}

			FUIAction Action = FUIAction(FExecuteAction::CreateSP(this, &FDialogNodeDetails::BecomeSharedWith, UsedIn[0]));
			MenuBuilder.AddMenuEntry(FText::FromString(Key), ToolTipBuilder.ToText(), FSlateIcon(), Action);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FDialogNodeDetails::BecomeSharedWith(UDialogBuilderNode_DialogLine* NewNode)
{
	if (DialogLineNode.Get())
	{
		DialogLineNode.Get()->UseSharedParticipant(NewNode);
		
	}
}



#undef LOCTEXT_NAMESPACE

