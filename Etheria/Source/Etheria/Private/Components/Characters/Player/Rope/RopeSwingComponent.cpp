#include "Components/Characters/Player/Rope/RopeSwingComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "DrawDebugHelpers.h"

/* ================= DEBUG MACROS ================= */

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	#define SWING_DEBUG_ENABLED 1
#else
	#define SWING_DEBUG_ENABLED 0
#endif

static void DebugLine(
	UWorld* World,
	const FVector& A,
	const FVector& B,
	const FColor& Color,
	float Thickness
)
{
#if SWING_DEBUG_ENABLED
	if (World)
	{
		DrawDebugLine(World, A, B, Color, false, 0.f, 0, Thickness);
	}
#endif
}

static void DebugPoint(
	UWorld* World,
	const FVector& P,
	const FColor& Color,
	float Size = 8.f
)
{
#if SWING_DEBUG_ENABLED
	if (World)
	{
		DrawDebugPoint(World, P, Size, Color, false, 0.f);
	}
#endif
}

static void DebugMsg(const FString& Msg, const FColor& Color)
{
#if SWING_DEBUG_ENABLED
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color, Msg);
	}
#endif
}

/* ================= COMPONENT ================= */

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

	if (URopeConstraintComponent* Constraint =
		OwnerCharacter->GetRopeConstraintComponent())
	{
		Constraint->OnRopeTensioned.AddDynamic(
			this,
			&URopeSwingComponent::OnRopeTensioned
		);
	}
}

void URopeSwingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsSwinging)
	{
		UpdateSwing(DeltaTime);
	}
}

/* ================= SWING CORE ================= */

void URopeSwingComponent::StartSwing()
{
	if (bIsSwinging || !OwnerCharacter || !MoveComp || !LockComponent)
		return;

	ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
	if (!Point || Point->AttachType != ERopeAttachType::Swing)
		return;

	SwingPoint = Point;
	bIsSwinging = true;

	const FVector Anchor = SwingPoint->GetActorLocation();
	const FVector ToPlayer = OwnerCharacter->GetActorLocation() - Anchor;
	const FVector RopeDir = ToPlayer.GetSafeNormal();

	FVector Vel = MoveComp->Velocity;
	Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;
	VelocityProjected = Vel;

	MoveComp->SetMovementMode(MOVE_Flying);
	PrimaryComponentTick.SetTickFunctionEnable(true);

	if (bDebugSwing)
	{
		DebugMsg(TEXT("Swing Started"), FColor::Green);
	}
}

void URopeSwingComponent::StopSwing()
{
	if (!bIsSwinging)
		return;

	bIsSwinging = false;
	SwingPoint.Reset();

	if (MoveComp)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		MoveComp->Velocity = VelocityProjected;
	}

	PrimaryComponentTick.SetTickFunctionEnable(false);

	if (bDebugSwing)
	{
		DebugMsg(TEXT("Swing Stopped"), FColor::Red);
	}
}

void URopeSwingComponent::UpdateSwing(float DeltaTime)
{
	if (!SwingPoint.IsValid() || HasTouchedGround())
	{
		StopSwing();
		return;
	}

	const FVector Anchor = SwingPoint->GetActorLocation();
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();
	const FVector RopeDir = (PlayerLoc - Anchor).GetSafeNormal();

	FVector Vel = VelocityProjected;
	Vel -= FVector::DotProduct(Vel, RopeDir) * RopeDir;

	Vel += FVector(0.f, 0.f, GetWorld()->GetGravityZ()) * DeltaTime;

	VelocityProjected = Vel;
	OwnerCharacter->AddActorWorldOffset(Vel * DeltaTime, true);

	if (bDebugSwing)
	{
		DebugLine(GetWorld(), Anchor, PlayerLoc, FColor::Cyan, DebugLineThickness);
		DebugLine(
			GetWorld(),
			PlayerLoc,
			PlayerLoc + VelocityProjected * 0.05f,
			FColor::Yellow,
			DebugLineThickness
		);
	}
}

/* ================= CONDITIONS ================= */

void URopeSwingComponent::OnRopeTensioned()
{
	if (!bIsSwinging && ShouldStartSwing())
	{
		StartSwing();
	}
}

bool URopeSwingComponent::ShouldStartSwing() const
{
	if (!AttachComponent || !AttachComponent->IsAttached())
		return false;

	if (!LockComponent || !LockComponent->HasLockedPoint())
		return false;

	const ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
	if (!Point || Point->AttachType != ERopeAttachType::Swing)
		return false;

	if (!MoveComp)
		return false;

	if (MoveComp->Velocity.Z > -FallingSpeedToStartSwing)
		return false;

	if (!IsFarEnoughFromGround())
		return false;

	return true;
}

bool URopeSwingComponent::IsFarEnoughFromGround() const
{
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!Capsule) return false;

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	const FVector Start =
		OwnerCharacter->GetActorLocation() - FVector(0, 0, HalfHeight - 2.f);
	const FVector End =
		Start - FVector(0, 0, MinHeightAboveGround);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params
	);

	if (bDebugSwing)
	{
		DebugLine(GetWorld(), Start, End,
			bHit ? FColor::Red : FColor::Green, DebugLineThickness);

		if (bHit)
		{
			DebugPoint(GetWorld(), Hit.ImpactPoint, FColor::Red);
		}
	}

	return !bHit;
}

bool URopeSwingComponent::HasTouchedGround() const
{
	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!Capsule || !MoveComp) return false;

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	const FVector Start =
		OwnerCharacter->GetActorLocation() - FVector(0, 0, HalfHeight - 2.f);
	const FVector End =
		Start - FVector(0, 0, GroundStopDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params
	);

	if (bDebugSwing)
	{
		DebugLine(GetWorld(), Start, End,
			bHit ? FColor::Red : FColor::Green, DebugLineThickness);
	}

	return bHit && MoveComp->Velocity.Z > -150.f;
}
