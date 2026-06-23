// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "SGraphNode_DialogBuilderNode.h"
#include "DialogBuilderEditor.h"
#include "DialogBuilder_EditorStyle.h"
#include "SlateOptMacros.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderEdNode_PlayerChoice.h"
#include "DialogBuilderEdNode_DialogSequence.h"
#include "Types/SlateStructs.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"
#include "SGraphPanel.h"
#include "NodeFactory.h"
#include "Components/VerticalBox.h"
#include "SGraphPreviewer.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SGraphPin.h"
#include "Colors_DialogBuilder.h"
#include "GraphEditorSettings.h"
#include "SCommentBubble.h"
#include "SLevelOfDetailBranchNode.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderEdNode_Root.h"
#include "DialogBuilderEdNode_DialogLine.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderSetting.h"	
#include "Fonts/SlateFontInfo.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DialogBuilderEdSubNode_Decorator.h"
#include "DialogBuilderEdSubNode_Event.h"
#include "IDocumentation.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "EdNode_DialogBuilder"

#define ALWAYS_SHOW_QUEST_EXECUTION_INDEX 1

bool ShouldShowExecutionIndex()
{
#ifdef ALWAYS_SHOW_QUEST_EXECUTION_INDEX
	return true;
#else
	return GEditor && (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != NULL);
#endif // ALWAYS_SHOW_BT_EXECUTION_INDEX
}

//////////////////////////////////////////////////////////////////////////
class SDialogSystemPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogSystemPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		SetCursor(EMouseCursor::Default);

		bShowLabel = true;
		GraphPinObj = InPin;
		check(GraphPinObj != nullptr);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		CachePinIcons();

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SDialogSystemPin::GetPinIcon)
			.BorderBackgroundColor(this, &SDialogSystemPin::GetPinColor)
			.OnMouseButtonDown(this, &SDialogSystemPin::OnPinMouseDown)
			.Cursor(this, &SDialogSystemPin::GetPinCursor)
			.Padding(FMargin(1.0f))
			[
				SNew(SBox)
					.WidthOverride(10.0f)
					.HeightOverride(14.0f)
			]
		);
	}

protected:
	const FSlateBrush* CachedImg_Pin_ConnectedHovered;
	const FSlateBrush* CachedImg_Pin_DisconnectedHovered;

	void CachePinIcons()
	{
		CachedImg_Pin_ConnectedHovered = FAppStyle::GetBrush(TEXT("Graph.ExecPin.ConnectedHovered"));
		CachedImg_Pin_Connected = FAppStyle::GetBrush(TEXT("Graph.ExecPin.Connected"));
		CachedImg_Pin_DisconnectedHovered = FAppStyle::GetBrush(TEXT("Graph.ExecPin.DisconnectedHovered"));
		CachedImg_Pin_Disconnected = FAppStyle::GetBrush(TEXT("Graph.ExecPin.Disconnected"));
	}


	const FSlateBrush* GetPinIcon() const
	{
		const FSlateBrush* Brush = NULL;

		if (IsConnected())
		{
			Brush = IsHovered() ? CachedImg_Pin_ConnectedHovered : CachedImg_Pin_Connected;
		}
		else
		{
			Brush = IsHovered() ? CachedImg_Pin_ConnectedHovered : CachedImg_Pin_Disconnected;
		}

		return Brush;
	}
	virtual FSlateColor GetPinColor() const override
	{
		return bIsDiffHighlighted ? DialogBuilderColors::Pin::Diff :
			IsHovered() ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(.5f, .5f, .5f);
	}

	virtual TSharedRef<SWidget> GetDefaultValueWidget() override
	{
		return SNew(SSpacer);
	}

	const FSlateBrush* GetPinBorder() const
	{
		const bool bIsConnected = GraphPinObj && GraphPinObj->LinkedTo.Num() > 0;
		return FAppStyle::GetBrush(bIsConnected ? TEXT("Graph.Pin.Connected") : TEXT("Graph.Pin.Disconnected"));
	}
};


/** Widget for overlaying an execution-order index onto a node */
class SDialogNodeIndex : public SCompoundWidget
{
public:
	/** Delegate event fired when the hover state of this widget changes */
	DECLARE_DELEGATE_OneParam(FOnHoverStateChanged, bool /* bHovered */);

	/** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
	DECLARE_DELEGATE_RetVal_OneParam(FSlateColor, FOnGetIndexColor, bool /* bHovered */);

	SLATE_BEGIN_ARGS(SDialogNodeIndex) {}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_EVENT(FOnHoverStateChanged, OnHoverStateChanged)
		SLATE_EVENT(FOnGetIndexColor, OnGetIndexColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OnHoverStateChangedEvent = InArgs._OnHoverStateChanged;
		OnGetIndexColorEvent = InArgs._OnGetIndexColor;

		const FSlateBrush* IndexBrush = FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Index"));

		ChildSlot
			[
				SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						// Add a dummy box here to make sure the widget doesnt get smaller than the brush
						SNew(SBox)
							.WidthOverride(IndexBrush->ImageSize.X)
							.HeightOverride(IndexBrush->ImageSize.Y)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBorder)
							.BorderImage(IndexBrush)
							.BorderBackgroundColor(this, &SDialogNodeIndex::GetColor)
							.Padding(FMargin(4.0f, 0.0f, 4.0f, 1.0f))
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
									.Text(InArgs._Text)
									.Font(FAppStyle::GetFontStyle("BTEditor.Graph.BTNode.IndexText"))
							]
					]
			];
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		OnHoverStateChangedEvent.ExecuteIfBound(true);
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		OnHoverStateChangedEvent.ExecuteIfBound(false);
		SCompoundWidget::OnMouseLeave(MouseEvent);
	}

	/** Get the color we use to display the rounded border */
	FSlateColor GetColor() const
	{
		if (OnGetIndexColorEvent.IsBound())
		{
			return OnGetIndexColorEvent.Execute(IsHovered());
		}

		return FSlateColor::UseForeground();
	}

private:
	/** Delegate event fired when the hover state of this widget changes */
	FOnHoverStateChanged OnHoverStateChangedEvent;

	/** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
	FOnGetIndexColor OnGetIndexColorEvent;
};


void SGraphNode_DialogBuilderNode::Construct(const FArguments& InArgs, UDialogBuilderEdNode* InNode)
{
	SetCursor(EMouseCursor::CardinalCross);
	GraphNode = InNode;
	UpdateGraphNode();
	InNode->SGraphNode = this;
	bDragMarkerVisible = false;
	SequenceThumbnailFallbackBrush = FSlateBrush();
	SequenceThumbnailFallbackBrush.DrawAs = ESlateBrushDrawType::Image;
	SequenceThumbnailFallbackBrush.TintColor = FSlateColor(FLinearColor::Black);
	SequenceThumbnailFallbackBrush.ImageSize = FVector2D(32.0f, 32.0f);
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGraphNode_DialogBuilderNode::UpdateGraphNode()
{
	bDragMarkerVisible = false;
	InputPins.Empty();
	OutputPins.Empty();

	if (DecoratorsBox.IsValid())
	{
		DecoratorsBox->ClearChildren();
	}		
	else
	{
		SAssignNew(DecoratorsBox, SVerticalBox);
	}

	if (EventsBox.IsValid())
	{
		EventsBox->ClearChildren();
	}
	else
	{
		SAssignNew(EventsBox, SVerticalBox);
	}

	SAssignNew(StartEventsBox, SVerticalBox);
	SAssignNew(EndEventsBox, SVerticalBox);
	SAssignNew(BothEventsBox, SVerticalBox);

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset(); 
	DecoratorWidgets.Reset();
	EventsWidgets.Reset();
	SubNodes.Reset();
	OutputPinBox.Reset();

	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;

	if (DialogEdNode)
	{
		for (int32 i = 0; i < DialogEdNode->Decorators.Num(); i++)
		{
			if (DialogEdNode->Decorators.IsValidIndex(i))
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(DialogEdNode->Decorators[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				AddDecorator(NewNode);
				NewNode->UpdateGraphNode();
			}
		}
		for (int32 i = 0; i < DialogEdNode->Events.Num(); i++)
		{
			if (DialogEdNode->Events.IsValidIndex(i))
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(DialogEdNode->Events[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				UOrionEvent* Event = DialogEdNode->Events[i] ? Cast<UOrionEvent>(DialogEdNode->Events[i]->NodeInstance) : nullptr;
				AddEvent(NewNode, Event ? Event->EventLaunchType : EEventLaunchType::E_Start);
				NewNode->UpdateGraphNode();
			}
		}
	}

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<STextBlock> DescriptionText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	TWeakPtr<SNodeTitle> WeakNodeTitle = NodeTitle;

	auto GetNodeTitlePlaceholderWidth = [WeakNodeTitle]() -> FOptionalSize
		{
			TSharedPtr<SNodeTitle> NodeTitlePin = WeakNodeTitle.Pin();
			const float DesiredWidth = (NodeTitlePin.IsValid()) ? NodeTitlePin->GetTitleSize().X : 0.0f;
			return FMath::Max(75.0f, DesiredWidth);
		};
	auto GetNodeTitlePlaceholderHeight = [WeakNodeTitle]() -> FOptionalSize
		{
			TSharedPtr<SNodeTitle> NodeTitlePin = WeakNodeTitle.Pin();
			const float DesiredHeight = (NodeTitlePin.IsValid()) ? NodeTitlePin->GetTitleSize().Y : 0.0f;
			return FMath::Max(22.0f, DesiredHeight);
		};

	const FMargin NodePadding = (Cast<UDialogBuilderEdSubNode_Decorator>(GraphNode) || Cast<UDialogBuilderEdSubNode_Event>(GraphNode))
		? FMargin(1.f)
		: FMargin(8.0f);

	const float NodeMaxWidth = DialogEdNode && DialogEdNode->IsSubNode()
		? 325.f
		: 350.f;

	const float NodeMinWidth = DialogEdNode && DialogEdNode->IsSubNode()
		? 200.f
		: 125.f;

	const FMargin PinPadding = (Cast<UDialogBuilderEdSubNode_Decorator>(GraphNode) || Cast<UDialogBuilderEdSubNode_Event>(GraphNode))
		? FMargin(0.f)
		: FMargin(5.f, 3.f, 5.f, 3.f);

	UWorld* World = GEditor->GetEditorWorldContext().World();

	IndexOverlay = SNew(SDialogNodeIndex)
		.ToolTipText(this, &SGraphNode_DialogBuilderNode::GetIndexTooltipText)
		.Visibility(this, &SGraphNode_DialogBuilderNode::GetIndexVisibility)
		.Text(this, &SGraphNode_DialogBuilderNode::GetIndexText)
		.OnHoverStateChanged(this, &SGraphNode_DialogBuilderNode::OnIndexHoverStateChanged)
		.OnGetIndexColor(this, &SGraphNode_DialogBuilderNode::GetIndexColor);
	

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
					.BorderBackgroundColor(this, &SGraphNode_DialogBuilderNode::GetSelectorColor)
					.Visibility(this, &SGraphNode_DialogBuilderNode::GetNodeHeaderTitleVisibility)
					[
						SNew(SOverlay)
							// Pins and node details
							+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SNew(SHorizontalBox)
									// Selector Desc
									+ SHorizontalBox::Slot()
									.Padding(FMargin(3.f,0.5f))
									.AutoWidth()
									[
										SNew(STextBlock)
											.Text(this, &SGraphNode_DialogBuilderNode::GetNodeHeaderTitleText)
											.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5))
											.Clipping(EWidgetClipping::ClipToBounds)
									]
							]
					]
			]
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
						.Padding(0.0f)
						.BorderBackgroundColor(this, &SGraphNode_DialogBuilderNode::GetBorderBackgroundColor)
						.OnMouseButtonDown(this, &SGraphNode_DialogBuilderNode::OnMouseDown)
						[
							SNew(SOverlay)
								// Pins and node details
								+ SOverlay::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Fill)
								[
									SNew(SHorizontalBox)

										// INPUT PIN AREA
										+ SHorizontalBox::Slot()
										.Padding(PinPadding)
										.AutoWidth()
										[
											SNew(SBox)
												.MinDesiredWidth(NodePadding.Left)
												[
													SAssignNew(LeftNodeBox, SVerticalBox)
												]
										]

									// STATE NAME AREA
									+ SHorizontalBox::Slot()
										.Padding(FMargin(NodePadding))
										[
											SNew(SVerticalBox)
												+ SVerticalBox::Slot()
												.AutoHeight()
												[
													SAssignNew(NodeBody, SBorder)
														.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
														.BorderBackgroundColor(this, &SGraphNode_DialogBuilderNode::GetBackgroundColor)
														.HAlign(HAlign_Fill)
														.VAlign(VAlign_Center)
														.Visibility(EVisibility::SelfHitTestInvisible)
														[
															SNew(SOverlay)
																+ SOverlay::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Fill)
																[
																	SNew(SVerticalBox)
																		+ SVerticalBox::Slot()
																		.HAlign(HAlign_Fill)
																		.VAlign(VAlign_Center)
																		.AutoHeight()
																		[
																			SNew(SHorizontalBox)
																				.Visibility(this, &SGraphNode_DialogBuilderNode::GetTitleVisibility)
																				+ SHorizontalBox::Slot()
																				.AutoWidth()
																				[
																					// POPUP ERROR MESSAGE
																					SAssignNew(ErrorText, SErrorText)
																						.BackgroundColor(this, &SGraphNode_DialogBuilderNode::GetErrorColor)
																						.ToolTipText(this, &SGraphNode_DialogBuilderNode::GetErrorMsgToolTip)
																				]
																				+ SHorizontalBox::Slot()
																				.AutoWidth()
																				[
																					SNew(SHorizontalBox)
																						+ SHorizontalBox::Slot()
																						.AutoWidth()
																						.VAlign(VAlign_Top)
																						.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
																						[
																							SNew(SImage)
																								.Image(this, &SGraphNode_DialogBuilderNode::GetNameIcon)
																								//.Visibility(this, &SGraphNode_DialogBuilderNode::GetParentNodeVisibility)
																						]
																						//if parent node
																						+ SHorizontalBox::Slot()
																						//.MinWidth(NodeMinWidth)
																						.MaxWidth(NodeMaxWidth)
																						.Padding(FMargin(8.0f, 0.0f, 4.0f, 0.0f))
																						[
																							SNew(SVerticalBox)
																								.Visibility(this, &SGraphNode_DialogBuilderNode::GetParentNodeVisibility)
																								+ SVerticalBox::Slot()
																								.AutoHeight()
																								[
																									SAssignNew(InlineEditableText, SInlineEditableTextBlock)
																										.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
																										.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
																										.OnVerifyTextChanged(this, &SGraphNode_DialogBuilderNode::OnVerifyNameTextChanged)
																										.OnTextCommitted(this, &SGraphNode_DialogBuilderNode::OnNameTextCommited)
																										.IsReadOnly(this, &SGraphNode_DialogBuilderNode::IsNameReadOnly)
																										.IsSelected(this, &SGraphNode_DialogBuilderNode::IsSelectedExclusively)
																										.AutoWrapMultilineEditText(true)

																								]
																							+ SVerticalBox::Slot()
																								.AutoHeight()
																								[
																									NodeTitle.ToSharedRef()
																								]
																								+ SVerticalBox::Slot()
																								.AutoHeight()
																								.Padding(FMargin(2.0f, 1.0f, 2.0f, 1.0f))
																								[
																									// NodeTag
																									SAssignNew(DescriptionText, STextBlock)
																										.Visibility(this, &SGraphNode_DialogBuilderNode::GetNodeIDVisibility)
																										.Text(this, &SGraphNode_DialogBuilderNode::GetNodeIDText)
																										.TextStyle(FAppStyle::Get(), TEXT("RichTextBlock.Italic"))
																										.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
																								]
																							+ SVerticalBox::Slot()
																								.AutoHeight()
																								.Padding(FMargin(2.0f, 1.0f, 2.0f, 4.0f))
																								[
																									// DESCRIPTION MESSAGE
																									SAssignNew(DescriptionText, STextBlock)
																										.Visibility(this, &SGraphNode_DialogBuilderNode::GetDescriptionVisibility)
																										.Text(this, &SGraphNode_DialogBuilderNode::GetDescription)
																										.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5))
																										.WrapTextAt(NodeMaxWidth)
																										.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
																								]
																						]
																					//if subnode
																					+ SHorizontalBox::Slot()
																						//.MinWidth(NodeMinWidth)
																						.MaxWidth(NodeMaxWidth)
																						.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
																						[
																							SNew(SVerticalBox)
																								.Visibility(this, &SGraphNode_DialogBuilderNode::GetSubNodeVisibility)
																								+ SVerticalBox::Slot()
																								.AutoHeight()
																								[
																									SNew(STextBlock)
																										.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
																										.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5))
																										.Clipping(EWidgetClipping::ClipToBounds)
																								]
																							+ SVerticalBox::Slot()
																								.AutoHeight()
																								[
																									NodeTitle.ToSharedRef()
																								]
																								+ SVerticalBox::Slot()

																								.AutoHeight()
																								.Padding(FMargin(1.0f, 0.0f, 1.0f, 1.0f))
																								[
																									// DESCRIPTION MESSAGE
																									SAssignNew(DescriptionText, STextBlock)
																										.Visibility(this, &SGraphNode_DialogBuilderNode::GetDescriptionVisibility)
																										.Text(this, &SGraphNode_DialogBuilderNode::GetDescription)
																										.TextStyle(FAppStyle::Get(), TEXT("RichTextBlock.Italic"))
																										.WrapTextAt(NodeMaxWidth)
																										.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
																								]
																						]

																				]
																		]

																]
														]
												]
											/*+ SVerticalBox::Slot()
												.AutoHeight()
												.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
												[
													SNew(SBox)
														.WidthOverride(180.0f)
														.HeightOverride(100.0f)
														[
															SNew(SImage)
																.Image(this, &SGraphNode_DialogBuilderNode::GetDialogSequenceThumbnail)
														]
												]*/
											+ SVerticalBox::Slot()
												.AutoHeight()
												[
													SAssignNew(NodeBody, SBorder)
														.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
														.BorderBackgroundColor(DialogBuilderColors::NodeBorder::SubNodeBorder)
														.HAlign(HAlign_Fill)
														.VAlign(VAlign_Center)
														.Visibility(this, &SGraphNode_DialogBuilderNode::GetEventsVisibility)
														[
															SNew(SOverlay)
																+ SOverlay::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Fill)
																[
																	SNew(SVerticalBox)
																		+ SVerticalBox::Slot()
																		.HAlign(HAlign_Fill)
																		.VAlign(VAlign_Center)
																		.AutoHeight()
																		[
																			SNew(SHorizontalBox)
																				/*+ SHorizontalBox::Slot()
																				.AutoWidth()
																				.VAlign(VAlign_Center)
																				[
																					SNew(SImage)
																						.Image(FAppStyle::GetBrush(TEXT("GraphEditor.CustomEvent_16x")))
																				]*/
																				+SHorizontalBox::Slot()
																				.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
																				[
																					SNew(STextBlock)
																						.Text(LOCTEXT("EventLabel", "Events"))
																						.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																						.Clipping(EWidgetClipping::Inherit)
																				]
																		]
																	+ SVerticalBox::Slot()
																		.Padding(FMargin(8.0f, 0, 0, 0))
																		.AutoHeight()
																		[
																			SNew(SVerticalBox)
																			+ SVerticalBox::Slot()
																			.AutoHeight()
																			[
																				SNew(SBorder)
																					.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
																					.BorderBackgroundColor(DialogBuilderColors::NodeBorder::SubNodeBorder)
																					.Visibility(this, &SGraphNode_DialogBuilderNode::GetStartEventsVisibility)
																					[
																						SNew(SVerticalBox)
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(4.0f, 0.0f))
																						[
																							SNew(STextBlock)
																								.Text(LOCTEXT("StartEventLabel", "Start"))
																								.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																						]
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
																						[
																							StartEventsBox.ToSharedRef()
																						]
																					]
																			]
																			+ SVerticalBox::Slot()
																			.AutoHeight()
																			[
																				SNew(SBorder)
																					.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
																					.BorderBackgroundColor(DialogBuilderColors::NodeBorder::SubNodeBorder)
																					.Visibility(this, &SGraphNode_DialogBuilderNode::GetEndEventsVisibility)
																					[
																						SNew(SVerticalBox)
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(4.0f, 0.0f))
																						[
																							SNew(STextBlock)
																								.Text(LOCTEXT("EndEventLabel", "End"))
																								.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																						]
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
																						[
																							EndEventsBox.ToSharedRef()
																						]
																					]
																			]
																			+ SVerticalBox::Slot()
																			.AutoHeight()
																			[
																				SNew(SBorder)
																					.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
																					.BorderBackgroundColor(DialogBuilderColors::NodeBorder::SubNodeBorder)
																					.Visibility(this, &SGraphNode_DialogBuilderNode::GetBothEventsVisibility)
																					[
																						SNew(SVerticalBox)
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(4.0f, 0.0f))
																						[
																							SNew(STextBlock)
																								.Text(LOCTEXT("BothEventLabel", "Both"))
																								.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																						]
																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
																						[
																							BothEventsBox.ToSharedRef()
																						]
																					]
																			]
																		]
																]
														]
												]
											+ SVerticalBox::Slot()
												.AutoHeight()
												[
													SAssignNew(NodeBody, SBorder)
														.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
														.BorderBackgroundColor(DialogBuilderColors::NodeBorder::SubNodeBorder)
														.HAlign(HAlign_Fill)
														.VAlign(VAlign_Center)
														.Visibility(this, &SGraphNode_DialogBuilderNode::GetDecoratorVisibility)
														[
															SNew(SOverlay)
																+ SOverlay::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Fill)
																[
																	SNew(SVerticalBox)
																		+ SVerticalBox::Slot()
																		.HAlign(HAlign_Fill)
																		.VAlign(VAlign_Center)
																		.AutoHeight()
																		[
																			SNew(SHorizontalBox)
																				/*+ SHorizontalBox::Slot()
																				.AutoWidth()
																				.VAlign(VAlign_Center)
																				[
																					SNew(SImage)
																						.Image(FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Decorator.Conditional.Icon")))
																				]*/
																				+SHorizontalBox::Slot()
																				.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
																				[
																					SNew(STextBlock)
																						.Text(LOCTEXT("DecoratorLabel", "Conditions"))
																						.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
																						.Clipping(EWidgetClipping::Inherit)
																				]
																		]
																	+ SVerticalBox::Slot()
																		.Padding(FMargin(8.0f, 0, 0, 0))
																		.AutoHeight()
																		[
																			DecoratorsBox.ToSharedRef()
																		]
																]
														]
												]

										]

									// OUTPUT PIN AREA
										+ SHorizontalBox::Slot()
											.Padding(PinPadding)
											.AutoWidth()
											[
												SNew(SBox)
													.Padding(FMargin(0.0f, 5.0f, 0.0f, 0.0f))
													.MinDesiredWidth(NodePadding.Right)
													[
														SAssignNew(RightNodeBox, SVerticalBox)
													]
											]
								]
							// Drag marker overlay
							+ SOverlay::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Top)
								[
									SNew(SBorder)
										.BorderBackgroundColor(DialogBuilderColors::Action::DragMarker)
										.ColorAndOpacity(DialogBuilderColors::Action::DragMarker)
										.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
										.Visibility(this, &SGraphNode_DialogBuilderNode::GetDragOverMarkerVisibility)
										[
											SNew(SBox)
												.HeightOverride(4.f)
										]
								]

							// Node action indicators overlay
							+ SOverlay::Slot()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Top)
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
												.ButtonStyle(FAppStyle::Get(), "NoBorder")
												.ContentPadding(FMargin(-10.f, -32.5f, 0.f, 0.f))
												.ToolTipText(LOCTEXT("OpenDialogSequenceTooltip", "Open Dialog Sequence in Sequencer"))
												.Visibility(this, &SGraphNode_DialogBuilderNode::GetSequencerIconVisibility)
												.OnClicked(this, &SGraphNode_DialogBuilderNode::OnOpenSequencerClicked)
												.OnHovered(this, &SGraphNode_DialogBuilderNode::OnSequencerButtonHovered)
												.OnUnhovered(this, &SGraphNode_DialogBuilderNode::OnSequencerButtonUnhovered)
												[
													SNew(SBox)
														.WidthOverride(30.0f)
														.HeightOverride(30.0f)
														[
															SNew(SImage)
																.Image(FAppStyle::GetBrush(TEXT("ClassIcon.LevelSequence")))
																.ColorAndOpacity(this, &SGraphNode_DialogBuilderNode::GetSequencerButtonColor)
														]
												]
										]
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SImage)
											.Image(FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Blueprint")))
											.Visibility(this, &SGraphNode_DialogBuilderNode::GetBlueprintIconVisibility)
										]

								]

						]
				]
				
			
				
		];
		// Create comment bubble
		TSharedPtr<SCommentBubble> CommentBubble;
		const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

		SAssignNew(CommentBubble, SCommentBubble)
			.GraphNode(GraphNode)
			.Text(this, &SGraphNode::GetNodeComment)
			.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
			.ColorAndOpacity(CommentColor)
			.AllowPinning(true)
			.EnableTitleBarBubble(true)
			.EnableBubbleCtrls(true)
			.GraphLOD(this, &SGraphNode::GetCurrentLOD)
			.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

		GetOrAddSlot(ENodeZone::TopCenter)
			.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
			.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
			.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
			.VAlign(VAlign_Top)
			[
				CommentBubble.ToSharedRef()
			];

		ErrorReporting = ErrorText;
		ErrorReporting->SetError(ErrorMsg);
	
	CreatePinWidgets();
}

void SGraphNode_DialogBuilderNode::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));

		UDialogBuilderEdNode* TestNode = Cast<UDialogBuilderEdNode>(GraphNode);
		if (DragConnectionOp->IsValidOperation() && TestNode && TestNode->IsSubNode())
		{
			SetDragMarker(true);
		}
	}

	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SGraphNode_DialogBuilderNode::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));
	}
	return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SGraphNode_DialogBuilderNode::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		DragConnectionOp->SetHoveredNode(TSharedPtr<SGraphNode>(NULL));
	}

	SetDragMarker(false);
	SGraphNode::OnDragLeave(DragDropEvent);
}

FReply SGraphNode_DialogBuilderNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetDragMarker(false);

	TSharedPtr<FDragDialogGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragDialogGraphNode>();
	if (DragNodeOp.IsValid())
	{
		if (!DragNodeOp->IsValidOperation())
		{
			return FReply::Handled();
		}

		const float DragTime = float(FPlatformTime::Seconds() - DragNodeOp->StartTime);
		if (DragTime < 0.25f)
		{
			return FReply::Handled();
		}

		UDialogBuilderEdNode* MyNode = Cast<UDialogBuilderEdNode>(GraphNode);
		if (MyNode == nullptr || MyNode->IsSubNode())
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_DragDropNode", "Drag&Drop Node"));
		bool bReorderOperation = true;

		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UDialogBuilderEdNode* DraggedNode = Cast<UDialogBuilderEdNode>(DraggedNodes[Idx]->GetNodeObj());
			if (DraggedNode && DraggedNode->ParentNode)
			{
				if (DraggedNode->ParentNode != GraphNode)
				{
					bReorderOperation = false;
				}

				DraggedNode->ParentNode->RemoveSubNode(DraggedNode);
			}
		}

		UDialogBuilderEdNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		const int32 InsertIndex = MyNode->FindSubNodeDropIndex(DropTargetNode);
		const EEventLaunchType DroppedEventLaunchType = GetEventLaunchTypeForDrop(MyGeometry, DragDropEvent);

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UDialogBuilderEdNode* DraggedTestNode = Cast<UDialogBuilderEdNode>(DraggedNodes[Idx]->GetNodeObj());
			DraggedTestNode->Modify();
			DraggedTestNode->ParentNode = MyNode;
			SetEventLaunchType(DraggedTestNode, DroppedEventLaunchType);

			MyNode->Modify();
			MyNode->InsertSubNodeAt(DraggedTestNode, InsertIndex);
		}

		if (bReorderOperation)
		{
			UpdateGraphNode();
		}
		else
		{
			UDialogBuilderEdGraph* MyGraph = MyNode->GetDialogBuilderEdGraph();
			if (MyGraph)
			{
				MyGraph->OnSubNodeDropped();
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}


FReply SGraphNode_DialogBuilderNode::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		//if we are holding mouse over a subnode
		UDialogBuilderEdNode* TestNode = Cast<UDialogBuilderEdNode>(GraphNode);
		if (TestNode && TestNode->IsSubNode())
		{
			const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragDialogGraphNode::New(Panel, Node));
		}
	}

	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bDragMarkerVisible)
	{
		SetDragMarker(false);
	}

	return FReply::Unhandled();
}

TSharedRef<SGraphNode> SGraphNode_DialogBuilderNode::GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, MouseEvent);
	return SubNode.IsValid() ? SubNode.ToSharedRef() : StaticCastSharedRef<SGraphNode>(AsShared());
}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
void SGraphNode_DialogBuilderNode::MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
void SGraphNode_DialogBuilderNode::MoveTo(const FVector2D & NewPosition, FNodeSet & NodeFilter, bool bMarkDirty)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
#endif
	// keep node order (defined by linked pins) up to date with actual positions
	// this function will keep spamming on every mouse move update
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (DialogEdNode && !DialogEdNode->IsSubNode())
	{
		UDialogBuilderEdGraph* DialogEdGraph = DialogEdNode->GetDialogBuilderEdGraph();
		if (DialogEdGraph)
		{
			for (int32 Idx = 0; Idx < DialogEdNode->Pins.Num(); Idx++)
			{
				UEdGraphPin* Pin = DialogEdNode->Pins[Idx];
				if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 1)
				{
					UEdGraphPin* ParentPin = Pin->LinkedTo[0];
					if (ParentPin)
					{
						DialogEdGraph->RebuildChildOrder(ParentPin->GetOwningNode());
					}
				}
			}
		}
	}
}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
TArray<FOverlayWidgetInfo> SGraphNode_DialogBuilderNode::GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
TArray<FOverlayWidgetInfo> SGraphNode_DialogBuilderNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
#endif
{
	TArray<FOverlayWidgetInfo> Widgets;

	check(NodeBody.IsValid());
	check(IndexOverlay.IsValid());

	FVector2D Origin(0.0f, 0.0f);
	

	FOverlayWidgetInfo Overlay(IndexOverlay);
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	Overlay.OverlayOffset = FVector2f(WidgetSize.X - (IndexOverlay->GetDesiredSize().X * 0.5f), Origin.Y);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
	Overlay.OverlayOffset = FVector2D(WidgetSize.X - (IndexOverlay->GetDesiredSize().X * 0.5f), Origin.Y);
#endif
	Widgets.Add(Overlay);

	

	return Widgets;
}

void SGraphNode_DialogBuilderNode::OnSequencerButtonHovered()
{
	bSequencerButtonHovered = true;
}

void SGraphNode_DialogBuilderNode::OnSequencerButtonUnhovered()
{
	bSequencerButtonHovered = false;
}

FSlateColor SGraphNode_DialogBuilderNode::GetSequencerButtonColor() const
{
	return bSequencerButtonHovered
		? FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
		: FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
}

const FSlateBrush* SGraphNode_DialogBuilderNode::GetDialogSequenceThumbnail() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = DialogEdNode
		? Cast<UDialogBuilderNode_DialogSequence>(DialogEdNode->NodeInstance)
		: nullptr;
	if (!DialogSequenceNode) return nullptr;

	if (DialogSequenceNode->Thumbnail)
	{
		SequenceThumbnailBrush = FSlateBrush();
		SequenceThumbnailBrush.DrawAs = ESlateBrushDrawType::Image;
		SequenceThumbnailBrush.SetResourceObject(DialogSequenceNode->Thumbnail);
		SequenceThumbnailBrush.ImageSize = FVector2D(180.0f, 100.0f);
		return &SequenceThumbnailBrush;
	}

	return &SequenceThumbnailFallbackBrush;
}

void SGraphNode_DialogBuilderNode::CreatePinWidgets()
{
	UDialogBuilderEdNode* StateNode = CastChecked<UDialogBuilderEdNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SDialogSystemPin, MyPin)
				.ToolTipText(this, &SGraphNode_DialogBuilderNode::GetPinTooltip, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

TSharedPtr<SToolTip> SGraphNode_DialogBuilderNode::GetComplexTooltip()
{
	const UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	const bool bHasErrors = DialogEdNode && DialogEdNode->HasErrors();

	if (!bHasErrors)
	{
	}
	return IDocumentation::Get()->CreateToolTip(TAttribute<FText>(this, &SGraphNode::GetNodeTooltip), NULL, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName());

}

void SGraphNode_DialogBuilderNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	TSharedPtr<SVerticalBox> PinBox;
	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		PinBox = LeftNodeBox;
		InputPins.Add(PinToAdd);
	}
	else
	{
		PinBox = RightNodeBox;
		OutputPins.Add(PinToAdd);
	}

	if (PinBox)
	{
		const bool bIsPlayerChoiceOutputPin =
			(PinObj != nullptr) &&
			(PinObj->Direction == EEdGraphPinDirection::EGPD_Output) &&
			Cast<UDialogBuilderEdNode_PlayerChoice>(GraphNode) != nullptr;

		if (bIsPlayerChoiceOutputPin)
		{
			const FText PinLabel = PinObj->PinFriendlyName.IsEmpty() ? FText::FromName(PinObj->PinName) : PinObj->PinFriendlyName;

			PinBox->AddSlot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 5.0f))
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
						.BorderBackgroundColor(FLinearColor(.05f, .05f, .05f))
						.Padding(FMargin(4.0f, 2.0f))
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
								[
									SNew(STextBlock)
										.Text(PinLabel)
										.TextStyle(FAppStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor::White)
								]
							+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									PinToAdd
								]
						]
				];
		}
		else
		{
			PinBox->AddSlot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					PinToAdd
				];
		}
	}
}

int32 SGraphNode_DialogBuilderNode::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (IsCurrentEditingSequenceNode())
	{
		const double Time = FSlateApplication::Get().GetCurrentTime();

		const float Cycle = 1.2f;
		const float T = FMath::Fmod(Time, Cycle) / Cycle;

		const float Expand = FMath::Lerp(6.0f, 28.0f, T);
		const float Alpha = 1.0f - T;

		const FVector2D NodeSize = AllottedGeometry.GetLocalSize();
		const FVector2D RippleSize = NodeSize + FVector2D(Expand * 2.0f);
		const FVector2D RippleOffset = FVector2D(-Expand, -Expand);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				RippleSize,
				FSlateLayoutTransform(RippleOffset)
			),
			FAppStyle::GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.1f, 0.55f, 1.0f, 0.22f * Alpha)
		);
	}

	return SGraphNode::OnPaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 1,
		InWidgetStyle,
		bParentEnabled
	);
}

void SGraphNode_DialogBuilderNode::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
{
	SGraphNode::SetOwner(OwnerPanel);

	for (auto& ChildWidget : SubNodes)
	{
		if (ChildWidget.IsValid())
		{
			ChildWidget->SetOwner(OwnerPanel);
			OwnerPanel->AttachGraphEvents(ChildWidget);
		}
	}
}

bool SGraphNode_DialogBuilderNode::IsNameReadOnly() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;

	UDialogBuilderGraph* DialogGraph = DialogNode ? Cast<UDialogBuilderGraph>(DialogNode->GetOwningDialogGraph()) : nullptr;

	return (DialogEdNode && DialogNode && DialogGraph) &&
		(!GetDefault<UDialogBuilderSetting>()->bCanRenameNode || !DialogNode->IsNameEditable()) || SGraphNode::IsNameReadOnly();
}
FReply SGraphNode_DialogBuilderNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	UDialogBuilderEdNode* TestNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
FReply SGraphNode_DialogBuilderNode::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}
FText SGraphNode_DialogBuilderNode::GetDescription() const
{
	UDialogBuilderEdNode* DialogEdNode = CastChecked<UDialogBuilderEdNode>(GraphNode);
	return DialogEdNode ? DialogEdNode->GetDescription() : FText::GetEmpty();
}
EVisibility SGraphNode_DialogBuilderNode::GetDescriptionVisibility() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
	UDialogBuilderNode_PlayerChoice* PlayerOptionNode = DialogEdNode ? Cast<UDialogBuilderNode_PlayerChoice>(DialogEdNode->NodeInstance) : nullptr;
	if (PlayerOptionNode)
		return EVisibility::Collapsed;
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;

}

EVisibility SGraphNode_DialogBuilderNode::GetEventsVisibility() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (DialogEdNode && DialogEdNode->Events.Num())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}


EVisibility SGraphNode_DialogBuilderNode::GetLaunchTypeEventsVisibility(EEventLaunchType LaunchType) const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
	
	if (DialogNode)
	{
		for (auto& Event : DialogNode->Events)
		{
			if (Event->EventLaunchType == LaunchType)
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}

EVisibility SGraphNode_DialogBuilderNode::GetStartEventsVisibility() const
{
	
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_Start);
}

EVisibility SGraphNode_DialogBuilderNode::GetEndEventsVisibility() const
{
	
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_End);
}

EVisibility SGraphNode_DialogBuilderNode::GetBothEventsVisibility() const
{
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_Both);
}


EVisibility SGraphNode_DialogBuilderNode::GetDecoratorVisibility() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (DialogEdNode && DialogEdNode->Decorators.Num())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

void SGraphNode_DialogBuilderNode::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

}

void SGraphNode_DialogBuilderNode::AddDecorator(TSharedPtr<SGraphNode> DecoratorWidget)
{
	DecoratorsBox->AddSlot()
		.AutoHeight()
		[
			DecoratorWidget.ToSharedRef()
		];

	DecoratorWidgets.Add(DecoratorWidget);
	AddSubNode(DecoratorWidget);
}

void SGraphNode_DialogBuilderNode::AddEvent(TSharedPtr<SGraphNode> EventWidget)
{
	UDialogBuilderEdNode* EventNode = EventWidget.IsValid() ? Cast<UDialogBuilderEdNode>(EventWidget->GetNodeObj()) : nullptr;
	UOrionEvent* Event = EventNode ? Cast<UOrionEvent>(EventNode->NodeInstance) : nullptr;
	AddEvent(EventWidget, Event ? Event->EventLaunchType : EEventLaunchType::E_Start);
}

void SGraphNode_DialogBuilderNode::AddEvent(TSharedPtr<SGraphNode> EventWidget, EEventLaunchType EventLaunchType)
{
	TSharedPtr<SVerticalBox> TargetBox = StartEventsBox;
	if (EventLaunchType == EEventLaunchType::E_End)
	{
		TargetBox = EndEventsBox;
	}
	else if (EventLaunchType == EEventLaunchType::E_Both)
	{
		TargetBox = BothEventsBox;
	}

	TargetBox->AddSlot().AutoHeight()
		[
			EventWidget.ToSharedRef()
		];
	EventsWidgets.Add(EventWidget);
	AddSubNode(EventWidget);
}

EEventLaunchType SGraphNode_DialogBuilderNode::GetEventLaunchTypeForDrop(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	TSet<TSharedRef<SWidget>> EventContainers;
	if (StartEventsBox.IsValid())
	{
		EventContainers.Add(StartEventsBox.ToSharedRef());
	}
	if (EndEventsBox.IsValid())
	{
		EventContainers.Add(EndEventsBox.ToSharedRef());
	}
	if (BothEventsBox.IsValid())
	{
		EventContainers.Add(BothEventsBox.ToSharedRef());
	}

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(MyGeometry, EventContainers, Result);

	if (Result.Num() > 0)
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray(ArrangedChildren.GetInternalArray());

		const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
		if (HoveredIndex != INDEX_NONE)
		{
			const TSharedRef<SWidget>& HoveredWidget = ArrangedChildren[HoveredIndex].Widget;
			if (EndEventsBox.IsValid() && &HoveredWidget.Get() == EndEventsBox.Get())
			{
				return EEventLaunchType::E_End;
			}
			if (BothEventsBox.IsValid() && &HoveredWidget.Get() == BothEventsBox.Get())
			{
				return EEventLaunchType::E_Both;
			}
		}
	}

	return EEventLaunchType::E_Start;
}

void SGraphNode_DialogBuilderNode::SetEventLaunchType(UDialogBuilderEdNode* EventNode, EEventLaunchType EventLaunchType)
{
	UOrionEvent* Event = EventNode ? Cast<UOrionEvent>(EventNode->NodeInstance) : nullptr;
	if (Event && Event->EventLaunchType != EventLaunchType)
	{
		Event->Modify();
		Event->EventLaunchType = EventLaunchType;
	}
}

void SGraphNode_DialogBuilderNode::AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);
}

TSharedPtr<SGraphNode> SGraphNode_DialogBuilderNode::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> ResultNode;

	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > SubWidgetsSet;
	for (int32 i = 0; i < SubNodes.Num(); i++)
	{
		SubWidgetsSet.Add(SubNodes[i].ToSharedRef());
	}

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(WidgetGeometry, SubWidgetsSet, Result);

	if (Result.Num() > 0)
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray(ArrangedChildren.GetInternalArray());

		const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
		if (HoveredIndex != INDEX_NONE)
		{
			ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredIndex].Widget);
		}
	}

	return ResultNode;
}

bool SGraphNode_DialogBuilderNode::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	FString FinalString = InText.ToString().TrimStartAndEnd();

	FName OriginalName;

	bool bValid(true);

	if ((GetEditableNodeTitle() != FinalString) && OnVerifyTextCommit.IsBound())
	{
		OutErrorMessage = LOCTEXT("RenameFailed_NotValid", "Name Not Valid.");
        bValid = OnVerifyTextCommit.Execute(FText::FromString(FinalString), GraphNode, OutErrorMessage);
	}

	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
	UDialogBuilderEdGraph* DialogEdGraph = DialogEdNode ? DialogEdNode->GetDialogBuilderEdGraph() : nullptr;

	if (DialogNode && DialogEdGraph)
	{
		OriginalName = DialogNode->ID;
		for (auto& Node : DialogEdGraph->Nodes)
		{
			UDialogBuilderEdNode* NodeEdNode = Cast<UDialogBuilderEdNode>(Node);
			if (NodeEdNode && NodeEdNode->NodeInstance)
			{
				UDialogBuilderNode* NodeInstance = Cast<UDialogBuilderNode>(NodeEdNode->NodeInstance);
				if (NodeInstance && NodeInstance != DialogNode)
				{
					if (FName(FinalString) == OriginalName)
					{
						return true;
					}
					if (FName(FinalString) == NodeInstance->ID)
					{
						OutErrorMessage = LOCTEXT("RenameFailed_NotValid", "This ID is already used in another Node.");
						return false;
					}
				}
			}
		}
	}


	return bValid;
}
bool SGraphNode_DialogBuilderNode::UseLowDetailNodeTitles() const
{
	if (InlineEditableText.IsValid())
	{
		if (const SGraphPanel* MyOwnerPanel = GetOwnerPanel().Get())
		{
			return (MyOwnerPanel->GetCurrentLOD() <= EGraphRenderingLOD::LowestDetail) && !InlineEditableText->IsInEditMode();
		}
	}

	return false;
}
FText SGraphNode_DialogBuilderNode::GetPinTooltip(UEdGraphPin* GraphPinObj) const
{
	FText HoverText = FText::GetEmpty();

	check(GraphPinObj != nullptr);
	UEdGraphNode* OwningGraphNode = GraphPinObj->GetOwningNode();
	if (OwningGraphNode != nullptr)
	{
		FString HoverStr;
		OwningGraphNode->GetPinHoverText(*GraphPinObj, /*out*/HoverStr);
		if (!HoverStr.IsEmpty())
		{
			HoverText = FText::FromString(HoverStr);
		}
	}

	return HoverText;

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGraphNode_DialogBuilderNode::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	const FString NewNameString = InText.ToString().TrimStartAndEnd();
	const FName NewName = *NewNameString;
	FName OriginalName;

	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;

	if (DialogEdNode && DialogNode)
	{
		OriginalName = DialogNode->ID;

		//Finalize Node and Update GraphNode
		const FScopedTransaction Transaction(LOCTEXT("DialogSystemEditorRenameNode", "Dialog System Editor: Rename Node"));
		DialogEdNode->Modify();
		DialogNode->Modify();
		DialogNode->SetNodeTitle(FText::FromString(NewNameString));
		UpdateGraphNode();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		TArray<FAssetData> DialogGraphDataArray;
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UDialogBuilderGraph::StaticClass()), DialogGraphDataArray);

		for (const FAssetData& AssetData : DialogGraphDataArray)
		{
			UDialogBuilderGraph* DialogGraph = Cast<UDialogBuilderGraph>(AssetData.GetAsset());
			if (DialogGraph)
			{
				
			}
		}
	}
}

FSlateColor SGraphNode_DialogBuilderNode::GetBackgroundColor() const
{
	UDialogBuilderEdNode* DialogEdNode = CastChecked<UDialogBuilderEdNode>(GraphNode);

	FLinearColor NodeColor = DialogBuilderColors::NodeBody::Default;
	if (DialogEdNode && DialogEdNode->HasErrors())
	{
		NodeColor = DialogBuilderColors::NodeBody::Error;
	}
	else if (!DialogEdNode->IsSubNode())
	{
		NodeColor = DialogBuilderColors::NodeBorder::NoHighlight;
	}
	else if (DialogEdNode)
	{
		NodeColor = DialogEdNode->GetBackgroundColor();
	}

	return NodeColor;
}

FSlateColor SGraphNode_DialogBuilderNode::GetBorderBackgroundColor() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderEdNode* DialogParentNode = DialogEdNode ? Cast<UDialogBuilderEdNode>(DialogEdNode->ParentNode) : nullptr;
	const bool bSelectedSubNode = DialogParentNode && GetOwnerPanel()->SelectionManager.SelectedNodes.Contains(GraphNode);


	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;

	return bSelectedSubNode ? DialogBuilderColors::NodeBorder::Selected :
		DialogEdNode && !DialogEdNode->IsSubNode() ? DialogEdNode->GetBackgroundColor() :
		DialogBuilderColors::NodeBorder::Inactive;

}

FSlateColor SGraphNode_DialogBuilderNode::GetSelectorColor() const
{
	return DialogBuilderColors::NodeBorder::Selector;
}

FText SGraphNode_DialogBuilderNode::GetNodeIDText() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
	if (DialogNode)
	{
		return FText::FromString("ID: " + DialogNode->ID.ToString());
	}
	return FText::FromString("ID: None");
}

EVisibility SGraphNode_DialogBuilderNode::GetNodeIDVisibility() const
{
	UDialogBuilderEdNode_Root* RootDialogEdNode = Cast<UDialogBuilderEdNode_Root>(GraphNode);
	if (RootDialogEdNode)
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility SGraphNode_DialogBuilderNode::GetTitleVisibility() const
{
	return EVisibility::Visible;
}

EVisibility SGraphNode_DialogBuilderNode::GetParentNodeVisibility() const
{
	UDialogBuilderEdNode* TestNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility SGraphNode_DialogBuilderNode::GetNodeHeaderTitleVisibility() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);

	bool bVisible = DialogEdNode && (DialogEdNode->IsA(UDialogBuilderEdNode_PlayerChoice::StaticClass()) || DialogEdNode->IsA(UDialogBuilderEdNode_DialogSequence::StaticClass()));
	if (bVisible)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility SGraphNode_DialogBuilderNode::GetSubNodeVisibility() const
{
	UDialogBuilderEdNode* TestNode = Cast<UDialogBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

EVisibility SGraphNode_DialogBuilderNode::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SGraphNode_DialogBuilderNode::IsCurrentEditingSequenceNode() const
{
	const UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	const UDialogBuilderNode_DialogSequence* ThisSequenceNode = DialogEdNode
		? Cast<UDialogBuilderNode_DialogSequence>(DialogEdNode->NodeInstance)
		: nullptr;

	const UDialogBuilderNode* DialogNode = DialogEdNode
		? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance)
		: nullptr;

	const UDialogBuilderGraph* DialogGraph = DialogNode
		? DialogNode->DialogGraph
		: nullptr;

	return ThisSequenceNode
		&& DialogGraph
		&& DialogGraph->CurrentEditingSequenceNode == ThisSequenceNode;
}

EVisibility SGraphNode_DialogBuilderNode::GetDebuggerSearchFailedMarkerVisibility() const
{
	return EVisibility();
}

const FSlateBrush* SGraphNode_DialogBuilderNode::GetNameIcon() const
{
	UDialogBuilderEdSubNode* DialogEdSubNode = Cast<UDialogBuilderEdSubNode>(GraphNode);
	if (DialogEdSubNode != nullptr)
	{
		return FAppStyle::GetBrush(DialogEdSubNode->GetNameIcon());
	}

	return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.DialogNode");
}

void SGraphNode_DialogBuilderNode::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SGraphNode_DialogBuilderNode::GetBlueprintIconVisibility() const
{
	UDialogBuilderEdNode* DialogEdNode = CastChecked<UDialogBuilderEdNode>(GraphNode);
	const bool bCanShowIcon = (DialogEdNode != nullptr && DialogEdNode->UsesBlueprint());

	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (bCanShowIcon && (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail)) ? EVisibility::Visible : EVisibility::Collapsed;

}

EVisibility SGraphNode_DialogBuilderNode::GetSequencerIconVisibility() const
{
	const UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	const UDialogBuilderNode_DialogSequence* DialogSequenceNode = DialogEdNode
		? Cast<UDialogBuilderNode_DialogSequence>(DialogEdNode->NodeInstance)
		: nullptr;

	const bool bCanShowIcon = DialogSequenceNode && DialogSequenceNode->DialogSequence != nullptr;

	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (bCanShowIcon && (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail))
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FReply SGraphNode_DialogBuilderNode::OnOpenSequencerClicked()
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = DialogEdNode
		? Cast<UDialogBuilderNode_DialogSequence>(DialogEdNode->NodeInstance)
		: nullptr;

	if (!DialogSequenceNode || !DialogSequenceNode->DialogSequence)
	{
		return FReply::Unhandled();
	}

	UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(GraphNode ? GraphNode->GetGraph() : nullptr);
	if (DialogEdGraph)
	{
		if (TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEdGraph->DialogEditorPtr.Pin())
		{
			DialogEditor->OnOpenDialogSequenceNode(DialogSequenceNode);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

EVisibility SGraphNode_DialogBuilderNode::GetIndexVisibility() const
{
	// always hide the index on the root node
	if (GraphNode->IsA(UDialogBuilderEdNode_Root::StaticClass()))
	{
		return EVisibility::Collapsed;
	}

	UDialogBuilderEdNode* DialogEdNode = CastChecked<UDialogBuilderEdNode>(GraphNode);
	UEdGraphPin* MyInputPin = DialogEdNode->GetInputPin();
	UEdGraphPin* MyParentOutputPin = NULL;
	if (MyInputPin != NULL && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
	}


	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();

	UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(GraphNode->GetGraph());

	FGraphPanelSelectionSet SelectedNodes;
	if (DialogEdGraph && DialogEdGraph->SEditorGraph)
	{
		SelectedNodes = DialogEdGraph->SEditorGraph->GetSelectedNodes();
	}

	UDialogBuilderNode* FirstSelectedNode = nullptr;
	if (SelectedNodes.Num() > 0)
	{
		// Get the first element of the SelectedNodes set
		UDialogBuilderEdNode* SelectedDialogEdNode = Cast<UDialogBuilderEdNode>(*SelectedNodes.CreateConstIterator());
		FirstSelectedNode = SelectedDialogEdNode ? Cast<UDialogBuilderNode>(SelectedDialogEdNode->NodeInstance) : nullptr;
	}

	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance);

	// Visible if ((we are in PIE || if we have siblings) && (SelectedNodes only one && Matching parent with selected nodes))
	const bool bCanShowIndex = (ShouldShowExecutionIndex() || (MyParentOutputPin && MyParentOutputPin->LinkedTo.Num() > 1)) &&
		(SelectedNodes.Num() == 1 && (FirstSelectedNode && DialogNode) &&
			(DialogNode->ParentNodes.Contains(FirstSelectedNode) || DialogNode == FirstSelectedNode));
	return (bCanShowIndex && (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail)) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SGraphNode_DialogBuilderNode::GetIndexText() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);
	UEdGraphPin* MyInputPin = DialogEdNode->GetInputPin();
	UEdGraphPin* MyParentOutputPin = NULL;
	if (MyInputPin != NULL && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
	}

	int32 Index = 0;

	UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(GraphNode->GetGraph());

	FGraphPanelSelectionSet SelectedNodes;
	if (DialogEdGraph && DialogEdGraph->SEditorGraph)
	{
		SelectedNodes = DialogEdGraph->SEditorGraph->GetSelectedNodes();
	}

	UDialogBuilderNode* FirstSelectedNode = nullptr;
	if (SelectedNodes.Num() > 0)
	{
		// Get the first element of the SelectedNodes set
		UDialogBuilderEdNode* SelectedDialogEdNode = Cast<UDialogBuilderEdNode>(*SelectedNodes.CreateConstIterator());
		FirstSelectedNode = SelectedDialogEdNode ? Cast<UDialogBuilderNode>(SelectedDialogEdNode->NodeInstance) : nullptr;
	}

	UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance);

	if (DialogNode->ParentNodes.Contains(FirstSelectedNode))
	{
		for (int i = 0; i < FirstSelectedNode->ChildrenNodes.Num(); i++)
		{
			//Select this index if node matches with the selected node childrens
			if (FirstSelectedNode->ChildrenNodes[i] == DialogNode)
			{
				Index = i + 1;//Index start from 1
				break;
			}
		}
	}


	return FText::AsNumber(Index);
}

FText SGraphNode_DialogBuilderNode::GetNodeHeaderTitleText() const
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(GraphNode);

	bool bIsSelector = DialogEdNode && DialogEdNode->IsA(UDialogBuilderEdNode_PlayerChoice::StaticClass());
	bool bIsSequence = DialogEdNode && DialogEdNode->IsA(UDialogBuilderEdNode_DialogSequence::StaticClass());
	FText HeaderTitle = FText::GetEmpty();
	if (bIsSelector)
	{
		HeaderTitle = LOCTEXT("PlayerChoiceNodeHeader", "SELECTOR");
	}
	else if (bIsSequence)
	{
		HeaderTitle = LOCTEXT("DialogSequenceNodeHeader", "SEQUENCE");
	}


	return HeaderTitle;
}

FText SGraphNode_DialogBuilderNode::GetIndexTooltipText() const
{
	if (ShouldShowExecutionIndex())
	{
		return LOCTEXT("ExecutionIndexTooltip", "Execution index: this shows the order in which nodes are executed.");
	}
	else
	{
		return LOCTEXT("ChildIndexTooltip", "Child index: this shows the order in which child nodes are executed.");
	}
}

FSlateColor SGraphNode_DialogBuilderNode::GetIndexColor(bool bHovered) const
{
	const bool bHighlightHover = bHovered;

	static const FName HoveredColor("BTEditor.Graph.BTNode.Index.HoveredColor");
	static const FName DefaultColor("BTEditor.Graph.BTNode.Index.Color");

	return FAppStyle::Get().GetSlateColor(HoveredColor);
	//return bHighlightHover ? FAppStyle::Get().GetSlateColor(HoveredColor) : FAppStyle::Get().GetSlateColor(DefaultColor);
}

void SGraphNode_DialogBuilderNode::OnIndexHoverStateChanged(bool bHovered)
{
}

TSharedRef<FDragDialogGraphNode> FDragDialogGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragDialogGraphNode> Operation = MakeShareable(new FDragDialogGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragDialogGraphNode> FDragDialogGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray<TSharedRef<SGraphNode>>& InDraggedNodes)
{
	TSharedRef<FDragDialogGraphNode> Operation = MakeShareable(new FDragDialogGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UDialogBuilderEdNode* FDragDialogGraphNode::GetDropTargetNode() const
{
	return Cast<UDialogBuilderEdNode>(GetHoveredNode());
}


#undef LOCTEXT_NAMESPACE

