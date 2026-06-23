// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "SDialogPalette.h"
#include "DialogBuilder_EditorStyle.h"
#include "SlateOptMacros.h"	
#include "TutorialMetaData.h"
#include "SDialogDefinitions.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "EdGraphSchema_DialogBuilder.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode.h"
#include "Decorator/OrionDecorator.h"
#include "DialogBuilderFunctionLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Event/OrionEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "DialogDefinition.h"
#include "ClassIconFinder.h"
#include "Engine/Light.h"


#define LOCTEXT_NAMESPACE "DialogPaletteItem"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDialogPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, TWeakPtr<FDialogBuilderEditor> InDialogEditor)
{
	Construct(InArgs, InCreateData, InDialogEditor.Pin()->GetDialogBuilderGraph(), InDialogEditor);

}

void SDialogPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, UDialogBuilderGraph* InDialogSystemGraph)
{
	Construct(InArgs, InCreateData, InDialogSystemGraph, TWeakPtr<FDialogBuilderEditor>());
}

void SDialogPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, UDialogBuilderGraph* InDialogSystemGraph, TWeakPtr<FDialogBuilderEditor> InDialogEditor)
{
	check(InCreateData->Action.IsValid());
	check(InDialogSystemGraph);

	DialogBuilderGraph = InDialogSystemGraph;

	bShowClassInTooltip = InArgs._ShowClassInTooltip;

	TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;
	DialogEditorPtr = InDialogEditor;

	// construct the icon widget
	FSlateBrush const* IconBrush = GetPaletteIconBrushForAction(GraphAction);
	FSlateBrush const* SecondaryBrush = FAppStyle::GetBrush(TEXT("NoBrush"));
	FSlateColor        IconColor = FSlateColor::UseForeground();
	FSlateColor        SecondaryIconColor = FSlateColor::UseForeground();
	FText			   IconToolTip = GraphAction->GetTooltipDescription();
	FString			   IconDocLink, IconDocExcerpt;
	TSharedRef<SWidget> IconWidget = CreateIconWidget(IconToolTip, IconBrush, IconColor);
	
	// Setup a meta tag for this node
	FTutorialMetaData TagMeta("PaletteItem");
	if (ActionPtr.IsValid())
	{
		TagMeta.Tag = *FString::Printf(TEXT("PaletteItem,%s,%d"), *GraphAction->GetMenuDescription().ToString(), GraphAction->GetSectionID());
		TagMeta.FriendlyName = GraphAction->GetMenuDescription().ToString();
	}

	// construct the text widget
	bool bIsReadOnly = false;
	TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(InCreateData, bIsReadOnly);

	// Create the widget with an icon
	TSharedRef<SHorizontalBox> ActionBox = SNew(SHorizontalBox)
		.AddMetaData<FTutorialMetaData>(TagMeta);


	ActionBox.Get().AddSlot()
		.AutoWidth()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
				.WidthOverride(22.0f)
				.HeightOverride(22.0f)
				[
					IconWidget
				]
		];


	ActionBox.Get().AddSlot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(/* horizontal */ 1.5f, /* vertical */ 1.0f)
		[
			NameSlotWidget
		];

	// Now, create the actual widget
	ChildSlot
		[
			ActionBox
		];

}



END_SLATE_FUNCTION_BUILD_OPTIMIZATION


const FSlateBrush* SDialogPaletteItem::GetPaletteIconBrushForAction(const TSharedPtr<FEdGraphSchemaAction>& Action)
{
	if (!Action.IsValid())
	{
		return FAppStyle::GetBrush(TEXT("NoBrush"));
	}

	UClass* IconClass = nullptr;
	switch (Action->GetSectionID())
	{
	case DialogSectionID::PARTICIPANTS:
	{
		return FClassIconFinder::FindThumbnailForClass(ACharacter::StaticClass());
		//return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.Dialog.Participant");
	}

	case DialogSectionID::PROPS:
	{

		return FClassIconFinder::FindThumbnailForClass(UObject::StaticClass());
		/*const FAssetSchemaAction_Prop* PropAction = static_cast<const FAssetSchemaAction_Prop*>(Action.Get());
		if (PropAction && PropAction->Prop)
		{
			IconClass = PropAction->Prop->PropClassSoft.Get();
			if (IconClass)
			{
				return FClassIconFinder::FindThumbnailForClass(IconClass);
			}
		}*/
		//return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.Dialog.Prop");
	}

	
	default:
		break;
	}

	return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.Dialog");
}

void SDialogPaletteItem::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (DialogEditorPtr.IsValid())
	{
		SGraphPaletteItem::OnDragEnter(MyGeometry, DragDropEvent);
	}
}

TSharedRef<SWidget> SDialogPaletteItem::CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnly)
{
	FName const ActionTypeId = InCreateData->Action->GetTypeId();

	FOnVerifyTextChanged OnVerifyTextChanged;
	FOnTextCommitted     OnTextCommitted;

	// default to our own rename methods
	OnVerifyTextChanged.BindSP(this, &SDialogPaletteItem::OnNameTextVerifyChanged);
	OnTextCommitted.BindSP(this, &SDialogPaletteItem::OnNameTextCommitted);

	// Copy the mouse delegate binding if we want it
	if (InCreateData->bHandleMouseButtonDown)
	{
		MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	}

	TSharedPtr<SToolTip> ToolTipWidget = ConstructToolTipWidget();

	TSharedPtr<SOverlay> DisplayWidget;
	TSharedPtr<SInlineEditableTextBlock> EditableTextElement;
	SAssignNew(DisplayWidget, SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(EditableTextElement, SInlineEditableTextBlock)
				.Text(this, &SDialogPaletteItem::GetDisplayText)
				.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
				.HighlightText(InCreateData->HighlightText)
				.ToolTip(ToolTipWidget)
				.OnVerifyTextChanged(OnVerifyTextChanged)
				.OnTextCommitted(OnTextCommitted)
				.IsSelected(InCreateData->IsRowSelectedDelegate)
				.IsReadOnly(false)
		];
	InlineRenameWidget = EditableTextElement.ToSharedRef();

	InCreateData->OnRenameRequest->BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);

	return DisplayWidget.ToSharedRef();
}

FText SDialogPaletteItem::GetDisplayText() const
{
	const UEdGraphSchema_DialogBuilder* DialogSchema = GetDefault<UEdGraphSchema_DialogBuilder>();
	if (!MenuDescriptionCache.IsOutOfDate(DialogSchema))
	{
		return MenuDescriptionCache;
	}

	TSharedPtr<FEdGraphSchemaAction> GraphAction = ActionPtr.Pin();
	if (!GraphAction.IsValid())
	{
		return FText::FromString("Empty");
	}

	if (GraphAction->GetTypeId() == FAssetSchemaAction_Participant::StaticGetTypeId())
	{
		const FAssetSchemaAction_Participant* ParticipantAction = static_cast<const FAssetSchemaAction_Participant*>(GraphAction.Get());
		if (const UDialogPlayerParticipant* PlayerParticipant = Cast<UDialogPlayerParticipant>(ParticipantAction ? ParticipantAction->Participant : nullptr))
		{
			const FText ParticipantName = FText::FromString("PLAYER - " + PlayerParticipant->DisplayName.ToString());
			MenuDescriptionCache.SetCachedText(ParticipantName.IsEmpty() ? FText::FromString("PLAYER - EMPTY") : ParticipantName, DialogSchema);
		}
		else if (const UDialogParticipant* DialogParticipant = Cast<UDialogParticipant>(ParticipantAction ? ParticipantAction->Participant : nullptr))
		{
			const FText ParticipantName = DialogParticipant->GetDisplayName();
			MenuDescriptionCache.SetCachedText(ParticipantName.IsEmpty() ? FText::FromString("Empty") : ParticipantName, DialogSchema);
		}
	}
	else if (GraphAction->GetTypeId() == FAssetSchemaAction_Prop::StaticGetTypeId())
	{
		const FAssetSchemaAction_Prop* PropAction = static_cast<const FAssetSchemaAction_Prop*>(GraphAction.Get());
		if (const UDialogProp* DialogProp = Cast<UDialogProp>(PropAction ? PropAction->Prop : nullptr))
		{
			const FText PropName = DialogProp->DisplayName;
			MenuDescriptionCache.SetCachedText(PropName.IsEmpty() ? FText::FromString("Empty") : PropName, DialogSchema);
		}
	}
	else if (GraphAction->GetTypeId() == FAssetSchemaAction_Light::StaticGetTypeId())
	{
		const FAssetSchemaAction_Light* LightAction = static_cast<const FAssetSchemaAction_Light*>(GraphAction.Get());
		if (const UDialogLight* DialogLight = Cast<UDialogLight>(LightAction ? LightAction->Light : nullptr))
		{
			const FText LightName = DialogLight->DisplayName;
			MenuDescriptionCache.SetCachedText(LightName.IsEmpty() ? FText::FromString("Empty") : LightName, DialogSchema);
		}
	}

	return MenuDescriptionCache;
}

bool SDialogPaletteItem::OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage)
{
	FString FinalString = InNewText.ToString().TrimStartAndEnd();

	FName OriginalName;

	FAssetSchemaAction_Participant* DialogGraphAction = (FAssetSchemaAction_Participant*)ActionPtr.Pin().Get();
	

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> DialogGraphDataArray;
	AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UDialogBuilderGraph::StaticClass()), DialogGraphDataArray);

	
	UStruct* ValidationScope = nullptr;

	const UEdGraphSchema* Schema = nullptr;

	return true;
}

#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"

void SDialogPaletteItem::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)  
{  
  const FString NewNameString = NewText.ToString().TrimStartAndEnd();  
  const FName NewName = *NewNameString;  
  FName OriginalName;  

  FAssetSchemaAction_Participant* DialogGraphAction = (FAssetSchemaAction_Participant*)ActionPtr.Pin().Get();  

  DialogEditorPtr.Pin()->GetDialogDefinitionsWidget()->Refresh();  

  DialogEditorPtr.Pin()->GetDialogBuilderGraph()->Modify();  
}

TSharedPtr<SToolTip> SDialogPaletteItem::ConstructToolTipWidget() const
{

	return TSharedPtr<SToolTip>();
}

FText SDialogPaletteItem::GetToolTipText() const
{
	return FText::FromString(TEXT(""));
}

void SDialogPalette::Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor)
{
}

void SDialogPalette::OnSplitterResized() const
{
}

#undef LOCTEXT_NAMESPACE