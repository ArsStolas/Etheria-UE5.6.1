/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GrappleComponent - Source
*/

#include "Components/Characters/Player/Grapple/GrappleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "World/Grapple/GrapplePointActor.h"

UGrappleComponent::UGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("GrappleComponent must be attached to a Character"));
		return;
	}

	// Cable setup
	Cable = NewObject<UCableComponent>(OwnerCharacter);
	Cable->RegisterComponent();
	Cable->SetVisibility(false);
	Cable->AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::KeepRelativeTransform
	);
	Cable->EndLocation = FVector::ZeroVector;
}

void UGrappleComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Détection seulement si pas accroché
	if (!bIsGrappling)
		CheckForGrapplePoint();

	// Swing
	if (bIsGrappling && CurrentGrapplePoint)
		HandleSwing(DeltaTime);
}

//////////////////////////////////////////////////////////////
// Detection (cone + sphere)
//////////////////////////////////////////////////////////////

void UGrappleComponent::CheckForGrapplePoint()
{
	if (!OwnerCharacter) return;
	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return;

	const FVector CameraLocation = Camera->GetComponentLocation();
	const FVector CameraForward = OwnerCharacter->GetControlRotation().Vector();

	const float MaxDistance = MaxGrappleDistance;
	const float MinDot = 0.85f;

	AGrapplePointActor* BestPoint = nullptr;
	float BestScore = 0.f;

	for (TActorIterator<AGrapplePointActor> It(GetWorld()); It; ++It)
	{
		AGrapplePointActor* Point = *It;
		if (!Point) continue;

		const FVector PointLocation = Point->AttachPoint->GetComponentLocation();
		const FVector ToPoint = PointLocation - CameraLocation;
		const float Distance = ToPoint.Size();

		if (Distance > Point->MaxAttachDistance || Distance > MaxDistance) continue;

		const FVector ToPointDir = ToPoint.GetSafeNormal();
		const float Dot = FVector::DotProduct(CameraForward, ToPointDir);

		if (Dot < MinDot) continue;

		const float Score = Dot * (1.f - Distance / MaxDistance);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestPoint = Point;
		}
	}

	if (BestPoint)
	{
		// Debug sphere sur le point
		DrawDebugSphere(GetWorld(), BestPoint->AttachPoint->GetComponentLocation(),
			80.f, 16, FColor::Yellow, false, 0.f);

		// Draw detection cone
		DrawDebugCone(GetWorld(), CameraLocation, CameraForward, MaxDistance,
			FMath::DegreesToRadians(30.f), FMath::DegreesToRadians(30.f),
			16, FColor::Cyan, false, 0.f);

		CurrentGrapplePoint = BestPoint;
		return;
	}

	CurrentGrapplePoint = nullptr;
}

//////////////////////////////////////////////////////////////
// Input
//////////////////////////////////////////////////////////////

void UGrappleComponent::ToggleGrapple()
{
	if (!CurrentGrapplePoint || bIsGrappling || !OwnerCharacter) return;

	GrappleLocation = CurrentGrapplePoint->AttachPoint->GetComponentLocation();
	CableLength = FVector::Distance(OwnerCharacter->GetActorLocation(), GrappleLocation);

	bIsGrappling = true;

	Cable->SetVisibility(true);
	Cable->CableLength = CableLength;
	Cable->SetAttachEndTo(CurrentGrapplePoint, NAME_None);

	FColor Color = CurrentGrapplePoint->GrappleMode == EGrapplePointType::Swing ? FColor::Green : FColor::Orange;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, Color,
		CurrentGrapplePoint->GrappleMode == EGrapplePointType::Swing ? TEXT("Grapple: SWING") : TEXT("Grapple: PULL"));
}

void UGrappleComponent::DetachGrapple()
{
	bIsGrappling = false;
	CurrentGrapplePoint = nullptr;
	GrappleLocation = FVector::ZeroVector;
	CableLength = 0.f;

	if (Cable) Cable->SetVisibility(false);
}

//////////////////////////////////////////////////////////////
// Swing
//////////////////////////////////////////////////////////////

void UGrappleComponent::HandleSwing(float DeltaTime)
{
	if (!OwnerCharacter || !CurrentGrapplePoint) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	if (!MoveComp) return;

	const FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector ToPlayer = PlayerLocation - GrappleLocation;
	const float CurrentDistance = ToPlayer.Size();

	FVector Direction = ToPlayer.GetSafeNormal();

	// ======================================================
	// CONTRAINTE DE LONGUEUR DU CÂBLE (ANTI-BONCE)
	// ======================================================
	if (CurrentDistance > CableLength)
	{
		const float ExcessLength = CurrentDistance - CableLength;

		// Repositionnement doux (évite l’overshoot)
		FVector CorrectedLocation = PlayerLocation - Direction * ExcessLength;
		OwnerCharacter->SetActorLocation(CorrectedLocation, true);

		// Suppression de la vitesse qui éloigne du point (damping physique)
		float RadialSpeed = FVector::DotProduct(MoveComp->Velocity, Direction);
		if (RadialSpeed > 0.f)
		{
			MoveComp->Velocity -= Direction * RadialSpeed;
		}
	}

	// ======================================================
	// CONTRÔLE DU JOUEUR DANS LE SWING
	// ======================================================
	if (!MoveComp->IsMovingOnGround())
	{
		const FVector Input(
			OwnerCharacter->GetInputAxisValue("MoveForward"),
			OwnerCharacter->GetInputAxisValue("MoveRight"),
			0.f
		);

		const FVector Forward = OwnerCharacter->GetActorForwardVector();
		const FVector Right = OwnerCharacter->GetActorRightVector();

		FVector ControlForce = (Forward * Input.X + Right * Input.Y) * SwingControlForce;
		MoveComp->AddForce(ControlForce);

		// ==================================================
		// PUMPING (BALANÇOIRE)
		// ==================================================
		FVector Tangent = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
		float PumpInput = FVector::DotProduct(ControlForce.GetSafeNormal(), Tangent);

		if (PumpInput > 0.f)
		{
			FVector PumpImpulse = Tangent * PumpInput * PumpForce * DeltaTime;
			MoveComp->AddImpulse(PumpImpulse, true);
		}
	}

	// ======================================================
	// DAMPING GLOBAL (STABILITÉ)
	// ======================================================
	MoveComp->Velocity *= 0.998f;

	// ======================================================
	// DEBUG VITESSE
	// ======================================================
	FVector HorizontalVelocity = MoveComp->Velocity;
	HorizontalVelocity.Z = 0.f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("Swing Speed: %.0f"), HorizontalVelocity.Size())
		);
	}
}
