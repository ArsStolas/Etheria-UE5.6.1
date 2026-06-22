// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderSetting.h"
#include "Blueprint/UserWidget.h"
#include "DialogSequenceShot.h"
#include "UObject/ConstructorHelpers.h"

UDialogBuilderSetting::UDialogBuilderSetting()
{
	MinDialogLineDuration = 2.5f;
	DialogLineWordsPerSecond = 4.0f; // Default words per second for dialog lines

	auto DialogueUserWidgetFinder = ConstructorHelpers::FClassFinder<UUserWidget>(TEXT("WidgetBlueprint'/OrionRPG/Widgets/Dialog/WBP_Dialog_Simple.WBP_Dialog_Simple_C'"));
	if (DialogueUserWidgetFinder.Succeeded())
	{
		DefaultDialogWidget = DialogueUserWidgetFinder.Class;
	}

	auto DialogueFreeMovementWidgetFinder = ConstructorHelpers::FClassFinder<UUserWidget>(TEXT("WidgetBlueprint'/OrionRPG/Widgets/Dialog/WBP_Dialog_FreeMovement.WBP_Dialog_FreeMovement_C'"));
	if (DialogueFreeMovementWidgetFinder.Succeeded())
	{
		DefaultFreeMovementDialogWidget = DialogueFreeMovementWidgetFinder.Class;
	}

	auto CloseUpShot = ConstructorHelpers::FClassFinder<UDialogSequenceShot>(TEXT("'/OrionRPG/Dialog/DefaultShot/SequenceShot/CloseUp.CloseUp_C'"));
	if (CloseUpShot.Succeeded())
	{
		DefaultCameraPresetsToGenerate.AddUnique(CloseUpShot.Class);
	}

	auto MediumShot = ConstructorHelpers::FClassFinder<UDialogSequenceShot>(TEXT("'/OrionRPG/Dialog/DefaultShot/SequenceShot/MediumShot.MediumShot_C'"));
	if (MediumShot.Succeeded())
	{
		DefaultCameraPresetsToGenerate.AddUnique(MediumShot.Class);
	}

	auto OTS_CloseUp = ConstructorHelpers::FClassFinder<UDialogSequenceShot>(TEXT("'/OrionRPG/Dialog/DefaultShot/SequenceShot/OTS_CloseUp.OTS_CloseUp_C'"));
	if (OTS_CloseUp.Succeeded())
	{
		DefaultCameraPresetsToGenerate.AddUnique(OTS_CloseUp.Class);
	}


	DialogSequenceNodeColor = FLinearColor(0.023f, 0.026f, 0.031f);
	DialogLineNodeColor = FLinearColor(0.15f, 0.15f, 0.15f);
	PlayerLineNodeColor = FLinearColor(.06f, .135f, .175f);
	RerouteNodeColor = FLinearColor(.063f, .096f, .1875f);
	EndDialogNodeColor = FLinearColor(.34f, .024f, .024f);
	PlayerOptionNodeColor = FLinearColor(0.015f, 0.029f, 0.052f);
	SubNodeColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.7f);
	RootNodeColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.2f);
	bEdgeEnabled = false;            
	bCanRenameNode = true;
	bCanBeCyclical = false;
	bShowGrid = true;
}
 

UDialogBuilderSetting::~UDialogBuilderSetting()
{
}
