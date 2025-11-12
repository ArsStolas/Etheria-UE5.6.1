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

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("StateComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("BPC_Combat"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	/*if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ABaseCharacter::HandleDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &ABaseCharacter::HandleHealthChanged);
	}*/
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/*void ABaseCharacter::HandleDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("%s est mort !"), *GetName());
	if (StateComponent)
	{
		StateComponent->SetLifeState(EtheriaTags::State_Life_Dead);
	}
}

void ABaseCharacter::HandleHealthChanged(const float NewHealth, const float MaxHealth)
{
	UE_LOG(LogTemp, Warning, TEXT("%s Health : %f / %f"), *GetName(), NewHealth, MaxHealth);
}*/