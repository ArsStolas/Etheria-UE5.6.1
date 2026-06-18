// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogNodeDetails.h"

#include "DialogBuilderEdNode_DialogLine.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderEdNode_PlayerLine.h"
#include "DialogBuilderNode_PlayerLine.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderEditor.h"
#include "DialogStage.h"
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
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "DetailsCustomization"

/////////////////////////////////////////////////////////////////////////

TSharedRef<IDetailCustomization> FDialogNodeDetails::MakeInstance()
{
	return MakeShareable(new FDialogNodeDetails);
}



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FDialogNodeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DialogSequenceNode.Reset();
	DialogLineNode.Reset();
	PlayerLineNode.Reset();
	PlayerChoiceNode.Reset();

	// Get a handle to the node we're viewing
	const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailBuilder.GetSelectedObjects();
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			if (UDialogBuilderNode_PlayerChoice* PlayerChoiceNodePtr = Cast<UDialogBuilderNode_PlayerChoice>(CurrentObject.Get()))
			{
				if (!PlayerChoiceNode.IsValid())
				{
					PlayerChoiceNode = PlayerChoiceNodePtr;
				}
			}

			if (UDialogBuilderNode_DialogSequence* SequenceNodePtr = Cast<UDialogBuilderNode_DialogSequence>(CurrentObject.Get()))
			{
				if (!DialogSequenceNode.IsValid())
				{
					DialogSequenceNode = SequenceNodePtr;
				}
			}

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

	//Player Line - Player Configuration
	IDetailCategoryBuilder& PlayerConfigurationCategory = DetailBuilder.EditCategory("PlayerConfiguration", LOCTEXT("PlayerConfigurationCategoryTitle", "Player Configuration"));
	if (PlayerLineNode != NULL && SelectedObjects.Num() == 1)
	{
		UDialogBuilderNode_PlayerLine* PlayerLineNodeInstance = PlayerLineNode.Get();
		UDialogBuilderGraph* DialogGraph = PlayerLineNodeInstance ? PlayerLineNodeInstance->GetOwningDialogGraph() : nullptr;

		if (DialogGraph)
		{
			TArray<UObject*> ExternalObjects;
			ExternalObjects.Add(DialogGraph);

			PlayerConfigurationCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, PlayerName));

			PlayerConfigurationCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, PlayerTransformOverride));


			PlayerConfigurationCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, DefaultPlayerShot));

			PlayerConfigurationCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, DefaultSelectingChoiceShot));

			PlayerConfigurationCategory.AddExternalObjectProperty(
				ExternalObjects,
				GET_MEMBER_NAME_CHECKED(UDialogBuilderGraph, DefaultPlayerImage));
		}

	}

	//Dialog Line - Participant Category
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


	// Dialog Sequence - Dialog Set
	
	const bool bIsSingleSelection = SelectedObjects.Num() == 1;
	const bool bCanShowSequenceMenu = DialogSequenceNode.IsValid();



	// Dialog Sequence / Choice Selection

	if (bIsSingleSelection)
	{
		if (PlayerChoiceNode.IsValid())
		{
			IDetailCategoryBuilder& ChoiceSelectionCategory = DetailBuilder.EditCategory("Choice Selection", LOCTEXT("ChoiceSelectionTitle", "Choice Selection"));

			const TSharedRef<IPropertyHandle> SelectionTypeHandle =
				DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, SelectionType));

			const TSharedRef<IPropertyHandle> SelectingChoiceShotOverrideHandle =
				DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, SelectingChoiceShotOverride));

			const TSharedRef<IPropertyHandle> SelectionTimeLimitHandle =
				DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, SelectionTimeLimit));

			const TSharedRef<IPropertyHandle> TimeLimitHandle =
				DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, TimeLimit));

			SelectionTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&DetailBuilder]()
			{
				DetailBuilder.ForceRefreshDetails();
			}));

			ChoiceSelectionCategory.AddProperty(SelectionTypeHandle);
			ChoiceSelectionCategory.AddProperty(SelectingChoiceShotOverrideHandle);
			ChoiceSelectionCategory.AddProperty(SelectionTimeLimitHandle);
			ChoiceSelectionCategory.AddProperty(TimeLimitHandle);

			if (PlayerChoiceNode->SelectionType == EDialogSelectionType::E_Sequence)
			{
				UDialogBuilderNode_DialogSequence* DialogSequenceNodeInstance = DialogSequenceNode.Get();

				ChoiceSelectionCategory.AddCustomRow(LOCTEXT("DialogStageSettingsLabel", "DialogStage To Use"))
					.NameContent()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("DialogStageLabel", "DialogStage To Use"))
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.ToolTipText(LOCTEXT("DialogStageSettings_ToolTip", "Pick which dialog set to use for this sequence"))
					]
					.ValueContent()
					.MaxDesiredWidth(300.0f)
					[
						GetWidgetForDialogStagePicker(
							TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([DialogSequenceNodeInstance]()
							{
								return FText::FromString(DialogSequenceNodeInstance->DialogStageToUse ? DialogSequenceNodeInstance->DialogStageToUse->Name.ToString() : "None");
							})),
							FOnClicked::CreateSP(this, &FDialogNodeDetails::OnPromoteToSharedClick),
							FOnGetContent::CreateSP(this, &FDialogNodeDetails::OnGetDialogStageListMenu))
					];

				ChoiceSelectionCategory.AddCustomRow(LOCTEXT("OpenSequenceRowLabel", "Open Sequence"))
					.WholeRowContent()
					[
						SNew(SBorder)
							.Padding(FMargin(0.0f, 6.0f, 0.0f, 5.0f))
							.BorderImage(FAppStyle::GetBrush("NoBorder"))
							[
								SNew(SButton)
									.HAlign(HAlign_Center)
									.Text(LOCTEXT("OpenSequenceButtonLabel", "Open Sequence"))
									.OnClicked_Lambda([DialogSequenceNodeInstance]()
									{
										if (DialogSequenceNodeInstance && DialogSequenceNodeInstance->GetOwningDialogGraph())
										{
											UDialogBuilderEdGraph* DialogGraph =
												DialogSequenceNodeInstance->GetOwningDialogGraph()->DialogGraphPages.IsValidIndex(0)
												? Cast<UDialogBuilderEdGraph>(DialogSequenceNodeInstance->GetOwningDialogGraph()->DialogGraphPages[0])
												: nullptr;

											FDialogBuilderEditor* DialogBuilderEditor = DialogGraph ? DialogGraph->DialogEditorPtr.Pin().Get() : nullptr;
											if (DialogBuilderEditor)
											{
												DialogBuilderEditor->OnOpenDialogSequenceNode(DialogSequenceNodeInstance);
											}
										}

										return FReply::Handled();
									})
							]
					];
			}
		}
		else if (DialogSequenceNode.IsValid())
		{
			IDetailCategoryBuilder& DialogSequenceCategory = DetailBuilder.EditCategory("Dialog Sequence", LOCTEXT("DialogSqeuenceTitle", "Dialog Sequence"));
			UDialogBuilderNode_DialogSequence* DialogSequenceNodeInstance = DialogSequenceNode.Get();

			DialogSequenceCategory.AddCustomRow(LOCTEXT("DialogStageSettingsLabel", "DialogStage To Use"))
				.NameContent()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DialogStageLabel", "DialogStage To Use"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.ToolTipText(LOCTEXT("DialogStageSettings_ToolTip", "Pick which dialog set to use for this sequence"))
				]
				.ValueContent()
				.MaxDesiredWidth(300.0f)
				[
					GetWidgetForDialogStagePicker(
						TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([DialogSequenceNodeInstance]()
						{
							return FText::FromString(DialogSequenceNodeInstance->DialogStageToUse ? DialogSequenceNodeInstance->DialogStageToUse->Name.ToString() : "None");
						})),
						FOnClicked::CreateSP(this, &FDialogNodeDetails::OnPromoteToSharedClick),
						FOnGetContent::CreateSP(this, &FDialogNodeDetails::OnGetDialogStageListMenu))
				];

			DialogSequenceCategory.AddCustomRow(LOCTEXT("OpenSequenceRowLabel", "Open Sequence"))
				.WholeRowContent()
				[
					SNew(SBorder)
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 5.0f))
						.BorderImage(FAppStyle::GetBrush("NoBorder"))
						[
							SNew(SButton)
								.HAlign(HAlign_Center)
								.Text(LOCTEXT("OpenSequenceButtonLabel", "Open Sequence"))
								.OnClicked_Lambda([DialogSequenceNodeInstance]()
								{
									if (DialogSequenceNodeInstance && DialogSequenceNodeInstance->GetOwningDialogGraph())
									{
										UDialogBuilderEdGraph* DialogGraph =
											DialogSequenceNodeInstance->GetOwningDialogGraph()->DialogGraphPages.IsValidIndex(0)
											? Cast<UDialogBuilderEdGraph>(DialogSequenceNodeInstance->GetOwningDialogGraph()->DialogGraphPages[0])
											: nullptr;

										FDialogBuilderEditor* DialogBuilderEditor = DialogGraph ? DialogGraph->DialogEditorPtr.Pin().Get() : nullptr;
										if (DialogBuilderEditor)
										{
											DialogBuilderEditor->OnOpenDialogSequenceNode(DialogSequenceNodeInstance);
										}
									}

									return FReply::Handled();
								})
						]
				];
		}
	}


	IDetailCategoryBuilder& DialogNodeDetail = DetailBuilder.EditCategory("Detail", LOCTEXT("DetailTitle", "Detail"));
	DialogNodeDetail.AddProperty(GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_DialogLine, ID)).DisplayName(LOCTEXT("IDLabel", "ID"));

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> FDialogNodeDetails::GetWidgetForDialogStagePicker(const TAttribute<FText>& InDialogStageNameText, FOnClicked PromoteClick, FOnGetContent GetContentMenu)
{
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ToolTipText(LOCTEXT("DialogStage_ToolTip", "DialogStage to use"))
				.OnGetMenuContent(GetContentMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([InDialogStageNameText]() { return  InDialogStageNameText.Get(); })
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];

}

TSharedRef<SWidget> FDialogNodeDetails::OnGetDialogStageListMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FText SectionText;

	SectionText = LOCTEXT("DialogStageList", "DialogStage List");


	MenuBuilder.BeginSection("DialogStageListSection", SectionText);

	if (UDialogBuilderNode_DialogSequence* DialogSequenceInstance = DialogSequenceNode.Get())
	{
		const UDialogBuilderGraph* CurrentDialogGraph = DialogSequenceInstance->GetOwningDialogGraph();


		for (const auto& DialogStage : CurrentDialogGraph->DialogStages)
		{
			if (DialogStage)
			{
				FTextBuilder ToolTipBuilder;
				FText DialogStageName = FText::FromString(DialogStage->Name.ToString());
				// Pass raw pointer obtained from TObjectPtr via .Get() so the delegate argument type matches UDialogStage*
				FUIAction Action = FUIAction(FExecuteAction::CreateSP(this, &FDialogNodeDetails::UseDialogStage, DialogStage.Get()));
				MenuBuilder.AddMenuEntry(DialogStageName, ToolTipBuilder.ToText(), FSlateIcon(), Action);
			}
		}
		
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FDialogNodeDetails::UseDialogStage(UDialogStage* InDialogStage)
{
	if (UDialogBuilderNode_DialogSequence* SequenceNode = DialogSequenceNode.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("SetDialogStage", "Set Dialog Stage"));
		SequenceNode->UseDialogStage(InDialogStage);
	}
}


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
	if (UDialogBuilderNode_DialogLine* DialogLine = DialogLineNode.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("BecomeSharedWith", "Set Shared Participant"));
		DialogLine->Modify();
		DialogLine->UseSharedParticipant(NewNode);
	}
}




#undef LOCTEXT_NAMESPACE

