/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: HealthTestManager - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Tests/BaseTestManager.h"
#include "HealthTestManager.generated.h"

UCLASS(Blueprintable)
class ETHERIA_API UHealthTestManager : public UBaseTestManager
{
	GENERATED_BODY()

public:
	virtual void Initialize(UObject* Outer) override;

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_ApplyDamage(const FString& TargetName, float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_HealByName(const FString& TargetName, float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_ResetHealth(const FString& TargetName);

	UFUNCTION(BlueprintCallable, Category="Test|Health")
	void Test_LogHealthStats(const FString& TargetName);
};
