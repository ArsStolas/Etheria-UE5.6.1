/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: TestManagerSubsystem - Source
*/

#include "Tests/TestManagerSubsystem.h"

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
#include "Tests/CharacterComponents/HealthTestManager.h"
#endif

void UTestManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	HealthTests = NewObject<UHealthTestManager>(this);
    
	UE_LOG(LogTemp, Warning, TEXT("TestManagerSubsystem initialized with all test modules"));
#endif

	Test_ApplyDamage(TEXT("TestDummy"), 10.0f);
}

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
void UTestManagerSubsystem::Test_ApplyDamage(const FString& TargetName, float DamageAmount)
{
	UE_LOG(LogTemp, Error, TEXT("=== Test_ApplyDamage called with Target: %s, Damage: %f ==="), *TargetName, DamageAmount);
	
	if (HealthTests)
	{
		HealthTests->Test_ApplyDamage(TargetName, DamageAmount);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HealthTests is null in TestManagerSubsystem"));
	}
}

void UTestManagerSubsystem::Test_Heal(const FString& TargetName, float HealAmount)
{
	if (HealthTests)
	{
		HealthTests->Test_HealByName(TargetName, HealAmount);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HealthTests is null in TestManagerSubsystem"));
	}
}
#endif