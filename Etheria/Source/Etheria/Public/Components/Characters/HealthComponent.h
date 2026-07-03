/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
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
class UPrimitiveComponent;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamagedDirectionalSignature, FVector, HitDirection, AActor*, DamageCauser);

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

	UFUNCTION(BlueprintCallable, Category="Damage Feedback|Overlay")
	void TriggerDamageOverlay();

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

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsInvulnerable() const { return bInvulnerable; }

	UFUNCTION(BlueprintCallable, Category="Health")
	void SetMinHealth(float NewMin) { MinHealth = FMath::Clamp(NewMin, 0.f, MaxHealth); }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMinHealth() const { return MinHealth; }

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnDamagedDirectionalSignature OnDamagedDirectional;

	TWeakObjectPtr<AActor> OwnerActor;
	TWeakObjectPtr<ABaseCharacter> OwnerCharacter;
	TWeakObjectPtr<UCharacterStateComponent> OwnerStateComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	float Health = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health", meta=(ClampMin="0"))
	float MinHealth = 0.f;

	UPROPERTY(EditAnywhere, Category="State Timing")
	float DamageStateDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category="State Timing")
	float HealStateDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay")
	bool bEnableDamageOverlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay"))
	TObjectPtr<UMaterialInterface> DamageOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay", ClampMin="0.0", UIMin="0.0", UIMax="0.25"))
	float DamageOverlayDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage Feedback|Overlay", meta=(EditCondition="bEnableDamageOverlay"))
	FComponentReference DamageOverlayMesh;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, const float Damage, const UDamageType* DamageType,
	                         AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	                           UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	                           const UDamageType* DamageType, AActor* DamageCauser);

	void SetTemporaryLifeState(FGameplayTag TempState, float Duration);

	UMeshComponent* ResolveDamageOverlayMesh();

	FTimerHandle DamageOverlayTimerHandle;
	TWeakObjectPtr<UMeshComponent> CachedDamageOverlayMesh;
	bool bDamageOverlayActive = false;
};
