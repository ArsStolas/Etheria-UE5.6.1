#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"

#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeConstraintComponent::URopeConstraintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URopeConstraintComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	MoveComp = OwnerCharacter->GetCharacterMovement();
}

void URopeConstraintComponent::ActivateConstraint(
	ARopeAttachPoint* InAnchor,
	float InRopeLength)
{
	if (!InAnchor || InRopeLength <= 0.f) return;

	Anchor = InAnchor;
	RopeLength = InRopeLength;
	bIsActive = true;

	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void URopeConstraintComponent::DeactivateConstraint()
{
	bIsActive = false;
	Anchor.Reset();
	RopeLength = 0.f;

	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void URopeConstraintComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsActive || !Anchor.IsValid()) return;

	ApplyConstraint();
}

void URopeConstraintComponent::ApplyConstraint()
{
	const FVector AnchorLoc = Anchor->GetActorLocation();
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

	FVector ToPlayer = PlayerLoc - AnchorLoc;
	const float CurrentDist = ToPlayer.Size();

	if (CurrentDist <= RopeLength || CurrentDist <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector RopeDir = ToPlayer / CurrentDist;
	const FVector ClampedLoc = AnchorLoc + RopeDir * RopeLength;

	OwnerCharacter->SetActorLocation(ClampedLoc, true);

	// Supprime la vitesse radiale (évite le jitter)
	if (MoveComp)
	{
		FVector Vel = MoveComp->Velocity;
		Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;
		MoveComp->Velocity = Vel;
	}
}
