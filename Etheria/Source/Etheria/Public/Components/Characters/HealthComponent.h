/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: HealthComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

struct FGameplayTag;

class ABaseCharacter;
class UCharacterStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();
	
	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetHealth();
	
	UFUNCTION(BlueprintCallable, Category="Health")
	void TakeDamage(const float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(const float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category="Health")
	bool IsDead() const;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnDeathSignature OnDeath;

	TWeakObjectPtr<ABaseCharacter> OwnerCharacter;
	TWeakObjectPtr<UCharacterStateComponent> OwnerStateComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float Health = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category="State Timing")
	float DamageStateDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category="State Timing")
	float HealStateDuration = 0.8f;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, const float Damage, const UDamageType* DamageType,
	                         AController* InstigatedBy, AActor* DamageCauser);

	void SetTemporaryLifeState(FGameplayTag TempState, float Duration);
};
