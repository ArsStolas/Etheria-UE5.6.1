// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "OrionRPG.h"
#include "Quest.h"

IMPLEMENT_MODULE(FOrionRPGModule, OrionRPG)

void FOrionRPGModule::StartupModule()
{
#if WITH_EDITOR
	RegisterQuestPresets();
#endif
}


void FOrionRPGModule::ShutdownModule()
{
	QuestPresets.Empty();
}

void FOrionRPGModule::RegisterQuestPresets()
{

	//Adding QuestPresets
	UQuest* MainQuestPreset = NewObject<UQuest>(UQuest::StaticClass(), NAME_None, RF_Transactional);
	MainQuestPreset->QuestCategory = FText::FromString("Main Quest");
	QuestPresets.Add(MainQuestPreset);

	UQuest* SecondaryQuestPreset = NewObject<UQuest>(UQuest::StaticClass(), NAME_None, RF_Transactional);
	SecondaryQuestPreset->QuestCategory = FText::FromString("Secondary Quest");
	QuestPresets.Add(SecondaryQuestPreset);

	
	MainQuestPreset->MakeQuestSettingShareable("Main Quest", true);
	SecondaryQuestPreset->MakeQuestSettingShareable("Secondary Quest", true);
}
