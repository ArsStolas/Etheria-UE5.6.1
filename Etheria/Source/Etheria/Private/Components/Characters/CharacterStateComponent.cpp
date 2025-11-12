/*
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: CharacterStateComponent - Source
*/

#include "Components/Characters/CharacterStateComponent.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterStateComponent::SetMovementState(FGameplayTag NewState)
{
	if (CurrentMovementState != NewState)
	{
		FGameplayTag Previous = CurrentMovementState;
		CurrentMovementState = NewState;
		OnMovementStateChanged.Broadcast(Previous, NewState);
	}
}

void UCharacterStateComponent::SetCombatState(FGameplayTag NewState)
{
	if (CurrentCombatState != NewState)
	{
		FGameplayTag Previous = CurrentCombatState;
		CurrentCombatState = NewState;
		OnCombatStateChanged.Broadcast(Previous, NewState);
	}
}

void UCharacterStateComponent::SetLifeState(FGameplayTag NewState)
{
	if (CurrentLifeState != NewState)
	{
		FGameplayTag Previous = CurrentLifeState;
		CurrentLifeState = NewState;
		OnLifeStateChanged.Broadcast(Previous, NewState);
	}
}

void UCharacterStateComponent::ClearMovementState()
{
	SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
}

void UCharacterStateComponent::ClearCombatState()
{
	SetCombatState(EtheriaTags::State_Combat);
}

void UCharacterStateComponent::ClearLifeState()
{
	SetLifeState(EtheriaTags::State_Life);
}

bool UCharacterStateComponent::IsInMovementState(FGameplayTag QueryTag) const
{
	return CurrentMovementState.MatchesTag(QueryTag);
}

bool UCharacterStateComponent::IsInCombatState(FGameplayTag QueryTag) const
{
	return CurrentCombatState.MatchesTag(QueryTag);
}

bool UCharacterStateComponent::IsInLifeState(FGameplayTag QueryTag) const
{
	return CurrentLifeState.MatchesTag(QueryTag);
}
