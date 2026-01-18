/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: "0nnen"
 * Class: HealthComponent - Source
*/

#include "Components/Characters/HealthComponent.h"
#include "Characters/BaseCharacter.h"
#include "Components/Characters/CharacterStateComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
		if (OwnerCharacter.IsValid())
		{
			OwnerCharacter->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
			OwnerStateComponent = OwnerCharacter->GetStateComponent();
		}
	}

	// Cache the mesh we will use for the hit overlay (optional but avoids resolving every hit).
	CachedDamageOverlayMesh = ResolveDamageOverlayMesh();
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

	TriggerDamageOverlay();

	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (!OwnerStateComponent.IsValid())
		return;

	if (IsDead())
	{
		OwnerStateComponent->SetLifeState(EtheriaTags::State_Life_Dead);
		OnDeath.Broadcast();
	}
	else
	{
		SetTemporaryLifeState(EtheriaTags::State_Life_TakingDamage, DamageStateDuration);
	}
}

// =============================================================
// Damage Feedback - Overlay
// =============================================================

UMeshComponent* UHealthComponent::ResolveDamageOverlayMesh()
{
	if (!bEnableDamageOverlay)
	{
		return nullptr;
	}

	if (CachedDamageOverlayMesh.IsValid())
	{
		return CachedDamageOverlayMesh.Get();
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 1) Explicit mesh reference (recommended)
	if (UActorComponent* Comp = DamageOverlayMesh.GetComponent(Owner))
	{
		if (UMeshComponent* Mesh = Cast<UMeshComponent>(Comp))
		{
			CachedDamageOverlayMesh = Mesh;
			return Mesh;
		}
	}

	// 2) OwnerCharacter mesh (if we are on a character)
	if (OwnerCharacter.IsValid() && OwnerCharacter->GetMesh())
	{
		CachedDamageOverlayMesh = OwnerCharacter->GetMesh();
		return OwnerCharacter->GetMesh();
	}

	// 3) First mesh component on owner
	if (UMeshComponent* Mesh = Owner->FindComponentByClass<UMeshComponent>())
	{
		CachedDamageOverlayMesh = Mesh;
		return Mesh;
	}

	return nullptr;
}

void UHealthComponent::TriggerDamageOverlay()
{
	if (!bEnableDamageOverlay || !DamageOverlayMaterial)
	{
		return;
	}

	UMeshComponent* Mesh = ResolveDamageOverlayMesh();
	if (!Mesh || !GetWorld())
	{
		return;
	}

	// Apply overlay + restart the timer if we're hit multiple times quickly.
	Mesh->SetOverlayMaterial(DamageOverlayMaterial);
	bDamageOverlayActive = true;

	GetWorld()->GetTimerManager().ClearTimer(DamageOverlayTimerHandle);
	if (DamageOverlayDuration <= 0.f)
	{
		ClearDamageOverlay();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		DamageOverlayTimerHandle,
		this,
		&UHealthComponent::ClearDamageOverlay,
		DamageOverlayDuration,
		false
	);
}

void UHealthComponent::ClearDamageOverlay()
{
	UMeshComponent* Mesh = ResolveDamageOverlayMesh();
	if (Mesh)
	{
		Mesh->SetOverlayMaterial(nullptr);
	}

	bDamageOverlayActive = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageOverlayTimerHandle);
	}
}

void UHealthComponent::SetDamageOverlayDuration(const float NewDuration)
{
	DamageOverlayDuration = FMath::Max(0.f, NewDuration);
}

void UHealthComponent::SetDamageOverlayMaterial(UMaterialInterface* NewMaterial)
{
	DamageOverlayMaterial = NewMaterial;
}

void UHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.f || IsDead()) return;

	Health = FMath::Clamp(Health + HealAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (OwnerStateComponent.IsValid())
	{
		SetTemporaryLifeState(EtheriaTags::State_Life_Healing, HealStateDuration);
	}
}

void UHealthComponent::SetTemporaryLifeState(FGameplayTag TempState, float Duration)
{
	if (!OwnerStateComponent.IsValid()) return;

	OwnerStateComponent->SetLifeState(TempState);

	FTimerHandle ResetTimer;
	GetWorld()->GetTimerManager().SetTimer(
		ResetTimer,
		[this]()
		{
			if (OwnerStateComponent.IsValid() && !IsDead())
			{
				OwnerStateComponent->ClearLifeState();
			}
		},
		Duration,
		false
	);
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
