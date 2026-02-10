/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: TestCheatManager - Source
*/

#include "Tests/TestCheatManager.h"

#include "Engine/World.h"
#include "Tests/CharacterComponents/HealthTestManager.h"

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT

void UTestCheatManager::InitCheatManager()
{
	Super::InitCheatManager();

	if (UHealthTestManager* HealthManager = NewObject<UHealthTestManager>(this))
	{
		HealthManager->Initialize(this);
		TestManagers.Add("Health", HealthManager);
	}
}

UBaseTestManager* UTestCheatManager::GetManager(const FString& ManagerName) const
{
	if (TestManagers.Contains(ManagerName))
	{
		return TestManagers[ManagerName];
	}
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Manager '%s' not found"), *ManagerName);
	return nullptr;
}

void UTestCheatManager::Test_ApplyDamage(const FString& TargetName, float DamageAmount)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_ApplyDamage(TargetName, DamageAmount);
	}
}

void UTestCheatManager::Test_Heal(const FString& TargetName, float HealAmount)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_HealByName(TargetName, HealAmount);
	}
}

void UTestCheatManager::Test_ResetHealth(const FString& TargetName)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_ResetHealth(TargetName);
	}
}

void UTestCheatManager::Test_LogHealthStats(const FString& TargetName)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_LogHealthStats(TargetName);
	}
}

#endif