/*
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: BaseCharacter - Source
*/

#include "Characters/BaseCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/HealthComponent.h"
#include "Components/Combat/CombatComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("StateComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("BPC_Combat"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
