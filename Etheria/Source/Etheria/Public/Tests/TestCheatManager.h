/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: TestCheatManager - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "TestCheatManager.generated.h"

class UBaseTestManager;
class UHealthTestManager;

UCLASS()
class ETHERIA_API UTestCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	virtual void InitCheatManager() override;

	UFUNCTION(Exec, Category="Test|Health")
	void Test_ApplyDamage(const FString& TargetName, float DamageAmount);

	UFUNCTION(Exec, Category="Test|Health")
	void Test_Heal(const FString& TargetName, float HealAmount);

	UFUNCTION(Exec, Category="Test|Health")
	void Test_ResetHealth(const FString& TargetName);

	UFUNCTION(Exec, Category="Test|Health")
	void Test_LogHealthStats(const FString& TargetName);
#endif

private:
	UPROPERTY(Transient)
	TMap<FString, UBaseTestManager*> TestManagers;

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	UBaseTestManager* GetManager(const FString& ManagerName) const;
#endif
};
