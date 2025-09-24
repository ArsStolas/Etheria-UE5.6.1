/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: HealthTestManager - Source
*/

#include "Tests/CharacterComponents/HealthTestManager.h"

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
#include "EngineUtils.h"
#include "Components/Characters/HealthComponent.h"

void UHealthTestManager::Test_ApplyDamage(const FString& TargetName, float DamageAmount)
{
    AActor* Target = FindActorByName(TargetName);
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s not found"), *TargetName);
        return;
    }

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s has no HealthComponent"), *TargetName);
        return;
    }

    float HealthBefore = Health->GetHealth();
    Health->TakeDamage(DamageAmount);
    float HealthAfter = Health->GetHealth();

    UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s took %f damage: %f -> %f"), 
           *Target->GetName(), DamageAmount, HealthBefore, HealthAfter);
}

void UHealthTestManager::Test_Heal(AActor* Target, const float HealAmount)
{
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Target is null"));
        return;
    }

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s has no HealthComponent"), *Target->GetName());
        return;
    }

    float HealthBefore = Health->GetHealth();
    Health->Heal(HealAmount);
    float HealthAfter = Health->GetHealth();

    UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s healed %f: %f -> %f"), 
           *Target->GetName(), HealAmount, HealthBefore, HealthAfter);
}

void UHealthTestManager::Test_HealByName(const FString& TargetName, const float HealAmount)
{
    AActor* Target = FindActorByName(TargetName);
    if (Target)
    {
        Test_Heal(Target, HealAmount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s not found for healing"), *TargetName);
    }
}

void UHealthTestManager::Test_ApplyDamageToAll(const FString& PartialName, float DamageAmount)
{
    TArray<AActor*> Targets = FindActorsByPartialName(PartialName);
    
    UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Found %d actors containing '%s'"), Targets.Num(), *PartialName);
    
    for (AActor* Target : Targets)
    {
        if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
        {
            float HealthBefore = Health->GetHealth();
            Health->TakeDamage(DamageAmount);
            UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s: %f -> %f"), 
                   *Target->GetName(), HealthBefore, Health->GetHealth());
        }
    }
}

void UHealthTestManager::Test_LogHealthStats(const FString& TargetName)
{
    AActor* Target = FindActorByName(TargetName);
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s not found"), *TargetName);
        return;
    }

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s has no HealthComponent"), *TargetName);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s Stats - Health: %f/%f (%.1f%%) - Dead: %s"), 
           *Target->GetName(), 
           Health->GetHealth(), 
           Health->GetMaxHealth(),
           (Health->GetHealth() / Health->GetMaxHealth()) * 100.0f,
           Health->IsDead() ? TEXT("YES") : TEXT("NO"));
}

void UHealthTestManager::Test_ResetHealth(const FString& TargetName)
{
    AActor* Target = FindActorByName(TargetName);
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s not found"), *TargetName);
        return;
    }

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (!Health)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Actor %s has no HealthComponent"), *TargetName);
        return;
    }

    Health->ResetHealth();
    UE_LOG(LogTemp, Warning, TEXT("[HealthTest] %s health reset to %f"), *Target->GetName(), Health->GetHealth());
}

AActor* UHealthTestManager::FindActorByName(const FString& TargetName) const
{
    UWorld* World = GWorld;
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName().Contains(TargetName))
        {
            return Actor;
        }
    }
    return nullptr;
}

TArray<AActor*> UHealthTestManager::FindActorsByPartialName(const FString& PartialName) const
{
    TArray<AActor*> FoundActors;
    UWorld* World = GWorld;
    if (!World) return FoundActors;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName().Contains(PartialName))
        {
            FoundActors.Add(Actor);
        }
    }
    return FoundActors;
}

#endif
