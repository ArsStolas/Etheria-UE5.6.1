/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonFallVolume" - Source
 */

#include "World/Dungeon/DungeonFallVolume.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

ADungeonFallVolume::ADungeonFallVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = root;

	triggerBox = CreateDefaultSubobject<UBoxComponent>("TriggerBox");
	triggerBox->SetupAttachment(root);
	triggerBox->SetBoxExtent(FVector(2000.f, 2000.f, 100.f));

	// QueryOnly + overlap UNIQUEMENT avec les Pawns.
	triggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	triggerBox->SetCollisionObjectType(ECC_WorldStatic);
	triggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	triggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	triggerBox->SetGenerateOverlapEvents(true);
}

void ADungeonFallVolume::BeginPlay()
{
	Super::BeginPlay();

	if (triggerBox)
	{
		triggerBox->OnComponentBeginOverlap.AddDynamic(this, &ADungeonFallVolume::OnVolumeBeginOverlap);
	}
}

void ADungeonFallVolume::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
	{
		return;
	}

	if (RespawnCharacter(Character))
	{
		OnPlayerRespawned(Character);
	}
}

bool ADungeonFallVolume::ResolveRespawnTransform(FTransform& OutTransform) const
{
	// 1) Référence directe (préféré) - acteur du même level choisi dans l'éditeur.
	if (IsValid(respawnTarget))
	{
		OutTransform = respawnTarget->GetActorTransform();
		return true;
	}

	// 2) Fallback : premier acteur avec respawnTag vivant dans le MÊME level que ce volume.
	if (!respawnTag.IsNone())
	{
		TArray<AActor*> TaggedActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), respawnTag, TaggedActors);

		for (AActor* Actor : TaggedActors)
		{
			if (IsValid(Actor) && Actor->GetLevel() == GetLevel())
			{
				OutTransform = Actor->GetActorTransform();
				return true;
			}
		}
	}

	return false;
}

float ADungeonFallVolume::ComputeLiftZ(const ACharacter* Character) const
{
	if (!bAutoLiftByCapsuleHalfHeight || !Character)
	{
		return extraTeleportZ;
	}

	if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		return Capsule->GetScaledCapsuleHalfHeight() + extraTeleportZ;
	}

	return extraTeleportZ;
}

bool ADungeonFallVolume::RespawnCharacter(ACharacter* Character) const
{
	FTransform Target;
	if (!ResolveRespawnTransform(Target))
	{
		return false;
	}

	Target.AddToTranslation(FVector(0.f, 0.f, ComputeLiftZ(Character)));

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}

	const FVector DestLocation = Target.GetLocation();
	const FRotator DestRotation = Target.Rotator();

	const bool bTeleported = Character->TeleportTo(DestLocation, DestRotation, false, false);
	if (!bTeleported)
	{
		Character->SetActorLocationAndRotation(DestLocation, DestRotation, true, nullptr, ETeleportType::TeleportPhysics);
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		PC->SetControlRotation(DestRotation);
	}

	Character->SetActorRotation(FRotator(0.f, DestRotation.Yaw, 0.f));
	return true;
}