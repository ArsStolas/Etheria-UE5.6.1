/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: TestManagerSubsystem - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TestManagerSubsystem.generated.h"

class UHealthTestManager;

UCLASS()
class ETHERIA_API UTestManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT

    UFUNCTION(Exec, BlueprintCallable, Category="Test|Health")
    void Test_ApplyDamage(const FString& TargetName, float DamageAmount);

    UFUNCTION(Exec, BlueprintCallable, Category="Test|Health")
    void Test_Heal(const FString& TargetName, float HealAmount);

    UFUNCTION(BlueprintCallable, Category="Test")
    UHealthTestManager* GetHealthTestManager() const { return HealthTests; }
#endif

protected:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    UHealthTestManager* HealthTests;
#endif
};
