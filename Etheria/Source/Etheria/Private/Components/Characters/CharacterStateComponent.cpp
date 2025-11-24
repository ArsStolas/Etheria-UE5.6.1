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
		PreviousMovementState = CurrentMovementState;
		FGameplayTag Previous = CurrentMovementState;
		CurrentMovementState = NewState;
		OnMovementStateChanged.Broadcast(Previous, NewState);
	}
}

void UCharacterStateComponent::SetCombatState(FGameplayTag NewState)
{
	if (CurrentCombatState != NewState)
	{
		PreviousCombatState = CurrentCombatState;
		FGameplayTag Previous = CurrentCombatState;
		CurrentCombatState = NewState;
		OnCombatStateChanged.Broadcast(Previous, NewState);
	}
}

void UCharacterStateComponent::SetLifeState(FGameplayTag NewState)
{
	if (CurrentLifeState != NewState)
	{
		PreviousLifeState = CurrentLifeState;
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

bool UCharacterStateComponent::WasInMovementState(FGameplayTag QueryTag) const
{
	return PreviousMovementState.MatchesTag(QueryTag);
}

bool UCharacterStateComponent::WasInCombatState(FGameplayTag QueryTag) const
{
	return PreviousCombatState.MatchesTag(QueryTag);
}

bool UCharacterStateComponent::WasInLifeState(FGameplayTag QueryTag) const
{
	return PreviousLifeState.MatchesTag(QueryTag);
}
