#include "Components/Characters/Player/Rope/RopeSwingComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeSwingComponent::URopeSwingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URopeSwingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	MoveComp = OwnerCharacter->GetCharacterMovement();
	AttachComponent = OwnerCharacter->GetRopeAttachComponent();
	LockComponent = OwnerCharacter->GetRopeLockComponent();
	
	if (LockComponent)
	{
		LockComponent->OnLockedPointChanged.AddDynamic(this, &URopeSwingComponent::OnLockedPointChanged);
	}
}

void URopeSwingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsSwinging)
	{
		UpdateSwing(DeltaTime);
	}
}

void URopeSwingComponent::StartSwing()
{
	if (bIsSwinging || !OwnerCharacter || !MoveComp || !LockComponent) return;

	ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
	if (!Point || Point->AttachType != ERopeAttachType::Swing) return;

	SwingPoint = Point;
	bIsSwinging = true;

	// Projeter la vitesse actuelle sur le plan tangent
	const FVector Anchor = SwingPoint->GetActorLocation();
	const FVector ToPlayer = OwnerCharacter->GetActorLocation() - Anchor;
	const FVector RopeDir = ToPlayer.GetSafeNormal();

	FVector Vel = MoveComp->Velocity;
	Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;
	VelocityProjected = Vel;

	MoveComp->SetMovementMode(MOVE_Flying);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Swing Started"));
}

void URopeSwingComponent::StopSwing()
{
	if (!bIsSwinging) return;

	bIsSwinging = false;
	SwingPoint.Reset();

	if (MoveComp)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		MoveComp->Velocity = VelocityProjected;
	}

	PrimaryComponentTick.SetTickFunctionEnable(false);
	
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Swing Stopped"));
}


void URopeSwingComponent::Detach()
{
	if (!bIsSwinging && (!AttachComponent || !AttachComponent->IsAttached()))
	{
		return;
	}

	bIsSwinging = false;
	SwingPoint.Reset();

	if (AttachComponent)
	{
		AttachComponent->DetachRope();
	}

	if (MoveComp)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
	}

	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void URopeSwingComponent::UpdateSwing(float DeltaTime)
{
	if (!SwingPoint.IsValid())
	{
		StopSwing();
		return;
	}

	const FVector Anchor = SwingPoint->GetActorLocation();
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();
	const FVector RopeDir = (PlayerLoc - Anchor).GetSafeNormal();

	// 1️⃣ Supprimer toute composante radiale
	FVector Vel = VelocityProjected;
	Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;

	// 2️⃣ Appliquer la gravité (elle sera contrainte automatiquement)
	const FVector Gravity(0.f, 0.f, GetWorld()->GetGravityZ());
	Vel += Gravity * DeltaTime;

	// 3️⃣ Appliquer la vitesse
	VelocityProjected = Vel;
	OwnerCharacter->AddActorWorldOffset(Vel * DeltaTime, true);
}

void URopeSwingComponent::OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint)
{
	if (!bIsSwinging) return;
	
	if (!NewLockedPoint)
	{
		SwingPoint.Reset();
	}
}
