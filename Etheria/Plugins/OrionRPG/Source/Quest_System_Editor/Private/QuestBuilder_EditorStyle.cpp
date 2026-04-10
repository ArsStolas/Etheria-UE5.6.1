// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestBuilder_EditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FQuestBuilder_EditorStyle::StyleInstance = nullptr;

#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);


void FQuestBuilder_EditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FQuestBuilder_EditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FQuestBuilder_EditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("QuestEditorStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FQuestBuilder_EditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("OrionRPG")->GetBaseDir() / TEXT("Resources"));

	//Thumbnails and icons
	Style->Set(FName(TEXT("ClassThumbnail.Quest")), new IMAGE_BRUSH("Quest", Icon64x64));
	Style->Set(FName(TEXT("ClassIcon.Quest")), new IMAGE_BRUSH("Quest", Icon40x40));
	return Style;
}

void FQuestBuilder_EditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FQuestBuilder_EditorStyle::Get()
{
	return *StyleInstance;
}
