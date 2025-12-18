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
	if (bIsSwinging || !LockComponent || !MoveComp) return;

	ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
	if (!Point || Point->AttachType != ERopeAttachType::Swing) return;

	SwingPoint = Point;
	RopeLength = FVector::Dist(OwnerCharacter->GetActorLocation(), SwingPoint->GetActorLocation());
	bIsSwinging = true;

	VelocityProjected = MoveComp->Velocity;
	MoveComp->SetMovementMode(MOVE_Flying);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Swing Started"));
}

void URopeSwingComponent::StopSwing()
{
	if (!bIsSwinging) return;

	bIsSwinging = false;
	SwingPoint.Reset();
	MoveComp->SetMovementMode(MOVE_Falling);
	MoveComp->Velocity = VelocityProjected;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Swing Stopped"));
}

void URopeSwingComponent::Detach()
{
	bIsSwinging = false;
	SwingPoint.Reset();

	if (AttachComponent && AttachComponent->IsAttached())
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
	if (!SwingPoint.IsValid() && !LockComponent->HasLockedPoint())
	{
		StopSwing();
		return;
	}

	const FVector Anchor = SwingPoint->GetActorLocation();
	FVector PlayerLoc = OwnerCharacter->GetActorLocation();
	FVector ToPlayer = PlayerLoc - Anchor;
	const float CurrentDist = ToPlayer.Size();
	if (CurrentDist <= KINDA_SMALL_NUMBER) return;

	FVector RopeDir = ToPlayer / CurrentDist;

	if (!bIsSwinging)
	{
		// Cas attaché au sol : on clamp juste la distance max
		if (MoveComp->IsMovingOnGround())
		{
			if (CurrentDist > RopeLength)
			{
				FVector CorrectedLoc = Anchor + RopeDir * RopeLength;
				OwnerCharacter->SetActorLocation(CorrectedLoc);
			}
			return;
		}
		else
		{
			// Si on est dans les airs et attaché mais pas swing → passer en swing
			StartSwing();
		}
	}

	// --- Swing actif ---
	FVector CorrectedLoc = Anchor + RopeDir * RopeLength;
	OwnerCharacter->SetActorLocation(CorrectedLoc);

	// Supprimer vitesse radiale
	FVector Vel = VelocityProjected;
	Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;

	// Gravité projetée
	const FVector Gravity(0,0,GetWorld()->GetGravityZ());
	Vel += Gravity * DeltaTime;

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
