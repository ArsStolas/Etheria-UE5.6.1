#include "SDialogStageManager.h"

#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogDefinition.h"
#include "DialogSequence.h"
#include "DialogBuilder_EditorStyle.h"
#include "DialogStage.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"

#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "DialogStageManager"


class SDialogStagePaletteItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogStagePaletteItem) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDialogStage>, DialogStage)
		SLATE_ARGUMENT(TWeakPtr<FDialogBuilderEditor>, DialogEditorPtr)
		SLATE_EVENT(FOnTextCommitted, OnNameCommitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		DialogStage = InArgs._DialogStage;
		DialogEditorPtr = InArgs._DialogEditorPtr;
		OnNameCommitted = InArgs._OnNameCommitted;

		ChildSlot
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(7.0f, 4.0f, 4.0f, 4.0f))
				.VAlign(VAlign_Center)
				[
					SNew(SInlineEditableTextBlock)
					.Text_Lambda([this]()
						{
							const TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();

							if (const UDialogStage* LocalSet = DialogStage.Get())
							{
								const bool bIsCurrentStage =
									DialogEditor.IsValid() &&
									LocalSet == DialogEditor->GetDialogStageTemplate();

								const FText StageName = LocalSet->Name.IsEmpty()
									? LOCTEXT("UnnamedDialogStage", "None")
									: LocalSet->Name;

								return bIsCurrentStage
									? FText::Format(LOCTEXT("CurrentDialogStageLabel", "{0} (current)"), StageName)
									: StageName;
							}

							return LOCTEXT("InvalidDialogStage", "Invalid");
						})
					.ColorAndOpacity_Lambda([this]()
						{
							const TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();

							if (const UDialogStage* LocalSet = DialogStage.Get())
							{
								const bool bIsCurrentStage =
									DialogEditor.IsValid() &&
									LocalSet == DialogEditor->GetDialogStageTemplate();

								return bIsCurrentStage
									? FSlateColor(FColor::Turquoise)
									: FSlateColor::UseForeground();
							}

							return FSlateColor::UseForeground();
						})
					.OnTextCommitted(OnNameCommitted)
					.IsReadOnly(false)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.0f))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(2.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SAssignNew(ExportButton, SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(LOCTEXT("ExportDialogStageTooltip", "Export Dialog Stage as Template"))
						.OnClicked_Lambda([this]()
							{
								const TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();
								UDialogStage* LocalStage = DialogStage.Get();

								if (DialogEditor.IsValid() && LocalStage)
								{
									DialogEditor->CreateNewDialogStageTemplate(LocalStage);
								}

								return FReply::Handled();
							})
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("LevelEditor.CreateClassBlueprint"))
								.ColorAndOpacity_Lambda([this]()
									{
										return (ExportButton.IsValid() && ExportButton->IsHovered())
											? FLinearColor::White
											: FLinearColor(1.0f, 1.0f, 1.0f, 0.4f);
									})
						]
				]
			];
	}

private:
	TWeakObjectPtr<UDialogStage> DialogStage;
	TWeakPtr<FDialogBuilderEditor> DialogEditorPtr;
	FOnTextCommitted OnNameCommitted;
	TSharedPtr<SButton> ExportButton;
};

class SDialogSlotPaletteItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogSlotPaletteItem) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDialogSequenceSlot>, Slot)
		SLATE_ARGUMENT(bool, IsCameraSlot)
		SLATE_ARGUMENT(bool, IsLightSlot)
		SLATE_ARGUMENT(int32, SlotIndex)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Slot = InArgs._Slot;
		bIsCameraSlot = InArgs._IsCameraSlot;
		bIsLightSlot = InArgs._IsLightSlot;
		SlotIndex = InArgs._SlotIndex;

		ChildSlot
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(FMargin(7.0f, 4.0f, 4.0f, 4.0f))
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text_Lambda([this]()
								{
									const UDialogSequenceSlot* LocalSlot = Slot.Get();
									if (!LocalSlot)
									{
										return LOCTEXT("InvalidSlot", "Invalid");
									}

									const UDialogDefinition* DialogDef = LocalSlot->DialogDefinition;
									const FString Label = DialogDef ? DialogDef->DisplayName.ToString() : TEXT("None");

									if (bIsLightSlot)
									{
										return FText::FromString(FString::Printf(TEXT("Light %d"), SlotIndex +1));
									}
									if (bIsCameraSlot)
									{
										return FText::FromString(FString::Printf(TEXT("%s"), *Label));
									}
									return FText::FromString(Label);
								})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.0f))
					]
			];
	}

private:
	TWeakObjectPtr<UDialogSequenceSlot> Slot;
	bool bIsCameraSlot = false;
	bool bIsLightSlot = false;
	int32 SlotIndex = INDEX_NONE;
};

// -----------------------------------------------------------------------------
// SDialogStageManager
// -----------------------------------------------------------------------------
void SDialogStageManager::Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor,  UDialogBuilderGraph* InDialogGraph)
{
	DialogGraph = InDialogGraph;
	DialogEditorPtr = InDialogEditor;
	CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SDialogStageManager::OnDeleteSelection),
		FCanExecuteAction::CreateSP(this, &SDialogStageManager::CanDeleteSelection));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

	DialogStageDetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	SlotDetailsView = PropertyModule.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SSplitter)

		// 1) DialogStage List
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("DialogStageListTitle", "Dialog Stages"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f, 0.0f, 0.0f)
					[
						SAssignNew(ImportDialogStageComboButton, SComboButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(2.f))
							.ToolTipText(LOCTEXT("ImportDialogStageTooltip", "Import Dialog Stage"))
							.HasDownArrow(false)
							.ButtonContent()
							[
								SNew(SImage)
									.Image(FAppStyle::GetBrush("Icons.Import"))
							]
							.OnGetMenuContent(this, &SDialogStageManager::BuildImportDialogStageMenu)

					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.f))
						.ToolTipText(LOCTEXT("AddDialogStageTooltip", "Add Dialog Stage"))
						.OnClicked(this, &SDialogStageManager::OnAddDialogStage)
						[
							SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
						]
					]
					

				]
				
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					SAssignNew(DialogStageListView, SListView<FDialogStageListItemPtr>)
					.ListItemsSource(&DialogStageItems)
					.OnGenerateRow(this, &SDialogStageManager::GenerateDialogStageRow)
					.OnSelectionChanged(this, &SDialogStageManager::OnDialogStageSelectionChanged)
					.OnContextMenuOpening(this, &SDialogStageManager::OnDialogStageContextMenuOpening)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		]

		// 2) DialogStage Details
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				DialogStageDetailsView.ToSharedRef()
			]
		]

		+ SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						SNew(SSplitter)
							.Orientation(Orient_Vertical)

							// A) Regular Slots (participants/props/camera)
							+ SSplitter::Slot()
							.Value(0.5f)
							[
								SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(4.0f)
									[
										SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("SlotListTitle", "Slots"))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
											]
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.Padding(0.f, 0.f, 2.f, 0.f)
											[
												SNew(SButton)
													.ButtonStyle(FAppStyle::Get(), "SimpleButton")
													.ContentPadding(FMargin(2.f))
													.ToolTipText(LOCTEXT("AddSlotTooltip", "Add Slot"))
													.OnClicked(this, &SDialogStageManager::OnAddSlot)
													[
														SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
													]
											]
									]

								+ SVerticalBox::Slot()
									.FillHeight(1.0f)
									.Padding(2.0f)
									[
										SAssignNew(SlotListView, SListView<FDialogSlotListItemPtr>)
											.ListItemsSource(&SlotItems)
											.OnGenerateRow(this, &SDialogStageManager::GenerateSlotRow)
											.OnSelectionChanged(this, &SDialogStageManager::OnSlotSelectionChanged)
											.OnContextMenuOpening(this, &SDialogStageManager::OnSlotContextMenuOpening)
											.SelectionMode(ESelectionMode::Single)
									]
							]

						// B) Light Slots
						+ SSplitter::Slot()
							.Value(0.5f)
							[
								SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(4.0f)
									[
										SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("LightSlotListTitle", "Light Slots"))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
											]
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.Padding(0.f, 0.f, 2.f, 0.f)
											[
												SNew(SButton)
													.ButtonStyle(FAppStyle::Get(), "SimpleButton")
													.ContentPadding(FMargin(2.f))
													.ToolTipText(LOCTEXT("AddLightSlotTooltip", "Add Light Slot"))
													.OnClicked(this, &SDialogStageManager::OnAddLightSlot)
													[
														SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
													]
											]
									]

								+ SVerticalBox::Slot()
									.FillHeight(1.0f)
									.Padding(2.0f)
									[
										SAssignNew(LightSlotListView, SListView<FDialogSlotListItemPtr>)
											.ListItemsSource(&LightSlotItems)
											.OnGenerateRow(this, &SDialogStageManager::GenerateLightSlotRow)
											.OnSelectionChanged(this, &SDialogStageManager::OnLightSlotSelectionChanged)
											.OnContextMenuOpening(this, &SDialogStageManager::OnLightSlotContextMenuOpening)
											.SelectionMode(ESelectionMode::Single)
									]
							]
					]
			]

		// 4) Slot Details
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				SlotDetailsView.ToSharedRef()
			]
		]
	];

	RefreshDialogStageList();


	//Try select when initialize
	if (DialogStageItems.Num() > 0)
	{
		FDialogStageListItemPtr ItemToSelect = nullptr;
		if (TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin())
		{
			if (UDialogStage* CurrentStage = DialogEditor->GetDialogStageTemplate())
			{
				for (const FDialogStageListItemPtr& Item : DialogStageItems)
				{
					if (Item->DialogStage.Get() == CurrentStage)
					{
						ItemToSelect = Item;
						break;
					}
				}
			}
		}

		if (!ItemToSelect.IsValid())
		{
			ItemToSelect = DialogStageItems[0];
		}

		if (DialogStageListView.IsValid())
		{
			DialogStageListView->SetSelection(ItemToSelect, ESelectInfo::Direct);
			ApplyDialogStageSelection(ItemToSelect);
		}
	}
}

FReply SDialogStageManager::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() && CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

TSharedRef<ITableRow> SDialogStageManager::GenerateDialogStageRow(FDialogStageListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable)
{
	const TWeakObjectPtr<UDialogStage> WeakDialogStage = InItem.IsValid() ? InItem->DialogStage : nullptr;

	

	return SNew(STableRow<FDialogStageListItemPtr>, InOwnerTable)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(0.0f))
				.OnClicked_Lambda([this, InItem]()
					{
						return OnDialogStageItemClicked(InItem);
					})
				[
					SNew(SDialogStagePaletteItem)
						.DialogStage(WeakDialogStage)
						.DialogEditorPtr(DialogEditorPtr)
						.OnNameCommitted(FOnTextCommitted::CreateSP(this, &SDialogStageManager::OnDialogStageNameCommitted, WeakDialogStage))
				]
		];
}

FReply SDialogStageManager::OnDialogStageItemClicked(FDialogStageListItemPtr InItem)
{
	if (!InItem.IsValid())
	{
		return FReply::Handled();
	}

	if (DialogStageListView.IsValid())
	{
		const bool bWasSelected = DialogStageListView->IsItemSelected(InItem);
		DialogStageListView->SetSelection(InItem, ESelectInfo::Direct);

		if (bWasSelected)
		{
			ApplyDialogStageSelection(InItem);
		}
	}
	else
	{
		ApplyDialogStageSelection(InItem);
	}

	return FReply::Handled();
}

void SDialogStageManager::ApplyDialogStageSelection(FDialogStageListItemPtr InItem)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();
	SelectedDialogStage = InItem.IsValid() ? InItem->DialogStage.Get() : nullptr;
	SelectedSlot.Reset();
	SelectedSlotIndex = INDEX_NONE;
	bSelectedSlotIsLight = false;

	if (DialogStageDetailsView.IsValid())
	{
		AActor* SequencePivot = DialogEditor ? DialogEditor->GetSequencePivot() : nullptr;

		if (SelectedDialogStage == DialogEditor->GetDialogStageTemplate())
		{
			DialogEditor->SelectActor(SequencePivot);
		}
		else
		{
			DialogEditor->SelectActor(nullptr);
		}
		

		DialogStageDetailsView->SetObject(SelectedDialogStage.Get());
	}

	if (SlotDetailsView.IsValid())
	{
		SlotDetailsView->SetObject(nullptr);
	}

	RefreshSlotList();
	RefreshLightSlotList();
}

void SDialogStageManager::OnDialogStageNameCommitted(const FText& InText, ETextCommit::Type InCommitType, TWeakObjectPtr<UDialogStage> InDialogStage)
{
	UDialogStage* DialogStage = InDialogStage.Get();
	if (!DialogStage)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("RenameDialogStageTransaction", "Rename Dialog Stage"));
	DialogStage->Modify();
	DialogStage->Name = InText;

	MarkGraphDirty();
	RefreshDialogStageList();
}

void SDialogStageManager::OnDialogStageSelectionChanged(FDialogStageListItemPtr InItem, ESelectInfo::Type InSelectInfo)
{
	ApplyDialogStageSelection(InItem);
}

FReply SDialogStageManager::OnAddDialogStage()
{
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!Graph)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddDialogStageTransaction", "Add Dialog Stage"));
	Graph->Modify();

	UDialogStage* NewDialogStage = NewObject<UDialogStage>(Graph, NAME_None, RF_Transactional);
	if (NewDialogStage)
	{
		NewDialogStage->OwningDialogGraph = Graph;
		NewDialogStage->Name = FText::FromString(FString::Printf(TEXT("Stage %d"), Graph->DialogStages.Num() + 1));
		Graph->DialogStages.Add(NewDialogStage);

		MarkGraphDirty();
		RefreshDialogStageList();

		SelectedDialogStage = NewDialogStage;
		if (DialogStageDetailsView.IsValid())
		{
			DialogStageDetailsView->SetObject(NewDialogStage);
		}
		RefreshSlotList();
	}

	return FReply::Handled();
}

void SDialogStageManager::OnRemoveDialogStage(UDialogStage* InDialogStage)
{
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!Graph || !InDialogStage)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteDialogStageTransaction", "Delete Dialog Stage"));
	Graph->Modify();
	InDialogStage->Modify();

	Graph->DialogStages.Remove(InDialogStage);

	if (SelectedDialogStage.Get() == InDialogStage)
	{
		SelectedDialogStage.Reset();
		SelectedSlot.Reset();

		if (DialogStageDetailsView.IsValid())
		{
			DialogStageDetailsView->SetObject(nullptr);
		}
		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(nullptr);
		}
	}

	MarkGraphDirty();
	RefreshDialogStageList();
	RefreshSlotList();
}

TSharedRef<ITableRow> SDialogStageManager::GenerateSlotRow(FDialogSlotListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable)
{
	const TWeakObjectPtr<UDialogSequenceSlot> WeakSlot = InItem.IsValid() ? InItem->Slot : nullptr;
	const bool bCamera = InItem.IsValid() ? InItem->bIsCameraSlot : false;

	return SNew(STableRow<FDialogSlotListItemPtr>, InOwnerTable)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(0.0f))
				.OnClicked_Lambda([this, InItem]()
					{
						return OnSlotItemClicked(InItem);
					})
				[
					SNew(SDialogSlotPaletteItem)
						.Slot(WeakSlot)
						.IsCameraSlot(bCamera)
						.IsLightSlot(false)
						.SlotIndex(InItem->SlotIndex)
				]
		];
}

FReply SDialogStageManager::OnSlotItemClicked(FDialogSlotListItemPtr InItem)
{
	if (!InItem.IsValid())
	{
		return FReply::Handled();
	}

	if (SlotListView.IsValid())
	{
		const bool bWasSelected = SlotListView->IsItemSelected(InItem);
		SlotListView->SetSelection(InItem, ESelectInfo::Direct);

		if (bWasSelected)
		{
			ApplySlotSelection(InItem);
		}
	}
	else
	{
		ApplySlotSelection(InItem);
	}

	return FReply::Handled();
}

void SDialogStageManager::ApplySlotSelection(FDialogSlotListItemPtr InItem)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();
	SelectedSlot = InItem.IsValid() ? InItem->Slot.Get() : nullptr;
	SelectedSlotIndex = InItem.IsValid() ? InItem->SlotIndex : INDEX_NONE;
	bSelectedSlotIsLight = InItem.IsValid() ? InItem->bIsLightSlot : false;

	if (SlotDetailsView.IsValid())
	{
		SlotDetailsView->SetObject(SelectedSlot.Get());

		if (DialogEditor && !bSelectedSlotIsLight)
		{
			DialogEditor->SelectDialogSlotActor(SelectedSlotIndex);
		}
		else
		{
			DialogEditor->SelectLightSlotActor(SelectedSlotIndex);
		}
	}
}

void SDialogStageManager::OnSlotSelectionChanged(FDialogSlotListItemPtr InItem, ESelectInfo::Type InSelectInfo)
{
	ApplySlotSelection(InItem);
}

FReply SDialogStageManager::OnAddSlot()
{
	UDialogStage* DialogStage = SelectedDialogStage.Get();
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!DialogStage || !Graph)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddSlotTransaction", "Add Slot"));
	DialogStage->Modify();

	UDialogSequenceSlot* NewSlot = NewObject<UDialogSequenceSlot>(DialogStage, NAME_None, RF_Transactional);
	if (NewSlot)
	{
		NewSlot->OwningDialogGraph = Graph;
		DialogStage->Slots.Add(NewSlot);

		MarkGraphDirty();
		RefreshSlotList();

		SelectedSlot = NewSlot;
		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(NewSlot);
		}
	}

	return FReply::Handled();
}

FReply SDialogStageManager::OnAddCameraSlot()
{
	UDialogStage* DialogStage = SelectedDialogStage.Get();
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!DialogStage || !Graph)	
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddCameraSlotTransaction", "Add Camera Slot"));
	DialogStage->Modify();

	UDialogSequenceSlot* NewSlot = NewObject<UDialogSequenceSlot>(DialogStage, NAME_None, RF_Transactional);
	if (NewSlot)
	{
		NewSlot->OwningDialogGraph = Graph;
		DialogStage->CameraSlots.Add(NewSlot);

		MarkGraphDirty();
		RefreshSlotList();

		SelectedSlot = NewSlot;
		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(NewSlot);
		}
	}

	return FReply::Handled();
}

TSharedRef<ITableRow> SDialogStageManager::GenerateLightSlotRow(FDialogSlotListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable)
{
	const TWeakObjectPtr<UDialogSequenceSlot> WeakSlot = InItem.IsValid() ? InItem->Slot : nullptr;

	return SNew(STableRow<FDialogSlotListItemPtr>, InOwnerTable)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(0.0f))
				.OnClicked_Lambda([this, InItem]()
					{
						return OnLightSlotItemClicked(InItem);
					})
				[
					SNew(SDialogSlotPaletteItem)
						.Slot(WeakSlot)
						.IsCameraSlot(false)
						.IsLightSlot(true)
						.SlotIndex(InItem->SlotIndex)
				]
		];
}

FReply SDialogStageManager::OnLightSlotItemClicked(FDialogSlotListItemPtr InItem)
{
	if (!InItem.IsValid())
	{
		return FReply::Handled();
	}

	if (LightSlotListView.IsValid())
	{
		const bool bWasSelected = LightSlotListView->IsItemSelected(InItem);
		LightSlotListView->SetSelection(InItem, ESelectInfo::Direct);

		if (bWasSelected)
		{
			ApplySlotSelection(InItem);
		}
	}
	else
	{
		ApplySlotSelection(InItem);
	}

	return FReply::Handled();
}

void SDialogStageManager::OnLightSlotSelectionChanged(FDialogSlotListItemPtr InItem, ESelectInfo::Type InSelectInfo)
{
	ApplySlotSelection(InItem);
}

FReply SDialogStageManager::OnAddLightSlot()
{
	UDialogStage* DialogStage = SelectedDialogStage.Get();
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!DialogStage || !Graph)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddLightSlotTransaction", "Add Light Slot"));
	DialogStage->Modify();

	UDialogSequenceSlot_Light* NewSlot = NewObject<UDialogSequenceSlot_Light>(DialogStage, NAME_None, RF_Transactional);
	if (NewSlot)
	{
		NewSlot->OwningDialogGraph = Graph;
		DialogStage->LightSlots.Add(NewSlot);

		MarkGraphDirty();
		RefreshLightSlotList();

		SelectedSlot = NewSlot;
		SelectedSlotIndex = DialogStage->LightSlots.Num() - 1;
		bSelectedSlotIsLight = true;

		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(NewSlot);
		}
	}

	return FReply::Handled();
}

void SDialogStageManager::OnRemoveLightSlot(UDialogSequenceSlot_Light* InSlot, int32 InSlotIndex)
{
	UDialogStage* DialogStage = SelectedDialogStage.Get();
	if (!DialogStage || !InSlot)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteLightSlotTransaction", "Delete Light Slot"));
	DialogStage->Modify();
	InSlot->Modify();

	if (DialogStage->LightSlots.IsValidIndex(InSlotIndex))
	{
		DialogStage->LightSlots.RemoveAt(InSlotIndex);
	}
	else
	{
		DialogStage->LightSlots.RemoveSingle(InSlot);
	}

	if (SelectedSlot.Get() == InSlot)
	{
		SelectedSlot.Reset();
		SelectedSlotIndex = INDEX_NONE;
		bSelectedSlotIsLight = false;

		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(nullptr);
		}
	}

	MarkGraphDirty();
	RefreshLightSlotList();
}

TSharedPtr<SWidget> SDialogStageManager::OnLightSlotContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddLightSlotCtx", "Add Light Slot"),
		LOCTEXT("AddLightSlotCtxTooltip", "Add Light Slot"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
			{
				OnAddLightSlot();
			}))
	);

	if (SelectedSlot.IsValid() && SelectedDialogStage.IsValid() && IsSelectedSlotLight())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DeleteLightSlotCtx", "Delete"),
			LOCTEXT("DeleteLightSlotCtxTooltip", "Delete selected light slot"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]()
				{
					OnRemoveLightSlot(Cast<UDialogSequenceSlot_Light>(SelectedSlot.Get()), SelectedSlotIndex);
				}))
		);
	}

	return MenuBuilder.MakeWidget();
}

void SDialogStageManager::OnRemoveSlot(UDialogSequenceSlot* InSlot, int32 InSlotIndex, bool bIsCameraSlot)
{
	UDialogStage* DialogStage = SelectedDialogStage.Get();
	if (!DialogStage || !InSlot)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteSlotTransaction", "Delete Slot"));
	DialogStage->Modify();
	InSlot->Modify();

	TArray<UDialogBuilderNode_DialogSequence*> SequenceNodes;
	DialogGraph->GetNodesOfType(SequenceNodes);

	for (UDialogBuilderNode_DialogSequence* SequenceNode : SequenceNodes)
	{
		if (SequenceNode && SequenceNode->DialogStageToUse == DialogStage)
		{
			UDialogSequence* DialogSequence = SequenceNode->DialogSequence;
			UDialogStage* DialogStageInstance = SequenceNode->DialogStage;
			if (DialogSequence && DialogStageInstance && DialogSequence->MovieScene)
			{
				UDialogSequenceSlot* SlotInSequence = DialogStageInstance->Slots.IsValidIndex(InSlotIndex) ? DialogStageInstance->Slots[InSlotIndex] : nullptr;
				
				if (SlotInSequence)
				{
					DialogSequence->MovieScene->RemovePossessable(SlotInSequence->ID);
				}
				
			}
		}
		
	}

	if (bIsCameraSlot)
	{
		if (DialogStage->CameraSlots.IsValidIndex(InSlotIndex))
		{
			DialogStage->CameraSlots.RemoveAt(InSlotIndex);
		}
		else
		{
			DialogStage->CameraSlots.RemoveSingle(InSlot);
		}
	}
	else
	{
		if (DialogStage->Slots.IsValidIndex(InSlotIndex))
		{
			DialogStage->Slots.RemoveAt(InSlotIndex);
		}
		else
		{
			DialogStage->Slots.RemoveSingle(InSlot);
		}
	}

	if (SelectedSlot.Get() == InSlot)
	{
		SelectedSlot.Reset();
		SelectedSlotIndex = INDEX_NONE;

		if (SlotDetailsView.IsValid())
		{
			SlotDetailsView->SetObject(nullptr);
		}
	}

	MarkGraphDirty();
	RefreshSlotList();
}

TSharedPtr<SWidget> SDialogStageManager::OnDialogStageContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);		

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddDialogStageCtx", "Add Dialog Stage"),
		LOCTEXT("AddDialogStageCtxTooltip", "Add a new Dialog Stage"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			OnAddDialogStage();
		}))
	);

	if (SelectedDialogStage.IsValid())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DeleteDialogStageCtx", "Delete"),
			LOCTEXT("DeleteDialogStageCtxTooltip", "Delete selected Dialog Stage"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]()
			{
				OnRemoveDialogStage(SelectedDialogStage.Get());
			}))
		);
	}

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SDialogStageManager::OnSlotContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddSlotCtx", "Add Slot"),
		LOCTEXT("AddSlotCtxTooltip", "Add Slot"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			OnAddSlot();
		}))
	);

	/*MenuBuilder.AddMenuEntry(
		LOCTEXT("AddCameraSlotCtx", "Add Camera Slot"),
		LOCTEXT("AddCameraSlotCtxTooltip", "Add Camera Slot"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			OnAddCameraSlot();
		}))
	);*/

	if (SelectedSlot.IsValid() && SelectedDialogStage.IsValid())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DeleteSlotCtx", "Delete"),
			LOCTEXT("DeleteSlotCtxTooltip", "Delete selected slot"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]()
			{
				OnRemoveSlot(SelectedSlot.Get(), SelectedSlotIndex, IsSelectedSlotCamera());
			}))
		);
	}

	return MenuBuilder.MakeWidget();
}

void SDialogStageManager::OnDeleteSelection()
{
	if (SelectedSlot.IsValid() && SelectedDialogStage.IsValid() && IsSelectedSlotLight())
	{
		OnRemoveLightSlot(Cast<UDialogSequenceSlot_Light>(SelectedSlot.Get()), SelectedSlotIndex);
		return;
	}
	if (SelectedSlot.IsValid() && SelectedDialogStage.IsValid())
	{
		OnRemoveSlot(SelectedSlot.Get(), SelectedSlotIndex, IsSelectedSlotCamera());
		return;
	}

	if (SelectedDialogStage.IsValid())
	{
		OnRemoveDialogStage(SelectedDialogStage.Get());
	}
}

bool SDialogStageManager::CanDeleteSelection() const
{
	return SelectedSlot.IsValid() || SelectedDialogStage.IsValid();
}

bool SDialogStageManager::IsSelectedSlotCamera() const
{
	const UDialogStage* DialogStage = SelectedDialogStage.Get();
	const UDialogSequenceSlot* Slot = SelectedSlot.Get();

	return DialogStage && Slot && DialogStage->CameraSlots.Contains(const_cast<UDialogSequenceSlot*>(Slot));
}


bool SDialogStageManager::IsSelectedSlotLight() const
{
	const UDialogStage* DialogStage = SelectedDialogStage.Get();
	const UDialogSequenceSlot* Slot = SelectedSlot.Get();
	return DialogStage && Slot && DialogStage->LightSlots.Contains(const_cast<UDialogSequenceSlot*>(Slot));
}
void SDialogStageManager::Refresh()
{
	const FName CachedDialogStageObjectName = SelectedDialogStage.IsValid() ? SelectedDialogStage->GetFName() : NAME_None;
	const FGuid CachedSlotId = SelectedSlot.IsValid() ? SelectedSlot->ID : FGuid();

	RefreshDialogStageList();
	RestoreSelection(CachedDialogStageObjectName, CachedSlotId);
	RefreshLightSlotList();
}

void SDialogStageManager::RefreshDialogStageList()
{
	DialogStageItems.Empty();

	if (UDialogBuilderGraph* Graph = DialogGraph.Get())
	{
		for (UDialogStage* DialogStage : Graph->DialogStages)
		{
			if (!DialogStage)
			{
				continue;
			}

			FDialogStageListItemPtr NewItem = MakeShared<FDialogStageListItem>();
			NewItem->DialogStage = DialogStage;
			DialogStageItems.Add(NewItem);
		}
	}

	if (DialogStageListView.IsValid())
	{
		DialogStageListView->RequestListRefresh();
	}
}

void SDialogStageManager::RefreshSlotList()
{
	SlotItems.Empty();

	UDialogStage* DialogStage = SelectedDialogStage.Get();
	if (DialogStage)
	{
		for (int32 SlotIndex = 0; SlotIndex < DialogStage->Slots.Num(); ++SlotIndex)
		{
			UDialogSequenceSlot* Slot = DialogStage->Slots[SlotIndex];
			if (!Slot) continue;

			FDialogSlotListItemPtr NewItem = MakeShared<FDialogSlotListItem>();
			NewItem->Slot = Slot;
			NewItem->bIsCameraSlot = false;
			NewItem->bIsLightSlot = false;
			NewItem->SlotIndex = SlotIndex;
			SlotItems.Add(NewItem);
		}

		for (int32 CameraSlotIndex = 0; CameraSlotIndex < DialogStage->CameraSlots.Num(); ++CameraSlotIndex)
		{
			UDialogSequenceSlot* Slot = DialogStage->CameraSlots[CameraSlotIndex];
			if (!Slot) continue;

			FDialogSlotListItemPtr NewItem = MakeShared<FDialogSlotListItem>();
			NewItem->Slot = Slot;
			NewItem->bIsCameraSlot = true;
			NewItem->bIsLightSlot = false;
			NewItem->SlotIndex = CameraSlotIndex;
			SlotItems.Add(NewItem);
		}
	}

	if (SlotListView.IsValid())
	{
		SlotListView->RequestListRefresh();
	}
}

void SDialogStageManager::RefreshLightSlotList()
{
	LightSlotItems.Empty();

	UDialogStage* DialogStage = SelectedDialogStage.Get();
	if (DialogStage)
	{
		for (int32 LightSlotIndex = 0; LightSlotIndex < DialogStage->LightSlots.Num(); ++LightSlotIndex)
		{
			UDialogSequenceSlot_Light* Slot = DialogStage->LightSlots[LightSlotIndex];
			if (!Slot) continue;

			FDialogSlotListItemPtr NewItem = MakeShared<FDialogSlotListItem>();
			NewItem->Slot = Slot;
			NewItem->bIsCameraSlot = false;
			NewItem->bIsLightSlot = true;
			NewItem->SlotIndex = LightSlotIndex;
			LightSlotItems.Add(NewItem);
		}
	}

	if (LightSlotListView.IsValid())
	{
		LightSlotListView->RequestListRefresh();
	}
}

void SDialogStageManager::MarkGraphDirty() const
{
	if (UDialogBuilderGraph* Graph = DialogGraph.Get())
	{
		Graph->Modify();
		if (UPackage* Package = Graph->GetOutermost())
		{
			Package->MarkPackageDirty();
		}
	}
}

FDialogStageListItemPtr SDialogStageManager::FindDialogStageItemByObjectName(const FName& InDialogStageObjectName) const
{
	if (InDialogStageObjectName.IsNone())
	{
		return nullptr;
	}

	for (const FDialogStageListItemPtr& Item : DialogStageItems)
	{
		if (Item.IsValid())
		{
			UDialogStage* DialogStage = Item->DialogStage.Get();
			if (DialogStage && DialogStage->GetFName() == InDialogStageObjectName)
			{
				return Item;
			}
		}
	}

	return nullptr;
}

FDialogSlotListItemPtr SDialogStageManager::FindSlotItemById(const FGuid& InSlotId) const
{
	if (!InSlotId.IsValid())
	{
		return nullptr;
	}

	for (const FDialogSlotListItemPtr& Item : SlotItems)
	{
		if (Item.IsValid())
		{
			UDialogSequenceSlot* Slot = Item->Slot.Get();
			if (Slot && Slot->ID == InSlotId)
			{
				return Item;
			}
		}
	}

	return nullptr;
}

bool SDialogStageManager::IsDialogStageBlueprint(const FAssetData& AssetData) const
{
	FString ParentClassPath;

	if (!AssetData.GetTagValue(FName(TEXT("NativeParentClassPath")), ParentClassPath) &&
		!AssetData.GetTagValue(FName(TEXT("ParentClass")), ParentClassPath))
	{
		return false;
	}

	return HasClassPathToken(ParentClassPath, UDialogStage::StaticClass());
}

bool SDialogStageManager::HasClassPathToken(const FString& InTagValue, const UClass* InClass)
{
	if (!InClass)
	{
		return false;
	}

	const FString ClassName = InClass->GetName();
	const FString ClassPath = InClass->GetPathName();

	return InTagValue.Contains(ClassName) || InTagValue.Contains(ClassPath);
}


void SDialogStageManager::SelectDialogStageByObjectName(const FName& InDialogStageObjectName)
{
	if (!DialogStageListView.IsValid())
	{
		return;
	}

	if (FDialogStageListItemPtr FoundItem = FindDialogStageItemByObjectName(InDialogStageObjectName))
	{
		DialogStageListView->SetSelection(FoundItem, ESelectInfo::Direct);
		DialogStageListView->RequestScrollIntoView(FoundItem);
	}
}

void SDialogStageManager::SelectSlotById(const FGuid& InSlotId)
{
	if (!SlotListView.IsValid())
	{
		return;
	}

	if (FDialogSlotListItemPtr FoundItem = FindSlotItemById(InSlotId))
	{
		SlotListView->SetSelection(FoundItem, ESelectInfo::Direct);
		SlotListView->RequestScrollIntoView(FoundItem);
	}
}

void SDialogStageManager::RestoreSelection(const FName& InDialogStageObjectName, const FGuid& InSlotId)
{
	SelectDialogStageByObjectName(InDialogStageObjectName);
	RefreshSlotList();
	SelectSlotById(InSlotId);
}

TSharedRef<SWidget> SDialogStageManager::BuildImportDialogStageMenu()
{
	RefreshDialogStageAssetList();

	if (DialogStageAssetItems.Num() == 0)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(6.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoDialogStageAssets", "No dialog stage assets found."))
			];
	}

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogStageAssetListView, SListView<FDialogStageAssetItemPtr>)
			.ListItemsSource(&DialogStageAssetItems)
			.OnGenerateRow(this, &SDialogStageManager::GenerateDialogStageAssetRow)
			.OnSelectionChanged(this, &SDialogStageManager::OnDialogStageAssetSelected)
			.SelectionMode(ESelectionMode::Single)
		];
}

void SDialogStageManager::RefreshDialogStageAssetList()
{
	DialogStageAssetItems.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& AssetData : Assets)
	{
		if (!IsDialogStageBlueprint(AssetData))
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(UDialogStage::StaticClass()))
		{
			continue;
		}

		FDialogStageAssetItemPtr Item = MakeShared<FDialogStageAssetItem>();
		Item->AssetData = AssetData;
		Item->DisplayName = !Blueprint->BlueprintDisplayName.IsEmpty()
			? FText::FromString(Blueprint->BlueprintDisplayName)
			: FText::FromName(AssetData.AssetName);

		DialogStageAssetItems.Add(Item);
	}

	DialogStageAssetItems.Sort([](const FDialogStageAssetItemPtr& A, const FDialogStageAssetItemPtr& B)
		{
			const FString AName = A.IsValid() ? A->DisplayName.ToString() : FString();
			const FString BName = B.IsValid() ? B->DisplayName.ToString() : FString();
			return AName < BName;
		});
}


TSharedRef<ITableRow> SDialogStageManager::GenerateDialogStageAssetRow(
	FDialogStageAssetItemPtr InItem,
	const TSharedRef<STableViewBase>& InOwnerTable)
{
	return SNew(STableRow<FDialogStageAssetItemPtr>, InOwnerTable)
		[
			SNew(STextBlock)
				.Text(InItem.IsValid() ? InItem->DisplayName : LOCTEXT("InvalidDialogStageAsset", "Invalid"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.0f))
		];
}

void SDialogStageManager::OnDialogStageAssetSelected(FDialogStageAssetItemPtr InItem, ESelectInfo::Type InSelectInfo)
{
	if (!InItem.IsValid())
	{
		return;
	}

	ImportDialogStageAsset(InItem->AssetData);

	if (ImportDialogStageComboButton.IsValid())
	{
		ImportDialogStageComboButton->SetIsOpen(false, false);
	}
}

void SDialogStageManager::ImportDialogStageAsset(const FAssetData& AssetData)
{
	UDialogBuilderGraph* Graph = DialogGraph.Get();
	if (!Graph)
	{
		return;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(UDialogStage::StaticClass()))
	{
		return;
	}

	UDialogStage* SourceStage = Cast<UDialogStage>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!SourceStage)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ImportDialogStageTransaction", "Import Dialog Stage"));
	Graph->Modify();

	UDialogStage* NewDialogStage = NewObject<UDialogStage>(Graph, NAME_None, RF_Transactional);
	if (!NewDialogStage)
	{
		return;
	}

	UEngine::CopyPropertiesForUnrelatedObjects(SourceStage, NewDialogStage);
	
	NewDialogStage->Modify();
	NewDialogStage->OwningDialogGraph = Graph;
	NewDialogStage->bIsTemplate = false;

	ResetDialogStageSlotDefinitions(NewDialogStage);

	Graph->DialogStages.Add(NewDialogStage);
	NewDialogStage->MakeUniqueDialogStageName();

	MarkGraphDirty();
	RefreshDialogStageList();
}

void SDialogStageManager::ResetDialogStageSlotDefinitions(UDialogStage* InStage)
{
	if (!InStage)
	{
		return;
	}

	UDialogBuilderGraph* Graph = DialogGraph.Get();

	auto ResetSlot = [Graph](UDialogSequenceSlot* Slot)
	{
		if (!Slot)
		{
			return;
		}

		Slot->Modify();
		Slot->DialogDefinition = nullptr;
		Slot->OwningDialogGraph = Graph;
	};

	for (UDialogSequenceSlot* Slot : InStage->Slots)
	{
		ResetSlot(Slot);
	}

	for (UDialogSequenceSlot* Slot : InStage->CameraSlots)
	{
		ResetSlot(Slot);
	}

	for (UDialogSequenceSlot_Light* Slot : InStage->LightSlots)
	{
		ResetSlot(Slot);
	}
}

#undef LOCTEXT_NAMESPACE