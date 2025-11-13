/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: HealthComponent - Source
*/

#include "Components/Characters/HealthComponent.h"
#include "Characters/BaseCharacter.h"
#include "Components/Characters/CharacterStateComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		OwnerCharacter = Cast<ABaseCharacter>(Owner);
		OwnerCharacter->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
		OwnerStateComponent = OwnerCharacter->GetStateComponent();
	}
}

void UHealthComponent::ResetHealth()
{
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

/** 
 * TakeDamage
 * ----------
 * Fonction principale du composant qui applique réellement les dégâts
 * Met à jour la santé, déclenche OnHealthChanged et OnDeath si nécessaire
 * Peut être appelée manuellement depuis d'autres systèmes (DOT, sorts, pièges, etc.)
*/
void UHealthComponent::TakeDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f || IsDead()) return;

	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (OwnerStateComponent)
	{
		OwnerStateComponent->SetLifeState(EtheriaTags::State_Life_TakingDamage);
	}

	if (IsDead())
	{
		OwnerStateComponent->SetLifeState(EtheriaTags::State_Life_Dead);
		OnDeath.Broadcast();
	}
}

void UHealthComponent::Heal(const float HealAmount)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Healing for %f"), HealAmount));
	
	if (HealAmount <= 0.f || IsDead()) return;

	Health = FMath::Clamp(Health + HealAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (OwnerStateComponent && !IsDead())
	{
		OwnerStateComponent->SetLifeState(EtheriaTags::State_Life_Healing);
	}
}

float UHealthComponent::GetHealth() const
{
	return Health;
}

float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

bool UHealthComponent::IsDead() const
{
	return Health <= 0.f;
}

/**
 * HandleTakeAnyDamage
 * -------------------
 * Callback interne automatiquement appelé par le système de dégâts natif d'Unreal
 * lorsqu'un acteur subit des dégâts via ApplyDamage() (Coup d'épée, balle, explosion, etc.)
 * Redirige simplement vers TakeDamage() pour appliquer la logique de santé du composant
*/
void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, const float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	TakeDamage(Damage);
}
