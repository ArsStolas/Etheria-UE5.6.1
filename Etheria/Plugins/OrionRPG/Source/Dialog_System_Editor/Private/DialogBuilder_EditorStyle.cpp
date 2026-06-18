// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilder_EditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FDialogBuilder_EditorStyle::StyleInstance = nullptr;

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon24x24(24.0f, 24.0f);
const FVector2D Icon32x32(32.0f, 32.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);

void FDialogBuilder_EditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FDialogBuilder_EditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FDialogBuilder_EditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("DialogEditorStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FDialogBuilder_EditorStyle::Create()
{

	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("OrionRPG")->GetBaseDir() / TEXT("Resources"));

	//Thumbnails and icons
	Style->Set(FName(TEXT("ClassThumbnail.Dialog")), new IMAGE_BRUSH("Dialog", Icon64x64));
	Style->Set(FName(TEXT("ClassIcon.Dialog")), new IMAGE_BRUSH("Dialog", Icon40x40));
	Style->Set(FName(TEXT("ClassIcon.Dialog.Participant")), new IMAGE_BRUSH("ParticipantIcon", Icon24x24));
	Style->Set(FName(TEXT("ClassIcon.Dialog.Prop")), new IMAGE_BRUSH("PropIcon", Icon24x24));
	Style->Set(FName(TEXT("ClassIcon.DialogNode")), new IMAGE_BRUSH("DialogNode", Icon20x20));
	return Style;
}

void FDialogBuilder_EditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FDialogBuilder_EditorStyle::Get()
{
	return *StyleInstance;
}
