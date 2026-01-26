/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: "0nnen"
 * Class: HealthComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "HealthComponent.generated.h"

struct FGameplayTag;

class ABaseCharacter;
class UCharacterStateComponent;
class UMaterialInterface;
class UMeshComponent;

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

	// =============================================================
	// Damage Feedback - Overlay
	// =============================================================
	/** Force the hit overlay to appear immediately (also called automatically on damage). */
	UFUNCTION(BlueprintCallable, Category="Damage Feedback|Overlay")
	void TriggerDamageOverlay();

	/** Clears the hit overlay right away (normally called automatically after Duration). */
	UFUNCTION(BlueprintCallable, Category="Damage Feedback|Overlay")
	void ClearDamageOverlay();

	UFUNCTION(BlueprintCallable, Category="Damage Feedback|Overlay")
	void SetDamageOverlayDuration(const float NewDuration);

	UFUNCTION(BlueprintCallable, Category="Damage Feedback|Overlay")
	void SetDamageOverlayMaterial(UMaterialInterface* NewMaterial);

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

	// =============================================================
	// Damage Feedback - Overlay
	// =============================================================
	/** Enable/disable the hit overlay feature. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay")
	bool bEnableDamageOverlay = true;

	/** Material used as overlay for a short hit flash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay"))
	TObjectPtr<UMaterialInterface> DamageOverlayMaterial = nullptr;

	/** How long the overlay stays on (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay", ClampMin="0.0", UIMin="0.0", UIMax="0.25"))
	float DamageOverlayDuration = 0.06f;

	/** Optional: explicitly pick which mesh component receives the overlay (recommended for complex characters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay"))
	FComponentReference DamageOverlayMesh;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, const float Damage, const UDamageType* DamageType,
	                         AController* InstigatedBy, AActor* DamageCauser);

	void SetTemporaryLifeState(FGameplayTag TempState, float Duration);

	UMeshComponent* ResolveDamageOverlayMesh();

	FTimerHandle DamageOverlayTimerHandle;
	TWeakObjectPtr<UMeshComponent> CachedDamageOverlayMesh;
	bool bDamageOverlayActive = false;
};
