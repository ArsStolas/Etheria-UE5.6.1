/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossCharacter - Header"
 * Notes: Drop-in arena Golem. A BaseAICharacter that owns a UGolemBossComponent and ships with
 *        boss-friendly defaults (Aggressive, Boss rank, hand-faced, paired with AGolemBossController).
 *        Make a Blueprint child to assign the mesh, montages, attack list and bind the Golem dispatchers.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAICharacter.h"
#include "GolemBossCharacter.generated.h"

class UGolemBossComponent;

UCLASS()
class ETHERIA_API AGolemBossCharacter : public ABaseAICharacter
{
	GENERATED_BODY()

public:
	AGolemBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Golem") UGolemBossComponent* GetGolemBoss() const { return GolemBossComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UGolemBossComponent> GolemBossComponent;

private:
	/** Fills the component with the 6 demo attacks + a light 2-phase ramp as editable defaults (overridable in a BP child). */
	void BuildDefaultAttacks();
};
