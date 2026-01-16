/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ChatGPT
* Class: BaseAI - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "BaseAI.generated.h"

UENUM(BlueprintType)
enum class EAIType : uint8
{
    Neutral,
    Hostile
};

UENUM(BlueprintType)
enum class EAIState : uint8
{
    Wander,
    Patrol,
    Chase,
    Fight
};

UCLASS()
class ETHERIA_API ABaseAI : public ABaseCharacter
{
    GENERATED_BODY()

public:
    ABaseAI();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    EAIType AIType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    UCharacterStateComponent* StateComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    class UCombatComponent* CombatComp;

    UFUNCTION(BlueprintCallable)
    bool IsHostile() const { return AIType == EAIType::Hostile; }

    UFUNCTION(BlueprintCallable)
    bool IsNeutral() const { return AIType == EAIType::Neutral; }

    /* Fonction à override pour le combat ou actions Fight */
    virtual void PerformAttack(AActor* TargetActor) {}

protected:
    virtual void BeginPlay() override;
};
