/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeConstraintComponent - Header
*/

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

void URopeConstraintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsActive || !Anchor.IsValid()) return;

	ApplyConstraint();
}

void URopeConstraintComponent::ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength)
{
	if (!InAnchor)
	{
		CONSTRAINT_LOG(LogTemp, Error, TEXT("[RopeConstraint] Activation failed - Anchor is null"));
		return;
	}

	if (InRopeLength <= 0.f)
	{
		CONSTRAINT_LOG(LogTemp, Error, TEXT("[RopeConstraint] Activation failed - Invalid rope length (%.2f)"), InRopeLength);
		return;
	}

	Anchor = InAnchor;
	RopeLength = InRopeLength;
	bIsActive = true;

	PrimaryComponentTick.SetTickFunctionEnable(true);
    
	CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Activated on %s with length: %.2f"), 
		*InAnchor->GetName(), RopeLength);
}

void URopeConstraintComponent::DeactivateConstraint()
{
	CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Deactivated"));
    
	bIsActive = false;
	Anchor.Reset();
	RopeLength = 0.f;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void URopeConstraintComponent::SetRopeLength(float NewLength)
{
	float OldLength = RopeLength;
	RopeLength = FMath::Max(NewLength, 50.f);
    
	CONSTRAINT_LOG(LogTemp, Verbose, TEXT("[RopeConstraint] Length changed: %.2f -> %.2f"), OldLength, RopeLength);
}

void URopeConstraintComponent::ApplyConstraint()
{
	const FVector AnchorLoc = Anchor->GetActorLocation();
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

	FVector ToPlayer = PlayerLoc - AnchorLoc;
	const float CurrentDist = ToPlayer.Size();

	if (CurrentDist <= RopeLength || CurrentDist <= KINDA_SMALL_NUMBER)
		return;

	const FVector RopeDir = ToPlayer / CurrentDist;

	// Broadcast tension event
	OnRopeTensioned.Broadcast();

	// Clamp player position
	const FVector ClampedLoc = AnchorLoc + RopeDir * RopeLength;
	OwnerCharacter->SetActorLocation(ClampedLoc, true);

	if (MoveComp)
	{
		FVector Vel = MoveComp->Velocity;
		Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;
		MoveComp->Velocity = Vel;
	}
}
