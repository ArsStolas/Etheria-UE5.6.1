/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseBoss - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAI.h"
#include "BaseBoss.generated.h"

class UWeaponData;
class UEquipmentComponent;

/**
 * Boss de base : hérite de BaseAI, réutilise le même pipeline combat qu les persos / IA
 * (CombatComponent + EquipmentComponent + WeaponData pour attaques et combos).
 * Pas d'inventaire : l'arme est définie via DefaultBossWeaponData ou sur l'EquipmentComponent.
 * Le comportement est piloté par le Behavior Tree (AIController_Boss).
 */
UCLASS()
class ETHERIA_API ABaseBoss : public ABaseAI
{
	GENERATED_BODY()

public:
	ABaseBoss();

	UFUNCTION(BlueprintPure, Category="Boss")
	UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	/** Arme par défaut du boss (phase 1). Utilisée aussi si Phase2/Phase3 non définis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Weapon")
	TObjectPtr<UWeaponData> DefaultBossWeaponData = nullptr;

	/** Arme pour la phase 2 (enragé). Si null, garde DefaultBossWeaponData. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Weapon|Phases")
	TObjectPtr<UWeaponData> Phase2WeaponData = nullptr;

	/** Arme pour la phase 3 (dernier souffle). Si null, garde l'arme de la phase précédente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Weapon|Phases")
	TObjectPtr<UWeaponData> Phase3WeaponData = nullptr;

	/** Applique le WeaponData correspondant à la phase (1/2/3). N'affecte que ce boss, pas le joueur ni les autres IA. */
	UFUNCTION(BlueprintCallable, Category="Boss|Weapon")
	void ApplyWeaponDataForPhase(int32 Phase);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UEquipmentComponent> EquipmentComponent = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual bool ShouldUseHostileCombatLoop_Implementation() const override { return false; }
};
