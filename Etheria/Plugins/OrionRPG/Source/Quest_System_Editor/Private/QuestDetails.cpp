// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestDetails.h"

#include "Quest.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
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
#include "OrionRPG.h"
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
#include "QuestBuilderNode.h"
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

TSharedRef<IDetailCustomization> FQuestDetails::MakeInstance()
{
	return MakeShareable(new FQuestDetails);
}



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FQuestDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get a handle to the node we're viewing
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetSelectedObjects();
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			if (UQuest* QuestPtr = Cast<UQuest>(CurrentObject.Get()))
			{
				if (!Quest.IsValid())
				{
					Quest = QuestPtr;
				}
			}
			if (UQuestBuilderNode* NodePtr = Cast<UQuestBuilderNode>(CurrentObject.Get()))
			{
				if (!QuestNode.IsValid())
				{
					QuestNode = NodePtr;
				}
			}
		}
	}


	if (Quest != NULL && SelectedObjects.Num() == 1)
	{
		IDetailCategoryBuilder& QuestDetailCategory = DetailBuilder.EditCategory("QuestSettings", LOCTEXT("QuestCategoryTitle", "Quest Settings"));
		UQuest* QuestInstance = Quest.Get();
		// The sharing option for the crossfade settings
		QuestDetailCategory.AddCustomRow(LOCTEXT("QuestSettingsSharingLabel", "Quest Sharing"))
			.NameContent()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("QuestSharingLabel", "Quest Sharing"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("UseSharedQuest_ToolTip", "Quest Sharing let you access and share other Quest data.\n Convenient when you need to input the same Quest setting multiple times."))
			]
			.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				GetWidgetForInlineShareMenu(
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([QuestInstance]() { return FText::FromString(QuestInstance->SharedCategoryName); })),
					TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([QuestInstance]() { return QuestInstance->bSharedCategory; })),
					FOnClicked::CreateSP(this, &FQuestDetails::OnPromoteToSharedClick),
					FOnClicked::CreateSP(this, &FQuestDetails::OnUnshareClick),
					FOnGetContent::CreateSP(this, &FQuestDetails::OnGetShareableNodesMenu))
			]; 
		QuestDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuest, QuestCategory)).DisplayName(LOCTEXT("QuestCategoryLabel", "Quest Category"));
		QuestDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuest, bCanRetakeQuest)).DisplayName(LOCTEXT("QuestRetakeLabel", "Can Retake Quest"));
		QuestDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuest, bCanQuestBeAborted)).DisplayName(LOCTEXT("QuestAbortLabel", "Can Abort Quest"));
	}

	if (QuestNode != NULL && SelectedObjects.Num() == 1)
	{
		IDetailCategoryBuilder& QuestNodeDetailCategory = DetailBuilder.EditCategory("Description", LOCTEXT("DescriptionCategory", "Description"));
		QuestNodeDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuestBuilderNode, NodeTag)).DisplayName(LOCTEXT("NodeTagLabel", "Node Tag"));
		QuestNodeDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuestBuilderNode, Description)).DisplayName(LOCTEXT("DescriptionLabel", "Description"));
		QuestNodeDetailCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UQuestBuilderNode, NodeName)).DisplayName(LOCTEXT("NodeNameLabel", "Node Name"));
	}

	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> FQuestDetails::GetWidgetForInlineShareMenu(const TAttribute<FText>& InSharedNameText, const TAttribute<bool>& bInIsCurrentlyShared, FOnClicked PromoteClick, FOnClicked DemoteClick, FOnGetContent GetContentMenu)
{
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ToolTipText(LOCTEXT("UseSharedQuest_ToolTip", "Use Shared Quest Setting"))
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
				.Text_Lambda([bInIsCurrentlyShared]() { return bInIsCurrentlyShared.Get() ? LOCTEXT("UnshareLabel", "Unshare") : LOCTEXT("ShareLabel", "New Shared Quest Setting"); })
				.TextStyle(&FAppStyle::Get(), TEXT("TinyText"))
		];
}

/** RuleShare = true if we are sharing the rules of this Quest (else we are implied to be sharing the Quest settings) */
FReply FQuestDetails::OnPromoteToSharedClick()
{
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (Parent.IsValid())
	{
		// Show dialog to enter new track name
		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label(LOCTEXT("PromoteQuestToSharedLabel", "Shared Quest Name"))
			.OnTextCommitted(this, &FQuestDetails::PromoteToShared);

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

void FQuestDetails::PromoteToShared(const FText& NewQuestSettingName, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (Quest.Get())
		{
			Quest.Get()->MakeQuestSettingShareable(NewQuestSettingName.ToString());
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

FReply FQuestDetails::OnUnshareClick()
{
	if (Quest.Get())
	{
		Quest.Get()->UnshareQuestSetting();
	}

	return FReply::Handled();
}

TSharedRef<SWidget> FQuestDetails::OnGetShareableNodesMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FText SectionText;

	SectionText = LOCTEXT("PickSharedQuestSettings", "Shared Settings");
	

	MenuBuilder.BeginSection("SharableQuests", SectionText);

	if (UQuest* QuestInstance = Quest.Get())
	{

		// Collect all unique shared Quest and group them by their name.
		TMultiMap<FString, UQuest*> SharedQuests;
		
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		TArray<FAssetData> DialogGraphDataArray;
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), DialogGraphDataArray);

		for (const FAssetData& AssetData : DialogGraphDataArray)
		{
			UQuestBuilderGraph* QuestGraphAsset = Cast<UQuestBuilderGraph>(AssetData.GetAsset());
			if (QuestGraphAsset)
			{
				for (auto& OtherQuest : QuestGraphAsset->QuestList)
				{
					if(OtherQuest)
					{
						if (!OtherQuest->SharedCategoryName.IsEmpty())
						{
							SharedQuests.Add(OtherQuest->SharedCategoryName, OtherQuest);
						}
					}
				}
			}
		}

		FOrionRPGModule& OrionRPGModule = FModuleManager::GetModuleChecked<FOrionRPGModule>(TEXT("OrionRPG"));
		for (auto& QuestPreset : OrionRPGModule.QuestPresets)
		{
			if (QuestPreset && !QuestPreset->SharedCategoryName.IsEmpty())
			{
				SharedQuests.Add(QuestPreset->SharedCategoryName, QuestPreset);
			}
		}
		
		// Get the unique shared Quest names
		TSet<FString> SharedQuestKeys;
		SharedQuests.GetKeys(SharedQuestKeys);

		//Iterate through the unique shared Quest names and list all the places where they are referenced in the tooltip.
		TArray<UQuest*> UsedIn;
		for (const FString& Key : SharedQuestKeys)
		{
			UsedIn.Reset();
			SharedQuests.MultiFind(Key, UsedIn, true);
			if (UsedIn.IsEmpty())
			{
				continue;
			}

			FTextBuilder ToolTipBuilder;
			ToolTipBuilder.AppendLine(LOCTEXT("CategoryUsedBy", "Used by:"));
			TArray<FString> UsedInGraph;
			for (const UQuest* UsedInQuestGraph : UsedIn)
			{
				if(!UsedInQuestGraph->GetOwningQuestGraph() ||
					UsedInGraph.Contains(UsedInQuestGraph->GetOwningQuestGraph()->GetName()))
					continue;

				ToolTipBuilder.AppendLine(UsedInQuestGraph->GetOwningQuestGraph()->GetName());
				UsedInGraph.AddUnique(UsedInQuestGraph->GetOwningQuestGraph()->GetName());
			}

			FUIAction Action = FUIAction(FExecuteAction::CreateSP(this, &FQuestDetails::BecomeSharedWith, UsedIn[0]));
			MenuBuilder.AddMenuEntry(FText::FromString(Key), ToolTipBuilder.ToText(), FSlateIcon(), Action);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FQuestDetails::BecomeSharedWith(UQuest* NewQuest)
{
	if (Quest.Get())
	{
		Quest.Get()->UseSharedQuestSetting(NewQuest);
	}
}


















#undef LOCTEXT_NAMESPACE

