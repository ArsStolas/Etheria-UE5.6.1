/*
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: CharacterStateComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/System/EtheriaGameplayTags.h"
#include "CharacterStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, FGameplayTag, PreviousState, FGameplayTag, NewState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UCharacterStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterStateComponent();

    // 🔸 Movement / Combat / Life
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States")
    FGameplayTag CurrentMovementState = EtheriaTags::State_Movement_Grounded_Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States")
    FGameplayTag CurrentCombatState = EtheriaTags::State_Combat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States")
    FGameplayTag CurrentLifeState = EtheriaTags::State_Life;

    // 🔹 Délégués d’événements
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStateChanged OnMovementStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStateChanged OnLifeStateChanged;

    // === Fonctions principales ===

    UFUNCTION(BlueprintCallable, Category = "States")
    void SetMovementState(FGameplayTag NewState);

    UFUNCTION(BlueprintCallable, Category = "States")
    void SetCombatState(FGameplayTag NewState);

    UFUNCTION(BlueprintCallable, Category = "States")
    void SetLifeState(FGameplayTag NewState);

    UFUNCTION(BlueprintCallable, Category = "States")
    void ClearMovementState();

    UFUNCTION(BlueprintCallable, Category = "States")
    void ClearCombatState();

    UFUNCTION(BlueprintCallable, Category = "States")
    void ClearLifeState();

    UFUNCTION(BlueprintPure, Category = "States")
    bool IsInMovementState(FGameplayTag QueryTag) const;

    UFUNCTION(BlueprintPure, Category = "States")
    bool IsInCombatState(FGameplayTag QueryTag) const;

    UFUNCTION(BlueprintPure, Category = "States")
    bool IsInLifeState(FGameplayTag QueryTag) const;
};

