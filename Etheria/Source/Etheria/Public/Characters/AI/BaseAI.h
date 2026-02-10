/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseAI - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "BaseAI.generated.h"

UENUM(BlueprintType)
enum class EAIType : uint8
{
	Neutral,
	Hostile
};

UCLASS()
class ETHERIA_API ABaseAI : public ABaseCharacter
{
	GENERATED_BODY()

public:
	ABaseAI();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	EAIType AIType;

	bool IsHostile() const { return AIType == EAIType::Hostile; }
	bool IsNeutral() const { return AIType == EAIType::Neutral; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UCharacterStateComponent* StateComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	class UCombatComponent* CombatComp;

	virtual void TryAttack(AActor* TargetActor) { }

protected:
	virtual void BeginPlay() override;
};
