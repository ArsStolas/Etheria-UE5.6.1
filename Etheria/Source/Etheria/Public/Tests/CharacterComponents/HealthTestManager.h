/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: HealthTestManager - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HealthTestManager.generated.h"

class AActor;
class UHealthComponent;

UCLASS(Blueprintable)
class ETHERIA_API UHealthTestManager : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_ApplyDamage(const FString& TargetName, float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_Heal(AActor* Target, const float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_HealByName(const FString& TargetName, const float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_ApplyDamageToAll(const FString& PartialName, float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_LogHealthStats(const FString& TargetName);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_ResetHealth(const FString& TargetName);

private:
	AActor* FindActorByName(const FString& TargetName) const;
    
	TArray<AActor*> FindActorsByPartialName(const FString& PartialName) const;
#endif
};
