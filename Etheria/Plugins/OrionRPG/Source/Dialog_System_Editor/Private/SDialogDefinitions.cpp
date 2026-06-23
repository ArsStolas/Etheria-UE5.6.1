// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "SDialogDefinitions.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "SGraphActionMenu.h"
#include "GraphActionNode.h"
#include "SGraphPalette.h"
#include "DialogBuilderEditorUtils.h"
#include "DialogBuilder_EditorCommands.h"
#include "Widgets/Input/SSearchBox.h"
#include "DialogBuilderEditor.h"
#include "DialogBuilder_EditorCommands.h"
#include "SlateOptMacros.h"
#include "SDialogPalette.h"


#define LOCTEXT_NAMESPACE "DialogDefinitions"

void FDialogDefinitionsCommands::RegisterCommands()
{
	UI_COMMAND(OpenGraph, "Open Graph", "Opens up this function, macro, or event graph's graph panel up.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenGraphInNewTab, "Open in New Tab", "Opens up this function, macro, or event graph's graph panel up in a new tab. Hold down Ctrl and double click for shortcut.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenExternalGraph, "Open External Graph", "Opens up this external graph's graph panel in its own asset editor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(FocusNode, "Focus", "Focuses on the associated node", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(FocusNodeInNewTab, "Focus in New Tab", "Focuses on the associated node in a new tab", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DeleteEntry, "Delete", "Deletes this function or variable from this Dialog Editor.", EUserInterfaceActionType::Button, FInputChord(EKeys::Platform_Delete));
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDialogDefinitions::Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor, const UDialogBuilderGraph* InDialogSystemGraph)
{
	bNeedsRefresh = false;
	bShowReplicatedVariablesOnly = false;

	DialogEditorPtr = InDialogEditor;
	EdGraph = nullptr;

	TSharedPtr<SWidget> ToolbarBuilderWidget = TSharedPtr<SWidget>();

	if (InDialogEditor.IsValid())
	{
		DialogBuilderGraph = DialogEditorPtr.Pin()->GetDialogBuilderGraph();

		CommandList = MakeShareable(new FUICommandList);

		CommandList->Append(InDialogEditor.Pin()->GetToolkitCommands());

		CommandList->MapAction(FDialogDefinitionsCommands::Get().OpenGraph,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnOpenGraph),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanOpenGraph));

		CommandList->MapAction(FDialogDefinitionsCommands::Get().OpenGraphInNewTab,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnOpenGraphInNewTab),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanOpenGraph));

		CommandList->MapAction(FDialogDefinitionsCommands::Get().OpenExternalGraph,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnOpenExternalGraph),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanOpenExternalGraph));

		CommandList->MapAction(FDialogDefinitionsCommands::Get().FocusNode,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnFocusNode),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanFocusOnNode));

		CommandList->MapAction(FDialogDefinitionsCommands::Get().FocusNodeInNewTab,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnFocusNodeInNewTab),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanFocusOnNode));


		CommandList->MapAction(FDialogDefinitionsCommands::Get().DeleteEntry,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnDeleteEntry),
			FCanExecuteAction::CreateSP(this, &SDialogDefinitions::CanDeleteEntry));

		CommandList->MapAction(FGenericCommands::Get().Duplicate,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnDuplicateAction),
			FCanExecuteAction::CreateSP(this, &SDialogDefinitions::CanDuplicateAction),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::IsDuplicateActionVisible));

		ToolbarBuilderWidget = SNullWidget::NullWidget;

		CommandList->MapAction(FGenericCommands::Get().Rename,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnRequestRenameOnActionNode),
			FCanExecuteAction::CreateSP(this, &SDialogDefinitions::CanRequestRenameOnActionNode));

		CommandList->MapAction(FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnCopy),
			FCanExecuteAction::CreateSP(this, &SDialogDefinitions::CanCopy));

		CommandList->MapAction(FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnCut),
			FCanExecuteAction::CreateSP(this, &SDialogDefinitions::CanCut));

		CommandList->MapAction(FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &SDialogDefinitions::OnPasteGeneric),
			FCanExecuteAction(), FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &SDialogDefinitions::CanPasteGeneric));

	}
	else
	{
		// we're in read only mode when there's no dialog editor:
		DialogBuilderGraph = const_cast<UDialogBuilderGraph*>(InDialogSystemGraph);
		check(DialogBuilderGraph);
		ToolbarBuilderWidget = SNew(SBox);
	}


	SAssignNew(FilterBox, SSearchBox)
		.OnTextChanged(this, &SDialogDefinitions::OnFilterTextChanged);

	// create the main action list piece of this widget
	SAssignNew(GraphActionMenu, SGraphActionMenu, false)
		.OnGetFilterText(this, &SDialogDefinitions::GetFilterText)
		.OnCreateWidgetForAction(this, &SDialogDefinitions::OnCreateWidgetForAction)
		.OnCollectAllActions(this, &SDialogDefinitions::CollectAllActions)
		.OnCollectStaticSections(this, &SDialogDefinitions::CollectStaticSections)
		.OnActionDragged(this, &SDialogDefinitions::OnActionDragged)
		.OnCategoryDragged(this, &SDialogDefinitions::OnCategoryDragged)
		.OnActionSelected(this, &SDialogDefinitions::OnGlobalActionSelected)
		.OnActionDoubleClicked(this, &SDialogDefinitions::OnActionDoubleClicked)
		.OnContextMenuOpening(this, &SDialogDefinitions::OnContextMenuOpening)
		.OnCategoryTextCommitted(this, &SDialogDefinitions::OnCategoryNameCommitted)
		.OnCanRenameSelectedAction(this, &SDialogDefinitions::CanRequestRenameOnActionNode)
		.OnGetSectionTitle(this, &SDialogDefinitions::OnGetSectionTitle)
		.OnGetSectionWidget(this, &SDialogDefinitions::OnGetSectionWidget)
		.OnActionMatchesName(this, &SDialogDefinitions::HandleActionMatchesName)
		.AlphaSortItems(false)
		.UseSectionStyling(true);

	ChildSlot
	[
		SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
					.Padding(4.0f)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("DialogDefinitionsPanel")))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							ToolbarBuilderWidget.ToSharedRef()
						]
					]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				GraphActionMenu.ToSharedRef()
			]
	];

	//ResetLastPinType();

	if (!DialogEditorPtr.IsValid())
	{
		Refresh();
	}

	TMap<int32, bool> ExpandedSections;
	ExpandedSections.Add(DialogSectionID::PARTICIPANTS, true);
	ExpandedSections.Add(DialogSectionID::PROPS, true);

	GraphActionMenu->SetSectionExpansion(ExpandedSections);

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &SDialogDefinitions::OnObjectPropertyChanged);

}

SDialogDefinitions::~SDialogDefinitions()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}
void SDialogDefinitions::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bNeedsRefresh)
	{
		Refresh();
	}
}

FReply SDialogDefinitions::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() && CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

template<class SchemaActionType> SchemaActionType* SelectionAsType(const TSharedPtr< SGraphActionMenu >& GraphActionMenu)
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	SchemaActionType* Selection = NULL;

	TSharedPtr<FEdGraphSchemaAction> SelectedAction(SelectedActions.Num() > 0 ? SelectedActions[0] : NULL);
	if (SelectedAction.IsValid() &&
		SelectedAction->GetTypeId() == SchemaActionType::StaticGetTypeId())
	{
		// TODO Why not? StaticCastSharedPtr<>()

		Selection = (SchemaActionType*)SelectedActions[0].Get();
	}

	return Selection;
}

FAssetSchemaAction_Participant* SDialogDefinitions::SelectionAsParticipant() const
{
	return SelectionAsType<FAssetSchemaAction_Participant>(GraphActionMenu);
}
FAssetSchemaAction_Prop* SDialogDefinitions::SelectionAsProp() const
{
	return SelectionAsType<FAssetSchemaAction_Prop>(GraphActionMenu);
}


void SDialogDefinitions::OnRequestRenameOnActionNode()
{
	// Attempt to rename in both menus, only one of them will have anything selected
	GraphActionMenu->OnRequestRenameOnActionNode();
	if (DialogEditorPtr.IsValid())
	{

	}
}
void SDialogDefinitions::OnPasteGeneric()
{
}
bool SDialogDefinitions::CanPasteGeneric()
{
	return false;
}
void SDialogDefinitions::Refresh()
{
	bNeedsRefresh = false;

	// Conform to our interfaces here to ensure we catch any newly added functions
	//FBlueprintEditorUtils::ConformImplementedInterfaces(GetDialogBuilderGraph());

	GraphActionMenu->RefreshAllActions(/*bPreserveExpansion=*/ true);
}

bool SDialogDefinitions::SelectionIsCategory() const
{
	return !SelectionHasContextMenu();
}

void SDialogDefinitions::SelectItemByName(const FName& ItemName, ESelectInfo::Type SelectInfo, int32 SectionId, bool bIsCategory)
{
	// Check if the graph action menu is being told to clear
	if (ItemName == NAME_None)
	{
		ClearGraphActionMenuSelection();
	}
	else
	{
		// Attempt to select the item in the main graph action menu
		const bool bSucceededAtSelecting = GraphActionMenu->SelectItemByName(ItemName, SelectInfo, SectionId, bIsCategory);
		if (!bSucceededAtSelecting)
		{
			// We failed to select the item, maybe because it was filtered out?
			// Reset the item filter and try again (we don't do this first because someone went to the effort of typing
			// a filter and probably wants to keep it unless it is getting in the way, as it just has)
			//OnResetItemFilter();
			GraphActionMenu->SelectItemByName(ItemName, SelectInfo, SectionId, bIsCategory);
		}
	}
}

void SDialogDefinitions::ClearGraphActionMenuSelection()
{
	GraphActionMenu->SelectItemByName(NAME_None);
}


TSharedRef<SWidget> SDialogDefinitions::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return DialogEditorPtr.IsValid() ? SNew(SDialogPaletteItem, InCreateData, DialogEditorPtr.Pin()) : SNew(SDialogPaletteItem, InCreateData, GetDialogBuilderGraph());
}

void SDialogDefinitions::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	UDialogBuilderGraph* DialogSystemObj = GetDialogBuilderGraph();
	check(DialogSystemObj);

	TSharedPtr<FDialogBuilderEditor> DialogSystemEditor = DialogEditorPtr.Pin();

	if (DialogSystemEditor.IsValid())
	{
		// Participants
		for (int32 i = 0; i < DialogSystemObj->ParticipantDefinitions.Num(); i++)
		{
			UDialogParticipant* DialogParticipant = DialogSystemObj->ParticipantDefinitions[i];
			if (!DialogParticipant) continue;

			TSharedPtr<FAssetSchemaAction_Participant> NewAction =
				MakeShareable(new FAssetSchemaAction_Participant(FText::GetEmpty(), FText::GetEmpty(), FText::GetEmpty(), 2, DialogSectionID::PARTICIPANTS));
			NewAction->FuncName = DialogParticipant->GetFName();
			NewAction->Participant = DialogParticipant;
			OutAllActions.AddAction(NewAction);
		}

		// Props
		for (int32 i = 0; i < DialogSystemObj->PropDefinitions.Num(); i++)
		{
			UDialogProp* DialogProp = DialogSystemObj->PropDefinitions[i];
			if (!DialogProp) continue;

			TSharedPtr<FAssetSchemaAction_Prop> NewAction =
				MakeShareable(new FAssetSchemaAction_Prop(FText::GetEmpty(), FText::GetEmpty(), FText::GetEmpty(), 2, DialogSectionID::PROPS));
			NewAction->FuncName = DialogProp->GetFName();
			NewAction->Prop = DialogProp;
			OutAllActions.AddAction(NewAction);
		}

		
	}
}
void SDialogDefinitions::CollectStaticSections(TArray<int32>& StaticSectionIDs)
{
	StaticSectionIDs.Add(DialogSectionID::PARTICIPANTS);
	StaticSectionIDs.Add(DialogSectionID::PROPS);
}
FReply SDialogDefinitions::OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}
FReply SDialogDefinitions::OnCategoryDragged(const FText& InCategory, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}
void SDialogDefinitions::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();
	TSharedPtr<FEdGraphSchemaAction> InAction(InActions.Num() > 0 ? InActions[0] : nullptr);
	if (!InAction.IsValid())
	{
		return;
	}

	if (FAssetSchemaAction_Participant* ParticipantAction = static_cast<FAssetSchemaAction_Participant*>(InAction.Get()))
	{
		if (DialogEditor && ParticipantAction->Participant)
		{
			DialogEditor->SetDetailsObject(ParticipantAction->Participant);
			DialogEditor->SelectDialogDefinitionActor(ParticipantAction->Participant);
		}
	}

	if (FAssetSchemaAction_Prop* PropAction = static_cast<FAssetSchemaAction_Prop*>(InAction.Get()))
	{
		if (DialogEditor && PropAction->Prop)
		{
			DialogEditor->SetDetailsObject(PropAction->Prop);
			DialogEditor->SelectDialogDefinitionActor(PropAction->Prop);
		}
	}

	if (FAssetSchemaAction_Light* LightAction = static_cast<FAssetSchemaAction_Light*>(InAction.Get()))
	{
		if (DialogEditor && LightAction->Light)
		{
			DialogEditor->SetDetailsObject(LightAction->Light);
			DialogEditor->SelectDialogDefinitionActor(LightAction->Light);
		}
	}
}

void SDialogDefinitions::OnGlobalActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || InSelectionType == ESelectInfo::OnNavigation || InActions.Num() == 0)
	{
		OnActionSelected(InActions);
	}
}

void SDialogDefinitions::OnActionDoubleClicked(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions)
{
	if (!DialogEditorPtr.IsValid())
	{
		return;
	}

	TSharedPtr<FEdGraphSchemaAction> InAction(InActions.Num() > 0 ? InActions[0] : NULL);
	ExecuteAction(InAction);

}

void SDialogDefinitions::ExecuteAction(TSharedPtr<FEdGraphSchemaAction> InAction)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin();
	
	if (InAction.IsValid())
	{
		//Participant
		FAssetSchemaAction_Participant* ParticipantAction = (FAssetSchemaAction_Participant*)InAction.Get();

		UDialogParticipant* DialogParticipant = ParticipantAction->Participant;
		if (DialogEditor && DialogParticipant)
		{
			DialogEditor->SetDetailsObject(DialogParticipant);
		}

		//Prop
		FAssetSchemaAction_Prop* PropAction = (FAssetSchemaAction_Prop*)InAction.Get();

		UDialogProp* DialogProp = PropAction->Prop;
		if (DialogEditor && DialogProp)
		{
			DialogEditor->SetDetailsObject(DialogProp);
		}

	}
}

TSharedPtr<SWidget> SDialogDefinitions::OnContextMenuOpening()
{
	if (!DialogEditorPtr.IsValid())
	{
		return TSharedPtr<SWidget>();
	}

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	// Check if the selected action is valid for a context menu
	if (SelectionHasContextMenu())
	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			/*MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().OpenGraph);
			MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().OpenGraphInNewTab);
			MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().OpenExternalGraph);
			MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().FocusNode);
			MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().FocusNodeInNewTab);*/
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, LOCTEXT("Rename", "Rename"), LOCTEXT("Rename_Tooltip", "Rename Participant"));
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FDialogDefinitionsCommands::Get().DeleteEntry);
		}
		MenuBuilder.EndSection();

		FAssetSchemaAction_Participant* Graph = SelectionAsParticipant();

		//if (Var && BlueprintEditorPtr.IsValid() && FBlueprintEditorUtils::DoesSupportEventGraphs(GetBlueprintObj()))
		//{
		//	FObjectProperty* ComponentProperty = CastField<FObjectProperty>(Var->GetProperty());

		//	if (ComponentProperty && ComponentProperty->PropertyClass &&
		//		ComponentProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
		//	{
		//		if (FBlueprintEditorUtils::CanClassGenerateEvents(ComponentProperty->PropertyClass))
		//		{
		//			TSharedPtr<FBlueprintEditor> BlueprintEditor(BlueprintEditorPtr.Pin());

		//			// If the selected item is valid, and is a component of some sort, build a context menu
		//			// of events appropriate to the component.
		//			MenuBuilder.AddSubMenu(LOCTEXT("AddEventSubMenu", "Add Event"),
		//				LOCTEXT("AddEventSubMenu_ToolTip", "Add Event"),
		//				FNewMenuDelegate::CreateStatic(&SSubobjectBlueprintEditor::BuildMenuEventsSection,
		//					BlueprintEditor->GetBlueprintObj(), ComponentProperty->PropertyClass,
		//					FCanExecuteAction::CreateRaw(this, &SMyBlueprint::IsEditingMode),
		//					FGetSelectedObjectsDelegate::CreateSP(this, &SMyBlueprint::GetSelectedItemsForContextMenu)));
		//		}
		//	}
		//}
		
	}
	else
	{
		BuildAddNewMenu(MenuBuilder);
	}

	return MenuBuilder.MakeWidget();
}
void SDialogDefinitions::BuildAddNewMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("AddNew", LOCTEXT("AddOperations", "Add New"));

	if (UDialogBuilderGraph* CurrentBlueprint = GetDialogBuilderGraph())
	{
		MenuBuilder.AddMenuEntry(FDialogBuilder_EditorCommands::Get().AddNewDialogGraph);
		
	}
	MenuBuilder.EndSection();
}
TSharedRef<SWidget> SDialogDefinitions::CreateAddToSectionButton(int32 InSectionID, TWeakPtr<SWidget> WeakRowWidget, FText AddNewText, FName MetaDataTag)
{
	return
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.OnClicked(this, &SDialogDefinitions::OnAddButtonClickedOnSection, InSectionID)
		.IsEnabled(this, &SDialogDefinitions::CanAddNewElementToSection, InSectionID)
		.ContentPadding(FMargin(1, 0))
		.AddMetaData<FTagMetaData>(FTagMetaData(MetaDataTag))
		.ToolTipText(AddNewText)
		[
			SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
				.ColorAndOpacity(FSlateColor::UseForeground())
		];
}
void SDialogDefinitions::OnCategoryNameCommitted(const FText& InNewText, ETextCommit::Type InTextCommit, TWeakPtr<struct FGraphActionNode> InAction)
{
	// Remove excess whitespace and prevent categories with just spaces
	FText CategoryName = FText::TrimPrecedingAndTrailing(InNewText);

	TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
	GraphActionMenu->GetCategorySubActions(InAction, Actions);

	if (Actions.Num())
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameCategory", "Rename Category"));

		GetDialogBuilderGraph()->Modify();

		Refresh();
		SelectItemByName(FName(*CategoryName.ToString()), ESelectInfo::OnMouseClick, InAction.Pin()->SectionID, true);
	}
}
bool SDialogDefinitions::CanRequestRenameOnActionNode(TWeakPtr<struct FGraphActionNode> InSelectedNode) const
{
	return false;
}

FText SDialogDefinitions::OnGetSectionTitle(int32 InSectionID)
{
	FText SeperatorTitle;
	switch (InSectionID)
	{
	case DialogSectionID::PARTICIPANTS:
		SeperatorTitle = NSLOCTEXT("GraphActionNode", "Participants", "Participants");
		break;
	case DialogSectionID::PROPS:
		SeperatorTitle = NSLOCTEXT("GraphActionNode", "Props", "Props");
		break;

	case DialogSectionID::NONE:
		SeperatorTitle = FText::GetEmpty();
		break;
	default:
		break;
	}

	return SeperatorTitle;
}
TSharedRef<SWidget> SDialogDefinitions::OnGetSectionWidget(TSharedRef<SWidget> RowWidget, int32 InSectionID)
{
	TWeakPtr<SWidget> WeakRowWidget = RowWidget;

	FText AddNewText;
	FName MetaDataTag;

	switch (InSectionID)
	{
	case DialogSectionID::PARTICIPANTS:
		AddNewText = LOCTEXT("AddNewParticipant", "New Dialog Participant");
		MetaDataTag = TEXT("AddNewParticipant");
		break;
	case DialogSectionID::PROPS:
		AddNewText = LOCTEXT("AddNewProps", "New Props");
		MetaDataTag = TEXT("AddNewProps");
		break;
	default:
		return SNullWidget::NullWidget;
	}

	return CreateAddToSectionButton(InSectionID, WeakRowWidget, AddNewText, MetaDataTag);
}
FReply SDialogDefinitions::OnAddButtonClickedOnSection(int32 InSectionID)
{
	switch (InSectionID)
	{
	case DialogSectionID::PARTICIPANTS:
		CreateNewParticipant();
		break;
	case DialogSectionID::PROPS:
		CreateNewProp();
		break;
	}
	return FReply::Handled();
}
bool SDialogDefinitions::CanAddNewElementToSection(int32 InSectionID) const
{
	return true;
}
bool SDialogDefinitions::HandleActionMatchesName(FEdGraphSchemaAction* InAction, const FName& InName) const
{
	if (!InAction || InName.IsNone())
	{
		return false;
	}

	if (InAction->GetTypeId() == FAssetSchemaAction_Participant::StaticGetTypeId())
	{
		FAssetSchemaAction_Participant* ParticipantAction = static_cast<FAssetSchemaAction_Participant*>(InAction);
		if (ParticipantAction->FuncName == InName)
		{
			return true;
		}

		if (ParticipantAction->Participant && ParticipantAction->Participant->GetFName() == InName)
		{
			return true;
		}
	}
	else if (InAction->GetTypeId() == FAssetSchemaAction_Prop::StaticGetTypeId())
	{
		FAssetSchemaAction_Prop* PropAction = static_cast<FAssetSchemaAction_Prop*>(InAction);
		if (PropAction->FuncName == InName)
		{
			return true;
		}

		if (PropAction->Prop && PropAction->Prop->GetFName() == InName)
		{
			return true;
		}
	}

	// Optional fallback
	return InAction->GetMenuDescription().ToString() == InName.ToString();

}
void SDialogDefinitions::OnOpenGraph()
{
	OpenGraph(FDocumentTracker::OpenNewDocument);
}
void SDialogDefinitions::OnOpenGraphInNewTab()
{
	OpenGraph(FDocumentTracker::ForceOpenNewDocument);
}
void SDialogDefinitions::OnOpenExternalGraph()
{
}
bool SDialogDefinitions::CanOpenGraph() const
{
	return true;
}
bool SDialogDefinitions::CanOpenExternalGraph() const
{
	return false;
}
bool SDialogDefinitions::CanFocusOnNode() const
{
	return false;
}
void SDialogDefinitions::OnFocusNode()
{
}
void SDialogDefinitions::OnFocusNodeInNewTab()
{
}
void SDialogDefinitions::OnDeleteEntry()
{
	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	check(DialogGraph);

	const FScopedTransaction Transaction(LOCTEXT("RemoveDefinition", "Remove Definition"));
	DialogGraph->Modify();

	if (FAssetSchemaAction_Participant* ParticipantAction = SelectionAsParticipant())
	{
		if (DialogEditorPtr.IsValid())
		{
			DialogEditorPtr.Pin()->OnDialogDefinitionRemoved(ParticipantAction->Participant);
		}
		DialogGraph->ParticipantDefinitions.Remove(ParticipantAction->Participant);
	}

	if (FAssetSchemaAction_Prop* PropAction = SelectionAsProp())
	{
		DialogGraph->PropDefinitions.Remove(PropAction->Prop);
	}

	Refresh();

}

bool SDialogDefinitions::CanDeleteEntry() const
{
	if (const FAssetSchemaAction_Participant* ParticipantAction = SelectionAsParticipant())
	{
		if (const UDialogParticipant* DialogParticipant = ParticipantAction->Participant)
		{
			if (DialogParticipant->IsA(UDialogPlayerParticipant::StaticClass()))
			{
				return false;
			}
		}
	}

	return true;
}

bool SDialogDefinitions::CanRequestRenameOnActionNode() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	// If there is anything selected in the GraphActionMenu, check the item for if it can be renamed.
	if (SelectedActions.Num() || SelectionIsCategory())
	{
		return GraphActionMenu->CanRequestRenameOnActionNode();
	}
	return false;
}

bool SDialogDefinitions::IsDuplicateActionVisible() const
{
	return false;
}
bool SDialogDefinitions::CanDuplicateAction() const
{
	return false;
}
void SDialogDefinitions::OnDuplicateAction()
{
}



void SDialogDefinitions::OnCopy()
{
	
}
bool SDialogDefinitions::CanCopy() const
{
	return false;
}
void SDialogDefinitions::OnCut()
{
}
bool SDialogDefinitions::CanCut() const
{
	return false;
}

void SDialogDefinitions::CreateNewParticipant()
{
	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	if (!DialogGraph)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddParticipant", "Add Participant"));
	DialogGraph->Modify();

	UDialogParticipant* NewParticipant = NewObject<UDialogParticipant>(DialogGraph, NAME_None, RF_Transactional);
	NewParticipant->DisplayName = FText::FromString(FString::Printf(TEXT("Participant %d"), DialogGraph->ParticipantDefinitions.Num()));
	DialogGraph->ParticipantDefinitions.Add(NewParticipant);

	if (DialogEditorPtr.IsValid())
	{
		DialogEditorPtr.Pin()->OnDialogDefinitionAdded(NewParticipant);
	}

	Refresh();
}

void SDialogDefinitions::CreateNewProp()
{
	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	if (DialogGraph)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddParticipant", "Add Participant"));
		UDialogProp* NewProp = NewObject<UDialogProp>(DialogGraph, NAME_None, RF_Transactional);
		NewProp->DisplayName = FText::FromString(FString::Printf(TEXT("Prop %d"), DialogGraph->PropDefinitions.Num()));
		DialogGraph->PropDefinitions.Add(NewProp);

		DialogGraph->Modify();

		Refresh();
	}
}

void SDialogDefinitions::OnDeleteGraph(UEdGraph* InGraph)
{
	if (InGraph && InGraph->bAllowDeletion)
	{
		if (const UEdGraphSchema* Schema = InGraph->GetSchema())
		{
			if (Schema->TryDeleteGraph(InGraph))
			{
				return;
			}
		}

		const FScopedTransaction Transaction(LOCTEXT("RemoveGraph", "Remove Graph"));
		GetDialogBuilderGraph()->Modify();

		InGraph->Modify();

		
		// Remove any  nodes bound to this graph
		TArray<UDialogBuilderEdNode*> AllNodes;
		FDialogBuilderEditorUtils::GetAllNodesOfClass<UDialogBuilderEdNode>(GetDialogBuilderGraph(), AllNodes);

		const bool bDontRecompile = true;
		for (UDialogBuilderEdNode* CompNode : AllNodes)
		{
			if (CompNode->GetDialogBuilderEdGraph() == InGraph)
			{
				FDialogBuilderEditorUtils::RemoveNode(GetDialogBuilderGraph(), CompNode, bDontRecompile);
			}
		}
		

		FDialogBuilderEditorUtils::RemoveGraph(GetDialogBuilderGraph(), InGraph);
		DialogEditorPtr.Pin()->CloseDocumentTab(InGraph);


		InGraph = NULL;
	}
}

void SDialogDefinitions::OpenGraph(FDocumentTracker::EOpenDocumentCause InCause, bool bOpenExternalGraphInNewEditor)
{
	UEdGraph* GraphToOpen = nullptr;

	if (GraphToOpen)
	{
		if (bOpenExternalGraphInNewEditor )
		{
			DialogEditorPtr.Pin()->JumpToHyperlink(GraphToOpen, false);
		}
		else
		{
			DialogEditorPtr.Pin()->OpenDocument(GraphToOpen, InCause);
		}
	}
}

bool SDialogDefinitions::SelectionHasContextMenu() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);
	return SelectedActions.Num() > 0;
}

void SDialogDefinitions::OnObjectPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent)
{
	if (InObject == DialogBuilderGraph && (InPropertyChangedEvent.ChangeType != EPropertyChangeType::ValueSet && InPropertyChangedEvent.ChangeType != EPropertyChangeType::ArrayClear))
	{
		bNeedsRefresh = true;
	}
}

void SDialogDefinitions::OnFilterTextChanged(const FText& InFilterText)
{
}
FText SDialogDefinitions::GetFilterText() const
{
	return FText();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION



#undef LOCTEXT_NAMESPACE