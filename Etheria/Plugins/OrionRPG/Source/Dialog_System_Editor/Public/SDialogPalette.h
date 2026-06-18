// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEditor.h"
#include "SGraphPalette.h"
#include "Widgets/SCompoundWidget.h"


/*******************************************************************************
* SDialogPaletteItem
*******************************************************************************/

/** Widget for displaying a single item  */
class SDialogPaletteItem : public SGraphPaletteItem
{
public:
	SLATE_BEGIN_ARGS(SDialogPaletteItem)
		: _ShowClassInTooltip(false)
		{}

		SLATE_ARGUMENT(bool, ShowClassInTooltip)
	SLATE_END_ARGS()

	/**
	 * Creates the slate widget to be place in a palette.
	 *
	 * @param  InArgs				A set of slate arguments, defined above.
	 * @param  InCreateData			A set of data associated with a FEdGraphSchemaAction that this item represents.
	 * @param  InDialogEditor	A pointer to the Dialog editor that the palette belongs to.
	 */
	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, TWeakPtr<FDialogBuilderEditor> InDialogEditor);
	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, UDialogBuilderGraph* InDialogSystemGraph);

private:
	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, UDialogBuilderGraph* InDialogSystemGraph, TWeakPtr<FDialogBuilderEditor> InDialogEditor);

	// SWidget Interface
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End of SWidget Interface

	// SGraphPaletteItem Interface
	virtual TSharedRef<SWidget> CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnly) override;
	virtual FText GetDisplayText() const override;
	virtual bool OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage) override;
	virtual void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit) override;
	// End of SGraphPaletteItem Interface

	/**
	 * Creates a tooltip widget based off the specified action (attempts to
	 * mirror the tool-tip that would be found on the node once it's placed).
	 *
	 * @return A new slate widget to be used as the tool tip for this item's text element.
	 */
	TSharedPtr<SToolTip> ConstructToolTipWidget() const;
	const FSlateBrush* GetPaletteIconBrushForAction(const TSharedPtr<FEdGraphSchemaAction>& Action);
	/** Returns the up-to-date tooltip for the item */
	FText GetToolTipText() const;
private:
	/** True if the class should be displayed in the tooltip */
	bool bShowClassInTooltip;

	UDialogBuilderGraph* DialogBuilderGraph;

	/** Pointer back to the Dialog editor that owns this */
	TWeakPtr<FDialogBuilderEditor> DialogEditorPtr;

	/** Cache for the MenuDescription to be displayed for this item */
	FNodeTextCache MenuDescriptionCache;
};

/**
 * 
 */
class DIALOG_SYSTEM_EDITOR_API SDialogPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogPalette)
	{}
	SLATE_END_ARGS()

	/**
	 * Creates the slate widget that represents a list of available actions for
	 * the specified Dialog.
	 *
	 * @param  InArgs				A set of slate arguments, defined above.
	 * @param  InDialogEditor	A pointer to the Dialog editor that this palette belongs to.
	 */ 
	void Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor);

private:
	/**
	 * Saves off the user's new sub-palette configuration (so as to not annoy
	 * them by reseting it every time they open the Dialog editor).
	 */
	void OnSplitterResized() const;

	TSharedPtr<SWidget> FavoritesWrapper;
	TSharedPtr<SSplitter> PaletteSplitter;
	TSharedPtr<SWidget> LibraryWrapper;
};
