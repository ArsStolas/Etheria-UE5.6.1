/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseEnemy - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAI.h"
#include "BaseEnemy.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle,
	Chase,
	Fighting
};

inline FName SelectedComboId = NAME_None;
inline FName SingleAttackId = NAME_None;


UCLASS(Abstract)
class ETHERIA_API ABaseEnemy : public ABaseAI
{
	GENERATED_BODY()

public:
	ABaseEnemy();
	virtual void Tick(float DeltaTime) override;
	virtual void HandlePerception() override;
	virtual bool CanIdleMove() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	EEnemyState CurrentState = EEnemyState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	AActor* TargetActor = nullptr;

	FTimerHandle EnemyAttackTimerHandle;
	bool bIsAttacking = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat")
	FName ComboIdToUse = "DefaultCombo";

	int32 ComboStep = 0;
	int32 ComboLength = 0;
	bool bComboInProgress = false;

	void StartAttackCycle();
	void StopAttackCycle();
	void AttackEnemy();
	void StartChasePlayer();
	void ResetComboState();
	void AdvanceCombo();

	UFUNCTION()
	void OnComboEndHandler(FName AttackId);

	UFUNCTION()
	void TryComboAttackStep();

};
