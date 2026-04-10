// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Quest.h"
#include "Modules/ModuleManager.h"

class UQuest;

class FOrionRPGModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	UPROPERTY()
	TArray<TObjectPtr<UQuest>> QuestPresets;

	void RegisterQuestPresets();
};