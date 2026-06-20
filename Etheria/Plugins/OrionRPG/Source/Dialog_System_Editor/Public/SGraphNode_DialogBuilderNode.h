// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/SCompoundWidget.h"
#include "DialogBuilderEdNode.h"
#include "Editor/GraphEditor/Private/DragNode.h"

class FDragDialogGraphNode : public FDragNode
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragDialogGraphNode, FDragNode)

		static TSharedRef<FDragDialogGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);
	static TSharedRef<FDragDialogGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes);

	UDialogBuilderEdNode* GetDropTargetNode() const;

	double StartTime;

protected:
	typedef FDragNode Super;
};

class UDialogBuilderEdNode;
enum class EEventLaunchType : uint8;
class DIALOG_SYSTEM_EDITOR_API SGraphNode_DialogBuilderNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_DialogBuilderNode)
	{}
	SLATE_END_ARGS()
	// SWidget interface 

	//~ Begin SGraphNode Interface
	void Construct(const FArguments& InArgs, UDialogBuilderEdNode* InNode);
	virtual void UpdateGraphNode() override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	virtual TSharedRef<SGraphNode> GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel) override;
	virtual bool IsNameReadOnly() const override;
	virtual void CreatePinWidgets() override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	virtual void MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const override;
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
#endif
	//~ End SGraphNode Interface

	/** handle double click */
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	virtual FText GetDescription() const;
	virtual EVisibility GetDescriptionVisibility() const;
	virtual EVisibility GetEventsVisibility() const;
	virtual EVisibility GetLaunchTypeEventsVisibility(EEventLaunchType LaunchType) const;
	virtual EVisibility GetStartEventsVisibility() const;
	virtual EVisibility GetEndEventsVisibility() const;
	virtual EVisibility GetBothEventsVisibility() const;
	virtual EVisibility GetDecoratorVisibility() const;

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** adds decorator widget inside current node */
	void AddDecorator(TSharedPtr<SGraphNode> DecoratorWidget);

	/** adds event widget inside current node */
	void AddEvent(TSharedPtr<SGraphNode> EventWidget);
	void AddEvent(TSharedPtr<SGraphNode> EventWidget, EEventLaunchType EventLaunchType);


	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** adds subnode widget inside current node */
	virtual void AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget);

	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	/** Called when text is being committed to check for validity */
	bool OnVerifyNameTextChanged ( const FText& InText, FText& OutErrorMessage );

	/* Called when text is committed on the node */
	void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

	// Should we use low-detail node titles?
	virtual bool UseLowDetailNodeTitles() const;

	FText GetPinTooltip(UEdGraphPin* GraphPinObj) const;

	virtual FSlateColor GetBackgroundColor() const;
	virtual FSlateColor GetBorderBackgroundColor() const;
	virtual FSlateColor GetSelectorColor() const;

	virtual FText GetNodeIDText() const;
	
	virtual EVisibility GetNodeIDVisibility() const;
	virtual EVisibility GetTitleVisibility() const;
	virtual EVisibility GetParentNodeVisibility() const;
	virtual EVisibility GetNodeHeaderTitleVisibility() const;
	virtual EVisibility GetSubNodeVisibility() const;
	virtual EVisibility GetDragOverMarkerVisibility() const;

	bool IsCurrentEditingSequenceNode() const;
	
	/** shows red marker when search failed*/
	EVisibility GetDebuggerSearchFailedMarkerVisibility() const;

	virtual const FSlateBrush* GetNameIcon() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);
public:
	void OnSequencerButtonHovered();
	void OnSequencerButtonUnhovered();
	FSlateColor GetSequencerButtonColor() const;
	const FSlateBrush* GetDialogSequenceThumbnail() const;

protected:
	mutable FSlateBrush SequenceThumbnailBrush;
	mutable FSlateBrush SequenceThumbnailFallbackBrush;

	uint32 bSequencerButtonHovered : 1;
	/** The node body widget, cached here so we can determine its size when we want ot position our overlays */
	TSharedPtr<SBorder> NodeBody;

	TArray< TSharedPtr<SGraphNode> > SubNodes;

	uint32 bDragMarkerVisible : 1;

	TArray< TSharedPtr<SGraphNode> > DecoratorWidgets;
	TArray< TSharedPtr<SGraphNode> > EventsWidgets;
	TSharedPtr<SVerticalBox> DecoratorsBox;
	TSharedPtr<SVerticalBox> EventsBox;
	TSharedPtr<SVerticalBox> StartEventsBox;
	TSharedPtr<SVerticalBox> EndEventsBox;
	TSharedPtr<SVerticalBox> BothEventsBox;
	TSharedPtr<SHorizontalBox> OutputPinBox;
	TSharedPtr<SWidget> DialogNodeWidgetRef;

	/** The widget we use to display the index of the node */
	TSharedPtr<SWidget> IndexOverlay;

	EVisibility GetBlueprintIconVisibility() const;
	EEventLaunchType GetEventLaunchTypeForDrop(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	void SetEventLaunchType(UDialogBuilderEdNode* EventNode, EEventLaunchType EventLaunchType);

	/** Sequencer quick-open icon visibility */
	EVisibility GetSequencerIconVisibility() const;

	/** Open sequence asset in Sequencer */
	FReply OnOpenSequencerClicked();

	//Dialog Node Index

	/** Get the visibility of the index overlay */
	EVisibility GetIndexVisibility() const;

	/** Get the text to display in the index overlay */
	FText GetIndexText() const;

	/** Get the text to display top of node title */
	FText GetNodeHeaderTitleText() const;

	/** Get the tooltip for the index overlay */
	FText GetIndexTooltipText() const;

	/** Get the color to display for the index overlay. This changes on hover state of sibling nodes */
	FSlateColor GetIndexColor(bool bHovered) const;

	/** Handle hover state changing for the index widget - we use this to highlight sibling nodes */
	void OnIndexHoverStateChanged(bool bHovered);
};
