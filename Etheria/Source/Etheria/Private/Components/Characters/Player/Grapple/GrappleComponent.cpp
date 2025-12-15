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
	if (!OwnerCharacter) return;

	if (bIsGrappling)
	{
		DetachGrapple();
		return;
	}

	if (!CurrentGrapplePoint) return;

	GrappleLocation = CurrentGrapplePoint->AttachPoint->GetComponentLocation();
	CableLength = FVector::Distance(OwnerCharacter->GetActorLocation(), GrappleLocation);

	bIsGrappling = true;

	Cable->SetVisibility(true);
	Cable->CableLength = CableLength;
	Cable->SetAttachEndTo(CurrentGrapplePoint, NAME_None);

	FColor Color =
		CurrentGrapplePoint->GrappleMode == EGrapplePointType::Swing
		? FColor::Green
		: FColor::Orange;

	GEngine->AddOnScreenDebugMessage(
		-1, 2.f, Color,
		CurrentGrapplePoint->GrappleMode == EGrapplePointType::Swing
		? TEXT("Grapple: SWING")
		: TEXT("Grapple: PULL")
	);
}

void UGrappleComponent::DetachGrapple()
{
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->Velocity *= DetachVelocityPreserveRatio;
	}

	bIsGrappling = false;
	CurrentGrapplePoint = nullptr;
	GrappleLocation = FVector::ZeroVector;
	CableLength = 0.f;

	if (Cable)
		Cable->SetVisibility(false);
}

void UGrappleComponent::DetachGrappleWithJump()
{
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	if (!MoveComp) return;

	FHitResult GroundHit;
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, MinDetachHeightFromGround + 50.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	bool bNearGround = GetWorld()->LineTraceSingleByChannel(
		GroundHit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (bNearGround)
	{
		return;
	}

	FVector CurrentVelocity = MoveComp->Velocity;
	FVector HorizontalVelocity = CurrentVelocity;
	HorizontalVelocity.Z = 0.f;

	const float Speed = HorizontalVelocity.Size();

	if (Speed < MinSwingSpeedForBoost)
	{
		DetachGrapple();
		return;
	}

	FVector VelocityDir = HorizontalVelocity.GetSafeNormal();

	FVector CameraForward = OwnerCharacter->GetControlRotation().Vector();
	CameraForward.Z = 0.f;
	CameraForward.Normalize();

	FVector LaunchDir =
		(VelocityDir * (1.f - JumpForwardBias) +
		 CameraForward * JumpForwardBias).GetSafeNormal();

	MoveComp->Velocity *= DetachVelocityPreserveRatio;
	MoveComp->Velocity += LaunchDir * JumpDetachBoost;

	bIsGrappling = false;
	CurrentGrapplePoint = nullptr;
	GrappleLocation = FVector::ZeroVector;
	CableLength = 0.f;

	if (Cable)
		Cable->SetVisibility(false);

	// Debug
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor::Cyan,
			FString::Printf(
				TEXT("Jump Detach Boost | Speed: %.0f"),
				Speed
			)
		);
	}
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
	const float Distance = ToPlayer.Size();
	FVector Direction = ToPlayer.GetSafeNormal();

	// ======================================================
	// RIGIDITÉ PROGRESSIVE DU CÂBLE
	// ======================================================
	float TensionRatio = Distance / CableLength;
	float StiffnessAlpha = FMath::Clamp(
		(TensionRatio - CableStiffnessStartRatio) / (1.f - CableStiffnessStartRatio),
		0.f,
		1.f
	);

	// ======================================================
	// CONTRAINTE DE LONGUEUR (ANTI EXTENSION)
	// ======================================================
	if (Distance > CableLength)
	{
		float Excess = Distance - CableLength;

		// Repositionnement doux
		FVector CorrectedLocation = PlayerLocation - Direction * Excess * StiffnessAlpha;
		OwnerCharacter->SetActorLocation(CorrectedLocation, true);

		// Suppression de la vitesse radiale (anti bounce)
		float RadialSpeed = FVector::DotProduct(MoveComp->Velocity, Direction);
		if (RadialSpeed > 0.f)
		{
			FVector RadialVel = Direction * RadialSpeed;
			MoveComp->Velocity -= RadialVel * (1.f + CableDamping);
		}
	}

	// ======================================================
	// PUMPING RÉEL (GAIN D'INERTIE)
	// ======================================================
	if (!MoveComp->IsMovingOnGround())
	{
		FVector Input(
			OwnerCharacter->GetInputAxisValue("MoveForward"),
			OwnerCharacter->GetInputAxisValue("MoveRight"),
			0.f
		);

		if (!Input.IsNearlyZero())
		{
			// Direction radiale (câble)
			FVector RadialDir = (PlayerLocation - GrappleLocation).GetSafeNormal();

			// Vitesse actuelle
			FVector Velocity = MoveComp->Velocity;

			// Projection tangentielle (clé du swing)
			FVector TangentialVel = Velocity - FVector::DotProduct(Velocity, RadialDir) * RadialDir;

			float TangentialSpeed = TangentialVel.Size();

			if (TangentialSpeed > 1.f)
			{
				FVector TangentDir = TangentialVel.GetSafeNormal();

				// Input aligné avec le mouvement
				float PumpInput = FVector::DotProduct(
					(OwnerCharacter->GetActorForwardVector() * Input.X +
					 OwnerCharacter->GetActorRightVector() * Input.Y).GetSafeNormal(),
					TangentDir
				);

				if (PumpInput > 0.f)
				{
					// Plus la corde est tendue, plus ça boost
					float TensionFactor = FMath::Clamp(Distance / CableLength, 0.6f, 1.f);

					float SpeedGain =
						PumpForce *
						PumpInput *
						TensionFactor *
						DeltaTime;

					// Injection directe de vitesse (IMPORTANT)
					MoveComp->Velocity += TangentDir * SpeedGain;
				}
			}
		} /*else
		{
			ApplyNoInputDamping(MoveComp, GrappleLocation, DeltaTime);
		}*/
	}

	// ======================================================
	// LIMITE DE VITESSE (ANTI NUCLEAR)
	// ======================================================
	FVector HorizontalVel = MoveComp->Velocity;
	HorizontalVel.Z = 0.f;

	if (HorizontalVel.Size() > MaxSwingSpeed)
	{
		FVector Clamped = HorizontalVel.GetSafeNormal() * MaxSwingSpeed;
		MoveComp->Velocity.X = Clamped.X;
		MoveComp->Velocity.Y = Clamped.Y;
	}

	// ======================================================
	// AIR DAMPING (STABILITÉ GLOBALE)
	// ======================================================
	if (MoveComp->IsFalling())
	{
		MoveComp->Velocity -= MoveComp->Velocity * (1.f - NoInputSwingDamping) * DeltaTime;
	}

	// ======================================================
	// DEBUG
	// ======================================================
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 0.f, FColor::Green,
			FString::Printf(TEXT("Swing Speed: %.0f"), HorizontalVel.Size())
		);
	}
}

void UGrappleComponent::ApplyNoInputDamping(UCharacterMovementComponent* MoveComp, const FVector& InGrappleLocation, float DeltaTime)
{
	if (!MoveComp) return;

	const FVector PlayerLocation = MoveComp->GetOwner()->GetActorLocation();
	const FVector RadialDir = (PlayerLocation - InGrappleLocation).GetSafeNormal();

	FVector Velocity = MoveComp->Velocity;

	// Décomposition
	float RadialSpeed = FVector::DotProduct(Velocity, RadialDir);
	FVector RadialVel = RadialDir * RadialSpeed;
	FVector TangentialVel = Velocity - RadialVel;

	// Damping tangentielle progressif
	float TangentialSpeed = TangentialVel.Size();
	if (TangentialSpeed > KINDA_SMALL_NUMBER)
	{
		// Damping exponentiel plus doux et indépendant du framerate
		float DampingFactor = FMath::Pow(NoInputSwingDamping, DeltaTime * 60.f);

		// On ne jamais écraser la tangente à 0 directement
		TangentialVel *= DampingFactor;

		// Optionnel : un petit pull radial pour éviter l'effet flottement
		TangentialVel += RadialVel * 0.01f; // ajustable
	}

	// Recomposition
	MoveComp->Velocity = RadialVel + TangentialVel;
}
