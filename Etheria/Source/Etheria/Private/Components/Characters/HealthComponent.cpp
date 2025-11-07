/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: HealthComponent - Source
*/

#include "Components/Characters/HealthComponent.h"

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
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
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

	if (IsDead())
	{
		OnDeath.Broadcast();

		if (AActor* Owner = GetOwner())
		{
			Owner->Destroy();
		}
	}
}


void UHealthComponent::Heal(const float HealAmount)
{
	if (HealAmount <= 0.f || IsDead()) return;

	Health = FMath::Clamp(Health + HealAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
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
