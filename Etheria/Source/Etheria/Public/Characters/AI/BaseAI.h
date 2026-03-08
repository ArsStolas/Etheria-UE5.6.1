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
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "BaseAI.generated.h"

UENUM(BlueprintType)
enum class EAIType : uint8
{
    Friendly,
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
    EAIType AIType = EAIType::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Teams")
    FGameplayTagContainer TeamTags;

    /** Accélération max pour le déplacement (patrol, chase, wander). Utilisée pour des animations fluides selon la vitesse. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="1", ClampMax="2048"))
    float AIMaxAcceleration = 512.f;

    /** Décélération au freinage (marche). Plus la valeur est haute, plus l'arrêt est rapide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0", ClampMax="2048"))
    float AIBrakingDeceleration = 512.f;

    /** Vitesse de marche max par défaut (chase, points patrol, wander). Peut être surchargée par les composants (ex. spline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="1", ClampMax="1200"))
    float DefaultMaxWalkSpeed = 300.f;

    bool IsFriendly() const { return AIType == EAIType::Friendly; }
    bool IsNeutral()  const { return AIType == EAIType::Neutral; }
    bool IsHostile()  const { return AIType == EAIType::Hostile; }

    UFUNCTION(BlueprintCallable, Category="AI|Combat")
    bool IsTargetHostile(AActor* InTargetActor) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    UCharacterStateComponent* StateComp = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
    class UCombatComponent* CombatComp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat")
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="100", ClampMax="1000"))
    float AttackRange = 300.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
    FTimerHandle CombatTimerHandle;

    virtual void TryAttack(AActor* InTargetActor);

    UFUNCTION(BlueprintCallable, Category="AI|Combat")
    void StartHostileCombatLoop();

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="AI|Teams")
    virtual void SetupTeamTags();

    UFUNCTION(BlueprintCallable, Category="Combat")
    virtual void PerformAIAttack(AActor* InTargetActor);
};
