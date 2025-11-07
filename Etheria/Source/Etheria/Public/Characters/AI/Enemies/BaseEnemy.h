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

UCLASS(Abstract)
class ETHERIA_API ABaseEnemy : public ABaseAI
{
	GENERATED_BODY()

public:
	ABaseEnemy();
	virtual void Tick(float DeltaTime) override;
	virtual void HandlePerception() override;
	virtual bool CanIdleMove() const override;
	
	void StartChasePlayer();
	void AttackEnemy();
	void StartAttackCycle();

	void StopAttackCycle();
	void ResetComboState();

	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	EEnemyState CurrentState = EEnemyState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	bool bUseBehaviorTree = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	AActor* TargetActor = nullptr;

protected:
	virtual void BeginPlay() override;

	FTimerHandle EnemyAttackTimerHandle;

	int32 ComboStep = 0;
	int32 ComboLength = 0;
	bool bComboInProgress = false;

	UFUNCTION()
	void OnComboEndHandler(FName AttackId);

	UFUNCTION()
	void TryComboAttackStep();
	
	FName SelectedComboId = NAME_None;
	FName SingleAttackId = NAME_None;

};
