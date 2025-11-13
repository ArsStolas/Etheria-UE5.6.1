/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: BaseCharacter - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UHealthComponent;
class UCombatComponent;
class UCharacterStateComponent;

UCLASS()
class ETHERIA_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	UFUNCTION(BlueprintPure, Category="Combat")
	UCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category="States")
	UCharacterStateComponent* GetStateComponent() const { return StateComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UCharacterStateComponent* StateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta=(AllowPrivateAccess="true"))
	UCombatComponent* CombatComponent;

private:
};
