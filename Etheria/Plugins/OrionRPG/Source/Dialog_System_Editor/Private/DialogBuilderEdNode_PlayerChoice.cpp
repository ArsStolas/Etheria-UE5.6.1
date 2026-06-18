// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderEdNode_PlayerChoice.h"
#include "DialogBuilderSetting.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogStage.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "EdGraph/EdGraphPin.h"

namespace
{
	static UEdGraphPin* FindPinByNameAndDirection(const TArray<UEdGraphPin*>& Pins, EEdGraphPinDirection Direction, const FName PinName)
	{
		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin && Pin->Direction == Direction && Pin->PinName == PinName)
			{
				return Pin;
			}
		}

		return nullptr;
	}
}

UDialogBuilderEdNode_PlayerChoice::UDialogBuilderEdNode_PlayerChoice()
{
}

void UDialogBuilderEdNode_PlayerChoice::PostPasteNode()
{
	Super::PostPasteNode();
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode)
	{
		if (UDialogBuilderGraph* DialogGraph = DialogSequenceNode->GetOwningDialogGraph())
		{
			for (auto& DialogStage : DialogGraph->DialogStages)
			{
				UDialogStage* CurrentTemplateStage = DialogSequenceNode->DialogStageToUse;
				if (DialogStage && CurrentTemplateStage)
				{
					if (CurrentTemplateStage->Name.ToString() == DialogStage->Name.ToString())
					{
						DialogSequenceNode->DialogStageToUse = DialogStage;
						break;
					}
				}
			}
		}
	}
	GetGraph()->NotifyGraphChanged();
}

void UDialogBuilderEdNode_PlayerChoice::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode)
	{
		DialogSequenceNode->EnsureSequenceCreated();
	}
	GetGraph()->NotifyGraphChanged();

	BindChoiceChangeDelegate();
}

void UDialogBuilderEdNode_PlayerChoice::PostLoad()
{
	Super::PostLoad();
	BindChoiceChangeDelegate();
}

void UDialogBuilderEdNode_PlayerChoice::PostEditUndo()
{
	Super::PostEditUndo();
	BindChoiceChangeDelegate();
}

void UDialogBuilderEdNode_PlayerChoice::PostEditImport()
{
	Super::PostEditImport();
	BindChoiceChangeDelegate();
}

void UDialogBuilderEdNode_PlayerChoice::DestroyNode()
{
	UnbindChoiceChangeDelegate();
	Super::DestroyNode();
}

void UDialogBuilderEdNode_PlayerChoice::BindChoiceChangeDelegate()
{
	if (UDialogBuilderNode_PlayerChoice* PlayerChoiceNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance))
	{
		PlayerChoiceNode->OnPlayerChoiceDataChanged.RemoveAll(this);
		PlayerChoiceNode->OnPlayerChoiceDataChanged.AddUObject(this, &UDialogBuilderEdNode_PlayerChoice::HandleChoiceDataChanged);
	}
}

void UDialogBuilderEdNode_PlayerChoice::UnbindChoiceChangeDelegate()
{
	if (UDialogBuilderNode_PlayerChoice* PlayerChoiceNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance))
	{
		PlayerChoiceNode->OnPlayerChoiceDataChanged.RemoveAll(this);
	}
}

void UDialogBuilderEdNode_PlayerChoice::HandleChoiceDataChanged()
{
	AllocateDefaultPins();
	ReconstructNode();

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}

	if (UDialogBuilderEdGraph* DialogEdGraph = GetDialogBuilderEdGraph())
	{
		DialogEdGraph->UpdateAsset(true);
	}
}

void UDialogBuilderEdNode_PlayerChoice::AllocateDefaultPins()
{
	const UDialogBuilderNode_PlayerChoice* PlayerChoiceNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance);
	const int32 ChoiceCount = PlayerChoiceNode ? FMath::Max(1, PlayerChoiceNode->ChoiceList.Num()) : 1;

	UEdGraphPin* InputPin = nullptr;

	for (int32 PinIdx = Pins.Num() - 1; PinIdx >= 0; --PinIdx)
	{
		UEdGraphPin* Pin = Pins[PinIdx];
		if (!Pin)
		{
			continue;
		}

		if (Pin->Direction == EGPD_Input)
		{
			if (InputPin == nullptr)
			{
				InputPin = Pin;
				continue;
			}

			Pin->BreakAllPinLinks();
			RemovePin(Pin);
			continue;
		}

		if (Pin->Direction == EGPD_Output)
		{
			const int32 ChoiceIndex = GetChoiceIndexFromPin(Pin);
			if (ChoiceIndex == INDEX_NONE || ChoiceIndex >= ChoiceCount)
			{
				Pin->BreakAllPinLinks();
				RemovePin(Pin);
			}
		}
	}

	if (InputPin == nullptr)
	{
		CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	}

	for (int32 ChoiceIndex = 0; ChoiceIndex < ChoiceCount; ++ChoiceIndex)
	{
		const FName PinName = GetChoiceOutputPinName(ChoiceIndex);
		UEdGraphPin* OutPin = FindPinByNameAndDirection(Pins, EGPD_Output, PinName);

		if (OutPin == nullptr)
		{
			OutPin = CreatePin(EGPD_Output, "MultipleNodes", FName(), PinName);
		}

		if (OutPin)
		{
			if (PlayerChoiceNode && PlayerChoiceNode->ChoiceList.IsValidIndex(ChoiceIndex) && !PlayerChoiceNode->ChoiceList[ChoiceIndex].IsEmpty())
			{
				OutPin->PinFriendlyName = PlayerChoiceNode->ChoiceList[ChoiceIndex];
			}
			else
			{
				OutPin->PinFriendlyName = FText::Format(
					NSLOCTEXT("DialogBuilderEditor", "ChoicePinFallback", "Choice {0}"),
					FText::AsNumber(ChoiceIndex + 1));
			}
		}
	}
}

FName UDialogBuilderEdNode_PlayerChoice::GetChoiceOutputPinName(int32 ChoiceIndex)
{
	return FName(*FString::Printf(TEXT("Choice_%d"), ChoiceIndex));
}

int32 UDialogBuilderEdNode_PlayerChoice::GetChoiceIndexFromPin(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return INDEX_NONE;
	}

	const FString PinName = Pin->PinName.ToString();
	const FString Prefix = TEXT("Choice_");
	if (!PinName.StartsWith(Prefix))
	{
		return INDEX_NONE;
	}

	const FString NumberPart = PinName.RightChop(Prefix.Len());
	if (!NumberPart.IsNumeric())
	{
		return INDEX_NONE;
	}

	return FCString::Atoi(*NumberPart);
}

FText UDialogBuilderEdNode_PlayerChoice::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UDialogBuilderNode* MyNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FText UDialogBuilderEdNode_PlayerChoice::GetTooltipText() const
{
	const UDialogBuilderNode_PlayerChoice* PlayerChoiceNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance);
	if (PlayerChoiceNode)
	{
		return FText::Format(FText::FromString(TEXT("Player Choice Node\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "PlayerOptionNodeScopeTooltip", "This Node contain data for player choices.\n**Right click the node to start adding a subnodes."));
	}

	return Super::GetTooltipText();
}

FLinearColor UDialogBuilderEdNode_PlayerChoice::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->PlayerOptionNodeColor;
}

void UDialogBuilderEdNode_PlayerChoice::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsDecorators(Menu, "DialogBuilderEdNode", Context);
}
