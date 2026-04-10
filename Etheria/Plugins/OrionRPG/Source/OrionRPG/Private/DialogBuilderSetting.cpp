// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderSetting.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

UDialogBuilderSetting::UDialogBuilderSetting()
{
	MinDialogLineDuration = 2.5f;
	DialogLineWordsPerSecond = 4.0f; // Default words per second for dialog lines

	auto DialogueUserWidgetFinder = ConstructorHelpers::FClassFinder<UUserWidget>(TEXT("WidgetBlueprint'/OrionRPG/Widgets/Dialog/WBP_Dialog.WBP_Dialog_C'"));
	if (DialogueUserWidgetFinder.Succeeded())
	{
		DefaultDialogWidget = DialogueUserWidgetFinder.Class;
	}

	auto DialogueFreeMovementWidgetFinder = ConstructorHelpers::FClassFinder<UUserWidget>(TEXT("WidgetBlueprint'/OrionRPG/Widgets/Dialog/WBP_Dialog_FreeMovement.WBP_Dialog_FreeMovement_C'"));
	if (DialogueFreeMovementWidgetFinder.Succeeded())
	{
		DefaultFreeMovementDialogWidget = DialogueFreeMovementWidgetFinder.Class;
	}

	DialogLineNodeColor = FLinearColor(0.15f, 0.15f, 0.15f);
	PlayerLineNodeColor = FLinearColor(.06f, .135f, .175f);
	RerouteNodeColor = FLinearColor(.063f, .096f, .1875f);
	EndDialogNodeColor = FLinearColor(.34f, .024f, .024f);
	PlayerOptionNodeColor = FLinearColor(.123f, .274f, .34f);
	SubNodeColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.7f);
	RootNodeColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.2f);
	bEdgeEnabled = false;            
	bCanRenameNode = true;
	bCanBeCyclical = false;
}
 

UDialogBuilderSetting::~UDialogBuilderSetting()
{
}
