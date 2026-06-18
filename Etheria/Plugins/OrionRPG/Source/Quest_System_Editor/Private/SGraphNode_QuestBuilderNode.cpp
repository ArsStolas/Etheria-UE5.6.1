// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "SGraphNode_QuestBuilderNode.h"
#include "SlateOptMacros.h"
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
#include "Colors_QuestBuilder.h"
#include "GraphEditorSettings.h"
#include "SCommentBubble.h"
#include "SLevelOfDetailBranchNode.h"
#include "Quest.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderEdNode_Root.h"
#include "QuestBuilderEdNode_Objective.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderSetting.h"	
#include "Fonts/SlateFontInfo.h"
#include "Decorator/OrionDecorator.h"
#include "Event/OrionEvent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "QuestBuilderEdSubNode_Decorator.h"
#include "QuestBuilderEdSubNode_Event.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "EdNode_QuestBuilder"

#define ALWAYS_SHOW_QUEST_EXECUTION_INDEX 1

bool ShouldShowExecutionIndex()
{
#ifdef ALWAYS_SHOW_QUEST_EXECUTION_INDEX
	return true;
#else
	return GEditor && (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != NULL);
#endif // ALWAYS_SHOW_QUEST_EXECUTION_INDEX
}

//////////////////////////////////////////////////////////////////////////
class SQuestSystemPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SQuestSystemPin) {}
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
			.BorderImage(this, &SQuestSystemPin::GetPinIcon)
			.BorderBackgroundColor(this, &SQuestSystemPin::GetPinColor)
			.OnMouseButtonDown(this, &SQuestSystemPin::OnPinMouseDown)
			.Cursor(this, &SQuestSystemPin::GetPinCursor)
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
		return bIsDiffHighlighted ? QuestBuilderColors::Pin::Diff :
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
class SQuestNodeIndex : public SCompoundWidget
{
public:
	/** Delegate event fired when the hover state of this widget changes */
	DECLARE_DELEGATE_OneParam(FOnHoverStateChanged, bool /* bHovered */);

	/** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
	DECLARE_DELEGATE_RetVal_OneParam(FSlateColor, FOnGetIndexColor, bool /* bHovered */);

	SLATE_BEGIN_ARGS(SQuestNodeIndex) {}
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
							.BorderBackgroundColor(this, &SQuestNodeIndex::GetColor)
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


void SGraphNode_QuestBuilderNode::Construct(const FArguments& InArgs, UQuestBuilderEdNode* InNode)
{
	SetCursor(EMouseCursor::CardinalCross);
	GraphNode = InNode;
	UpdateGraphNode();
	InNode->SGraphNode = this;
	bDragMarkerVisible = false;
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGraphNode_QuestBuilderNode::UpdateGraphNode()
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

	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;

	if (QuestEdNode)
	{
		for (int32 i = 0; i < QuestEdNode->Decorators.Num(); i++)
		{
			if (QuestEdNode->Decorators.IsValidIndex(i))
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(QuestEdNode->Decorators[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				AddDecorator(NewNode);
				NewNode->UpdateGraphNode();
			}
		}
		for (int32 i = 0; i < QuestEdNode->Events.Num(); i++)
		{
			if (QuestEdNode->Events.IsValidIndex(i))
			{
				TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(QuestEdNode->Events[i]);
				if (OwnerGraphPanelPtr.IsValid())
				{
					NewNode->SetOwner(OwnerGraphPanelPtr.Pin().ToSharedRef());
					OwnerGraphPanelPtr.Pin()->AttachGraphEvents(NewNode);
				}
				UOrionEvent* Event = QuestEdNode->Events[i] ? Cast<UOrionEvent>(QuestEdNode->Events[i]->NodeInstance) : nullptr;
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

	const FMargin NodePadding = (Cast<UQuestBuilderEdSubNode_Decorator>(GraphNode) || Cast<UQuestBuilderEdSubNode_Event>(GraphNode))
		? FMargin( 1.f)
		: FMargin(8.0f);

	const float NodeMaxWidth = QuestEdNode && QuestEdNode->IsSubNode()
		? 425.f
		: 450.f;

	const float NodeMinWidth = QuestEdNode && QuestEdNode->IsSubNode()
		? 200.f
		: 125.f;

	const FMargin PinPadding = (Cast<UQuestBuilderEdSubNode_Decorator>(GraphNode) || Cast<UQuestBuilderEdSubNode_Event>(GraphNode))
		? FMargin(0.f)
		: FMargin(5.f, 3.f, 5.f, 3.f);
	

	UWorld* World = GEditor->GetEditorWorldContext().World();

	IndexOverlay = SNew(SQuestNodeIndex)
		.ToolTipText(this, &SGraphNode_QuestBuilderNode::GetIndexTooltipText)
		.Visibility(this, &SGraphNode_QuestBuilderNode::GetIndexVisibility)
		.Text(this, &SGraphNode_QuestBuilderNode::GetIndexText)
		.OnHoverStateChanged(this, &SGraphNode_QuestBuilderNode::OnIndexHoverStateChanged)
		.OnGetIndexColor(this, &SGraphNode_QuestBuilderNode::GetIndexColor);
	

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0.0f)
			.BorderBackgroundColor(this, &SGraphNode_QuestBuilderNode::GetBorderBackgroundColor)
			.OnMouseButtonDown(this, &SGraphNode_QuestBuilderNode::OnMouseDown)
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
							.AutoWidth()
							.Padding(PinPadding)
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
										.BorderBackgroundColor(this, &SGraphNode_QuestBuilderNode::GetBackgroundColor)
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
													.Visibility(this, &SGraphNode_QuestBuilderNode::GetTitleVisibility)
													+ SHorizontalBox::Slot()
													.AutoWidth()
													[
														// POPUP ERROR MESSAGE
														SAssignNew(ErrorText, SErrorText)
															.BackgroundColor(this, &SGraphNode_QuestBuilderNode::GetErrorColor)
															.ToolTipText(this, &SGraphNode_QuestBuilderNode::GetErrorMsgToolTip)
													]
													+ SHorizontalBox::Slot()
													.AutoWidth()
													[
															SNew(SHorizontalBox)
															+ SHorizontalBox::Slot()
															.AutoWidth()
															.VAlign(VAlign_Top)
															.Padding(FMargin(0.0f, 5.0f, 0.0f, 0.0f))
															[
																SNew(SImage)
																	.Image(this, &SGraphNode_QuestBuilderNode::GetNameIcon)
																	//.Visibility(this, &SGraphNode_QuestBuilderNode::GetParentNodeVisibility)
															]
																//if parent node
															+ SHorizontalBox::Slot()
															//.MinWidth(NodeMinWidth)
															.MaxWidth(NodeMaxWidth)
															.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
															[
																SNew(SVerticalBox)
																.Visibility(this, &SGraphNode_QuestBuilderNode::GetParentNodeVisibility)
																+ SVerticalBox::Slot()
																.AutoHeight()
																[
																	SAssignNew(InlineEditableText, SInlineEditableTextBlock)
																		.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
																		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
																		.OnVerifyTextChanged(this, &SGraphNode_QuestBuilderNode::OnVerifyNameTextChanged)
																		.OnTextCommitted(this, &SGraphNode_QuestBuilderNode::OnNameTextCommited)
																		.IsReadOnly(this, &SGraphNode_QuestBuilderNode::IsNameReadOnly)
																		.IsSelected(this, &SGraphNode_QuestBuilderNode::IsSelectedExclusively)
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
																		.Visibility(this, &SGraphNode_QuestBuilderNode::GetNodeTagVisibility)
																		.Text(this, &SGraphNode_QuestBuilderNode::GetNodeTagDescription)
																		.TextStyle(FAppStyle::Get(), TEXT("RichTextBlock.Italic"))
																		.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
																]
																+ SVerticalBox::Slot()
																.AutoHeight()
																.Padding(FMargin(2.0f, 1.0f, 2.0f, 4.0f))
																[
																	// DESCRIPTION MESSAGE
																	SAssignNew(DescriptionText, STextBlock)
																		.Visibility(this, &SGraphNode_QuestBuilderNode::GetDescriptionVisibility)
																		.Text(this, &SGraphNode_QuestBuilderNode::GetDescription)
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
																.Visibility(this, &SGraphNode_QuestBuilderNode::GetSubNodeVisibility)
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
																				.Visibility(this, &SGraphNode_QuestBuilderNode::GetDescriptionVisibility)
																				.Text(this, &SGraphNode_QuestBuilderNode::GetDescription)
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
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SAssignNew(NodeBody, SBorder)
											.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
											.BorderBackgroundColor(QuestBuilderColors::NodeBorder::SubNodeBorder)
											.HAlign(HAlign_Fill)
											.VAlign(VAlign_Center)
											.Visibility(this, &SGraphNode_QuestBuilderNode::GetEventsVisibility)
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
																	+ SHorizontalBox::Slot()
																	.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
																	[
																		SNew(STextBlock)
																			.Text(LOCTEXT("TaskLabel", "Events"))
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
																		.BorderBackgroundColor(QuestBuilderColors::NodeBorder::SubNodeBorder)
																		.Visibility(this, &SGraphNode_QuestBuilderNode::GetStartEventsVisibility)
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
																		.BorderBackgroundColor(QuestBuilderColors::NodeBorder::SubNodeBorder)
																		.Visibility(this, &SGraphNode_QuestBuilderNode::GetEndEventsVisibility)
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
																		.BorderBackgroundColor(QuestBuilderColors::NodeBorder::SubNodeBorder)
																		.Visibility(this, &SGraphNode_QuestBuilderNode::GetBothEventsVisibility)
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
											.BorderBackgroundColor(QuestBuilderColors::NodeBorder::SubNodeBorder)
											.HAlign(HAlign_Fill)
											.VAlign(VAlign_Center)
											.Visibility(this, &SGraphNode_QuestBuilderNode::GetDecoratorVisibility)
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
																	+ SHorizontalBox::Slot()
																	.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
																	[
																		SNew(STextBlock)
																			.Text(LOCTEXT("TaskLabel", "Conditions"))
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
							.AutoWidth()
							.Padding(PinPadding)
							[
								SNew(SBox)
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
							.BorderBackgroundColor(QuestBuilderColors::Action::DragMarker)
							.ColorAndOpacity(QuestBuilderColors::Action::DragMarker)
							.BorderImage(FAppStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
							.Visibility(this, &SGraphNode_QuestBuilderNode::GetDragOverMarkerVisibility)
							[
								SNew(SBox)
									.HeightOverride(4.f)
							]
					]

					// Blueprint indicator overlay
					+SOverlay::Slot()
					.Padding(FMargin(10.f, 0.f))
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Blueprint")))
							.Visibility(this, &SGraphNode_QuestBuilderNode::GetBlueprintIconVisibility)
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

void SGraphNode_QuestBuilderNode::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));

		UQuestBuilderEdNode* TestNode = Cast<UQuestBuilderEdNode>(GraphNode);
		if (DragConnectionOp->IsValidOperation() && TestNode && TestNode->IsSubNode())
		{
			SetDragMarker(true);
		}
	}

	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SGraphNode_QuestBuilderNode::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
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

void SGraphNode_QuestBuilderNode::OnDragLeave(const FDragDropEvent& DragDropEvent)
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

FReply SGraphNode_QuestBuilderNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetDragMarker(false);

	TSharedPtr<FDragQuestGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragQuestGraphNode>();
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

		UQuestBuilderEdNode* MyNode = Cast<UQuestBuilderEdNode>(GraphNode);
		if (MyNode == nullptr || MyNode->IsSubNode())
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_DragDropNode", "Drag&Drop Node"));
		bool bReorderOperation = true;

		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UQuestBuilderEdNode* DraggedNode = Cast<UQuestBuilderEdNode>(DraggedNodes[Idx]->GetNodeObj());
			if (DraggedNode && DraggedNode->ParentNode)
			{
				if (DraggedNode->ParentNode != GraphNode)
				{
					bReorderOperation = false;
				}

				DraggedNode->ParentNode->RemoveSubNode(DraggedNode);
			}
		}

		UQuestBuilderEdNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		const int32 InsertIndex = MyNode->FindSubNodeDropIndex(DropTargetNode);
		const EEventLaunchType DroppedEventLaunchType = GetEventLaunchTypeForDrop(MyGeometry, DragDropEvent);

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UQuestBuilderEdNode* DraggedTestNode = Cast<UQuestBuilderEdNode>(DraggedNodes[Idx]->GetNodeObj());
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
			UQuestBuilderEdGraph* MyGraph = MyNode->GetQuestBuilderEdGraph();
			if (MyGraph)
			{
				MyGraph->OnSubNodeDropped();
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}


FReply SGraphNode_QuestBuilderNode::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		//if we are holding mouse over a subnode
		UQuestBuilderEdNode* TestNode = Cast<UQuestBuilderEdNode>(GraphNode);
		if (TestNode && TestNode->IsSubNode())
		{
			const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragQuestGraphNode::New(Panel, Node));
		}
	}

	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bDragMarkerVisible)
	{
		SetDragMarker(false);
	}

	return FReply::Unhandled();
}

TSharedRef<SGraphNode> SGraphNode_QuestBuilderNode::GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, MouseEvent);
	return SubNode.IsValid() ? SubNode.ToSharedRef() : StaticCastSharedRef<SGraphNode>(AsShared());
}


#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
void SGraphNode_QuestBuilderNode::MoveTo(const FVector2f& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
void SGraphNode_QuestBuilderNode::MoveTo(const FVector2D & NewPosition, FNodeSet & NodeFilter, bool bMarkDirty)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
#endif
	// keep node order (defined by linked pins) up to date with actual positions
	// this function will keep spamming on every mouse move update
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (QuestEdNode && !QuestEdNode->IsSubNode())
	{
		UQuestBuilderEdGraph* QuestEdGraph = QuestEdNode->GetQuestBuilderEdGraph();
		if (QuestEdGraph)
		{
			for (int32 Idx = 0; Idx < QuestEdNode->Pins.Num(); Idx++)
			{
				UEdGraphPin* Pin = QuestEdNode->Pins[Idx];
				if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 1)
				{
					UEdGraphPin* ParentPin = Pin->LinkedTo[0];
					if (ParentPin)
					{
						QuestEdGraph->RebuildChildOrder(ParentPin->GetOwningNode());
					}
				}
			}
		}
	}
}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
TArray<FOverlayWidgetInfo> SGraphNode_QuestBuilderNode::GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
TArray<FOverlayWidgetInfo> SGraphNode_QuestBuilderNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
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

void SGraphNode_QuestBuilderNode::CreatePinWidgets()
{
	UQuestBuilderEdNode* StateNode = CastChecked<UQuestBuilderEdNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SQuestSystemPin, MyPin)
				.ToolTipText(this, &SGraphNode_QuestBuilderNode::GetPinTooltip, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

TSharedPtr<SToolTip> SGraphNode_QuestBuilderNode::GetComplexTooltip()
{
	const UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	const bool bHasErrors = QuestEdNode && QuestEdNode->HasErrors();

	if (!bHasErrors)
	{
	}
	return IDocumentation::Get()->CreateToolTip(TAttribute<FText>(this, &SGraphNode::GetNodeTooltip), NULL, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName());

}

void SGraphNode_QuestBuilderNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
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
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		PinBox = RightNodeBox;
		OutputPins.Add(PinToAdd);
	}

	if (PinBox)
	{
		PinBox->AddSlot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				PinToAdd
			];
	}
}

void SGraphNode_QuestBuilderNode::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
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

bool SGraphNode_QuestBuilderNode::IsNameReadOnly() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;

	UQuestBuilderGraph* QuestGraph = QuestNode ? Cast<UQuestBuilderGraph>(QuestNode->GetOwningQuestGraph()) : nullptr;

	return (QuestEdNode && QuestNode && QuestGraph) &&
		(!GetDefault<UQuestBuilderSetting>()->bCanRenameNode || !QuestNode->IsNameEditable()) || SGraphNode::IsNameReadOnly();
}
FReply SGraphNode_QuestBuilderNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	UQuestBuilderEdNode* TestNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
FReply SGraphNode_QuestBuilderNode::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}
FText SGraphNode_QuestBuilderNode::GetDescription() const
{
	UQuestBuilderEdNode* QuestEdNode = CastChecked<UQuestBuilderEdNode>(GraphNode);
	return QuestEdNode ? QuestEdNode->GetDescription() : FText::GetEmpty();
}
FText SGraphNode_QuestBuilderNode::GetNodeTagDescription() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
	if (QuestNode)
	{
		return FText::FromString("Tag: " + QuestNode->NodeTag.GetTagName().ToString());
	}
	return FText::FromString("Tag: None");
}
EVisibility SGraphNode_QuestBuilderNode::GetDescriptionVisibility() const
{
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;

}
EVisibility SGraphNode_QuestBuilderNode::GetNodeTagVisibility() const
{
	UQuestBuilderEdNode_Root* RootQuestEdNode = Cast<UQuestBuilderEdNode_Root>(GraphNode);
	if (RootQuestEdNode)
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility SGraphNode_QuestBuilderNode::GetEventsVisibility() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (QuestEdNode && QuestEdNode->Events.Num())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

EVisibility SGraphNode_QuestBuilderNode::GetLaunchTypeEventsVisibility(EEventLaunchType LaunchType) const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
	
	if (QuestNode)
	{
		for (auto& Event : QuestNode->Events)
		{
			if (Event->EventLaunchType == LaunchType)
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}

EVisibility SGraphNode_QuestBuilderNode::GetStartEventsVisibility() const
{
	
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_Start);
}

EVisibility SGraphNode_QuestBuilderNode::GetEndEventsVisibility() const
{
	
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_End);
}

EVisibility SGraphNode_QuestBuilderNode::GetBothEventsVisibility() const
{
	return GetLaunchTypeEventsVisibility(EEventLaunchType::E_Both);
}

EVisibility SGraphNode_QuestBuilderNode::GetDecoratorVisibility() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (QuestEdNode && QuestEdNode->Decorators.Num())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

void SGraphNode_QuestBuilderNode::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SGraphNode_QuestBuilderNode::AddDecorator(TSharedPtr<SGraphNode> DecoratorWidget)
{
	DecoratorsBox->AddSlot()
		.AutoHeight()
		[
			DecoratorWidget.ToSharedRef()
		];

	DecoratorWidgets.Add(DecoratorWidget);
	AddSubNode(DecoratorWidget);
}

void SGraphNode_QuestBuilderNode::AddEvent(TSharedPtr<SGraphNode> EventWidget)
{
	UQuestBuilderEdNode* EventNode = EventWidget.IsValid() ? Cast<UQuestBuilderEdNode>(EventWidget->GetNodeObj()) : nullptr;
	UOrionEvent* Event = EventNode ? Cast<UOrionEvent>(EventNode->NodeInstance) : nullptr;
	AddEvent(EventWidget, Event ? Event->EventLaunchType : EEventLaunchType::E_Start);
}

void SGraphNode_QuestBuilderNode::AddEvent(TSharedPtr<SGraphNode> EventWidget, EEventLaunchType EventLaunchType)
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

EEventLaunchType SGraphNode_QuestBuilderNode::GetEventLaunchTypeForDrop(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
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
			if (HoveredWidget == EndEventsBox.ToSharedRef())
			{
				return EEventLaunchType::E_End;
			}
			if (HoveredWidget == BothEventsBox.ToSharedRef())
			{
				return EEventLaunchType::E_Both;
			}
		}
	}

	return EEventLaunchType::E_Start;
}

void SGraphNode_QuestBuilderNode::SetEventLaunchType(UQuestBuilderEdNode* EventNode, EEventLaunchType EventLaunchType)
{
	UOrionEvent* Event = EventNode ? Cast<UOrionEvent>(EventNode->NodeInstance) : nullptr;
	if (Event && Event->EventLaunchType != EventLaunchType)
	{
		Event->Modify();
		Event->EventLaunchType = EventLaunchType;
	}
}

void SGraphNode_QuestBuilderNode::AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);
}

TSharedPtr<SGraphNode> SGraphNode_QuestBuilderNode::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
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

bool SGraphNode_QuestBuilderNode::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	FString FinalString = InText.ToString().TrimStartAndEnd();

	FName OriginalName;

	bool bValid(true);

	if ((GetEditableNodeTitle() != FinalString) && OnVerifyTextCommit.IsBound())
	{
		OutErrorMessage = LOCTEXT("RenameFailed_NotValid", "Name Not Valid.");
        bValid = OnVerifyTextCommit.Execute(FText::FromString(FinalString), GraphNode, OutErrorMessage);
	}

	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
	UQuestBuilderEdGraph* QuestEdGraph = QuestEdNode ? QuestEdNode->GetQuestBuilderEdGraph() : nullptr;

	if (QuestNode && QuestEdGraph)
	{
		OriginalName = QuestNode->ID;
		for (auto& Node : QuestEdGraph->Nodes)
		{
			UQuestBuilderEdNode* NodeEdNode = Cast<UQuestBuilderEdNode>(Node);
			if (NodeEdNode && NodeEdNode->NodeInstance)
			{
				UQuestBuilderNode* NodeInstance = Cast<UQuestBuilderNode>(NodeEdNode->NodeInstance);
				if (NodeInstance && NodeInstance != QuestNode)
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
bool SGraphNode_QuestBuilderNode::UseLowDetailNodeTitles() const
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
FText SGraphNode_QuestBuilderNode::GetPinTooltip(UEdGraphPin* GraphPinObj) const
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

void SGraphNode_QuestBuilderNode::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	const FString NewNameString = InText.ToString().TrimStartAndEnd();
	const FName NewName = *NewNameString;
	FName OriginalName;

	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
	UQuest* OwningQuest = QuestEdNode ? QuestEdNode->GetQuestBuilderEdGraph()->Quest : nullptr;

	if (QuestEdNode && QuestNode)
	{
		OriginalName = QuestNode->ID;

		//Finalize Node and Update GraphNode
		const FScopedTransaction Transaction(LOCTEXT("QuestSystemEditorRenameNode", "Quest System Editor: Rename Node"));
		QuestEdNode->Modify();
		QuestNode->Modify();
		QuestNode->SetNodeTitle(FText::FromString(NewNameString));
		UpdateGraphNode();
		
	}
}

FSlateColor SGraphNode_QuestBuilderNode::GetBackgroundColor() const
{
	UQuestBuilderEdNode* QuestEdNode = CastChecked<UQuestBuilderEdNode>(GraphNode);

	FLinearColor NodeColor = QuestBuilderColors::NodeBody::Default;
	if (QuestEdNode && QuestEdNode->HasErrors())
	{
		NodeColor = QuestBuilderColors::NodeBody::Error;
	}
	else if (!QuestEdNode->IsSubNode())
	{
		NodeColor = QuestBuilderColors::NodeBorder::NoHighlight;
	}
	else if (QuestEdNode)
	{
		NodeColor = QuestEdNode->GetBackgroundColor();
	}

	return NodeColor;
}

FSlateColor SGraphNode_QuestBuilderNode::GetBorderBackgroundColor() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UQuestBuilderEdNode* QuestParentNode = QuestEdNode ? Cast<UQuestBuilderEdNode>(QuestEdNode->ParentNode) : nullptr;
	const bool bSelectedSubNode = QuestParentNode && GetOwnerPanel()->SelectionManager.SelectedNodes.Contains(GraphNode);


	UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;

	return bSelectedSubNode ? QuestBuilderColors::NodeBorder::Selected :
		QuestEdNode && !QuestEdNode->IsSubNode() ? QuestEdNode->GetBackgroundColor() :
		QuestBuilderColors::NodeBorder::Inactive;

}

EVisibility SGraphNode_QuestBuilderNode::GetTitleVisibility() const
{
	return EVisibility::Visible;
}

EVisibility SGraphNode_QuestBuilderNode::GetParentNodeVisibility() const
{
	UQuestBuilderEdNode* TestNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

EVisibility SGraphNode_QuestBuilderNode::GetSubNodeVisibility() const
{
	UQuestBuilderEdNode* TestNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

EVisibility SGraphNode_QuestBuilderNode::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGraphNode_QuestBuilderNode::GetDebuggerSearchFailedMarkerVisibility() const
{
	return EVisibility();
}

const FSlateBrush* SGraphNode_QuestBuilderNode::GetNameIcon() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	if (QuestEdNode != nullptr)
	{
		return FAppStyle::GetBrush(QuestEdNode->GetNameIcon());
	}
	return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

void SGraphNode_QuestBuilderNode::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SGraphNode_QuestBuilderNode::GetBlueprintIconVisibility() const
{
	UQuestBuilderEdNode* QuestEdNode = CastChecked<UQuestBuilderEdNode>(GraphNode);
	const bool bCanShowIcon = (QuestEdNode != nullptr && QuestEdNode->UsesBlueprint());

	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (bCanShowIcon && (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail)) ? EVisibility::Visible : EVisibility::Collapsed;

}

EVisibility SGraphNode_QuestBuilderNode::GetIndexVisibility() const
{
	// always hide the index on the root node
	if (GraphNode->IsA(UQuestBuilderEdNode_Root::StaticClass()))
	{
		return EVisibility::Collapsed;
	}

	UQuestBuilderEdNode* QuestEdNode = CastChecked<UQuestBuilderEdNode>(GraphNode);
	UEdGraphPin* MyInputPin = QuestEdNode->GetInputPin();
	UEdGraphPin* MyParentOutputPin = NULL;
	if (MyInputPin != NULL && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
	}

	
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();

	UQuestBuilderEdGraph* QuestEdGraph = Cast<UQuestBuilderEdGraph>(GraphNode->GetGraph());

	FGraphPanelSelectionSet SelectedNodes;
	if (QuestEdGraph && QuestEdGraph->SEditorGraph)
	{
		SelectedNodes = QuestEdGraph->SEditorGraph->GetSelectedNodes();
	}

	UQuestBuilderNode* FirstSelectedNode = nullptr;
	if (SelectedNodes.Num() > 0)
	{
		// Get the first element of the SelectedNodes set
		UQuestBuilderEdNode* SelectedQuestEdNode = Cast<UQuestBuilderEdNode>(*SelectedNodes.CreateConstIterator());
		FirstSelectedNode = SelectedQuestEdNode ? Cast<UQuestBuilderNode>(SelectedQuestEdNode->NodeInstance) : nullptr;
	}
	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance);

	// Visible if ((we are in PIE || if we have siblings) && (SelectedNodes only one && Matching parent with selected nodes))
	const bool bCanShowIndex = (ShouldShowExecutionIndex() || (MyParentOutputPin && MyParentOutputPin->LinkedTo.Num() > 1)) &&
		(SelectedNodes.Num() == 1 && (FirstSelectedNode && QuestNode) &&
		(QuestNode->ParentNodes.Contains(FirstSelectedNode) || QuestNode == FirstSelectedNode));

	return (bCanShowIndex && (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail)) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SGraphNode_QuestBuilderNode::GetIndexText() const
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(GraphNode);
	UEdGraphPin* MyInputPin = QuestEdNode->GetInputPin();
	UEdGraphPin* MyParentOutputPin = NULL;
	if (MyInputPin != NULL && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
	}

	int32 Index = 0;

	UQuestBuilderEdGraph* QuestEdGraph = Cast<UQuestBuilderEdGraph>(GraphNode->GetGraph());

	FGraphPanelSelectionSet SelectedNodes;
	if (QuestEdGraph && QuestEdGraph->SEditorGraph)
	{
		SelectedNodes = QuestEdGraph->SEditorGraph->GetSelectedNodes();
	}

	UQuestBuilderNode* FirstSelectedNode = nullptr;
	if (SelectedNodes.Num() > 0)
	{
		// Get the first element of the SelectedNodes set
		UQuestBuilderEdNode* SelectedQuestEdNode = Cast<UQuestBuilderEdNode>(*SelectedNodes.CreateConstIterator());
		FirstSelectedNode = SelectedQuestEdNode ? Cast<UQuestBuilderNode>(SelectedQuestEdNode->NodeInstance) : nullptr;
	}

	UQuestBuilderNode* QuestNode = Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance);

	if (QuestNode->ParentNodes.Contains(FirstSelectedNode))
	{
		for (int i = 0; i < FirstSelectedNode->ChildrenNodes.Num(); i++)
		{
			//Select this index if node matches with the selected node childrens
			if (FirstSelectedNode->ChildrenNodes[i] == QuestNode)
			{
				Index = i + 1;//Index start from 1
				break;
			}
		}
	}
	
	
	return FText::AsNumber(Index);
}

FText SGraphNode_QuestBuilderNode::GetIndexTooltipText() const
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

FSlateColor SGraphNode_QuestBuilderNode::GetIndexColor(bool bHovered) const
{
	const bool bHighlightHover = bHovered;

	static const FName HoveredColor("BTEditor.Graph.BTNode.Index.HoveredColor");
	static const FName DefaultColor("BTEditor.Graph.BTNode.Index.Color");

	return FAppStyle::Get().GetSlateColor(HoveredColor);
	//return bHighlightHover ? FAppStyle::Get().GetSlateColor(HoveredColor) : FAppStyle::Get().GetSlateColor(DefaultColor);
}

void SGraphNode_QuestBuilderNode::OnIndexHoverStateChanged(bool bHovered)
{
}

TSharedRef<FDragQuestGraphNode> FDragQuestGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragQuestGraphNode> Operation = MakeShareable(new FDragQuestGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragQuestGraphNode> FDragQuestGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray<TSharedRef<SGraphNode>>& InDraggedNodes)
{
	TSharedRef<FDragQuestGraphNode> Operation = MakeShareable(new FDragQuestGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UQuestBuilderEdNode* FDragQuestGraphNode::GetDropTargetNode() const
{
	return Cast<UQuestBuilderEdNode>(GetHoveredNode());
}


#undef LOCTEXT_NAMESPACE

