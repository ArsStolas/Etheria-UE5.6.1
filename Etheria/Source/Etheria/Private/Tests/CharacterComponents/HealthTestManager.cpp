/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: HealthTestManager - Source
*/

#include "Tests/CharacterComponents/HealthTestManager.h"

#include "EngineUtils.h"
#include "Components/Characters/HealthComponent.h"

void UHealthTestManager::Initialize(UObject* Outer)
{
}

void UHealthTestManager::Test_ApplyDamage(const FString& TargetName, float DamageAmount)
{
	AActor* Target = nullptr;
	UWorld* World = GWorld;
	
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(TargetName))
			{
				Target = *It;
				break;
			}
		}
	}

	if (!Target) return;

	if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
	{
		float Before = Health->GetHealth();
		Health->TakeDamage(DamageAmount);
		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s took %f damage: %f -> %f"),
		       *Target->GetName(), DamageAmount, Before, Health->GetHealth());
	}
}

void UHealthTestManager::Test_HealByName(const FString& TargetName, float HealAmount)
{
	AActor* Target = nullptr;
	UWorld* World = GWorld;
	
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(TargetName))
			{
				Target = *It;
				break;
			}
		}
	}

	if (!Target) return;

	if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
	{
		float Before = Health->GetHealth();
		Health->Heal(HealAmount);
		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s healed %f: %f -> %f"),
		       *Target->GetName(), HealAmount, Before, Health->GetHealth());
	}
}

void UHealthTestManager::Test_ResetHealth(const FString& TargetName)
{
	AActor* Target = nullptr;
	UWorld* World = GWorld;
	
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(TargetName))
			{
				Target = *It;
				break;
			}
		}
	}

	if (!Target) return;

	if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
	{
		Health->ResetHealth();
		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s health reset to %f"), *Target->GetName(), Health->GetHealth());
	}
}

void UHealthTestManager::Test_LogHealthStats(const FString& TargetName)
{
	AActor* Target = nullptr;
	UWorld* World = GWorld;
	
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(TargetName))
			{
				Target = *It;
				break;
			}
		}
	}

	if (!Target) return;

	if (const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s Stats - Health: %f/%f (%.1f%%) - Dead: %s"),
		       *Target->GetName(),
		       Health->GetHealth(),
		       Health->GetMaxHealth(),
		       (Health->GetHealth() / Health->GetMaxHealth()) * 100.0f,
		       Health->IsDead() ? TEXT("YES") : TEXT("NO"));
	}
}