/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GliderComponent - Source
*/

#include "Components/Characters/Player/Glider/GliderComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"

UGliderComponent::UGliderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGliderComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
}

void UGliderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter) return;

	switch (CurrentMode)
	{
		case EGliderMode::Gliding:
			HandleDescent(DeltaTime);
			if (OwnerCharacter->GetCharacterMovement()->IsWalking())
				StopGliding();
			break;

		case EGliderMode::Diving:
			HandleDive(DeltaTime);
			if (OwnerCharacter->GetCharacterMovement()->IsWalking())
				StopDiving();
			break;

		default:
			break;
	}
}

void UGliderComponent::ToggleGliding()
{
    if (!OwnerCharacter) return;

    switch (CurrentMode)
    {
        case EGliderMode::None:
            StartGliding();
            break;
        case EGliderMode::Gliding:
            StopGliding();
            break;
        case EGliderMode::Diving:
            StopDiving();
            StartGliding();
            break;
    }
}

void UGliderComponent::ToggleDiving()
{
	if (!OwnerCharacter) return;

	if (CurrentMode == EGliderMode::Diving)
	{
		StopDiving();
		StartGliding();
	}
	else if (CurrentMode == EGliderMode::Gliding)
	{
		StartDiving();
	}
}

void UGliderComponent::StartGliding()
{
	if (!OwnerCharacter || !CanStartGliding()) return;

	CurrentMode = EGliderMode::Gliding;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Glider Activated"));

	if (OwnerCharacter->GetGliderVisual())
		OwnerCharacter->GetGliderVisual()->SetVisibility(true);

	RecordOriginalSettings();

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	MoveComp->RotationRate = FRotator(0.f, 250.f, 0.f);
	MoveComp->GravityScale = 0.0f;
	MoveComp->AirControl = 0.9f;
	MoveComp->BrakingDecelerationFalling = 350.f;
	MoveComp->MaxAcceleration = 1024.f;
	MoveComp->MaxWalkSpeed = 640.f;
	MoveComp->bUseControllerDesiredRotation = true;
}

void UGliderComponent::StopGliding()
{
	if (!OwnerCharacter) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Glider Deactivated"));

	if (OwnerCharacter->GetGliderVisual())
		OwnerCharacter->GetGliderVisual()->SetVisibility(false);

	ApplyOriginalSettings();
}

void UGliderComponent::StartDiving()
{
	if (!OwnerCharacter || CurrentMode != EGliderMode::Gliding) return;

	CurrentMode = EGliderMode::Diving;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Diving Started"));

	if (OwnerCharacter->GetGliderVisual())
		OwnerCharacter->GetGliderVisual()->SetVisibility(false);

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	
	MoveComp->GravityScale = 0.0f;
	MoveComp->AirControl = 1.0f;
	MoveComp->BrakingDecelerationFalling = 0.f;
	
	MoveComp->SetMovementMode(MOVE_Flying);
}

void UGliderComponent::StopDiving()
{
	if (!OwnerCharacter || CurrentMode != EGliderMode::Diving) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Diving Stopped"));
	
	ApplyOriginalSettings();
}

bool UGliderComponent::CanStartGliding() const
{
    if (!OwnerCharacter) return false;

    FHitResult Hit;
    FVector TraceStart = OwnerCharacter->GetActorLocation();
    FVector TraceEnd = TraceStart - OwnerCharacter->GetActorUpVector() * MinimumHeight;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    bool bHit = OwnerCharacter->GetWorld()->LineTraceSingleByChannel(
        Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams
    );

    return (!bHit && OwnerCharacter->GetCharacterMovement()->IsFalling());
}

void UGliderComponent::RecordOriginalSettings()
{
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    OriginalOrientRotation = MoveComp->bOrientRotationToMovement;
    OriginalGravityScale = MoveComp->GravityScale;
    OriginalAirControl = MoveComp->AirControl;
    OriginalWalkingSpeed = MoveComp->MaxWalkSpeed;
    OriginalDeceleration = MoveComp->BrakingDecelerationFalling;
    OriginalAcceleration = MoveComp->MaxAcceleration;
    OriginalDesiredRotation = MoveComp->bUseControllerDesiredRotation;
}

void UGliderComponent::ApplyOriginalSettings()
{
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    MoveComp->bOrientRotationToMovement = OriginalOrientRotation;
    MoveComp->GravityScale = OriginalGravityScale;
    MoveComp->AirControl = OriginalAirControl;
    MoveComp->MaxWalkSpeed = OriginalWalkingSpeed;
    MoveComp->BrakingDecelerationFalling = OriginalDeceleration;
    MoveComp->MaxAcceleration = OriginalAcceleration;
    MoveComp->bUseControllerDesiredRotation = OriginalDesiredRotation;
    MoveComp->RotationRate = FRotator(0.f, 500.f, 0.f);
	MoveComp->SetMovementMode(MOVE_Falling);
	CurrentMode = EGliderMode::None;
}

void UGliderComponent::HandleDescent(float DeltaTime)
{
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

	FVector Vel = MoveComp->Velocity;
	Vel.Z = UKismetMathLibrary::FInterpTo(Vel.Z, -DescendingRate, DeltaTime, 3.f);
	MoveComp->Velocity = Vel;
}

void UGliderComponent::HandleDive(float DeltaTime)
{
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	FVector2D Input = OwnerCharacter->GetMoveInput();

	// === PITCH (Haut/Bas) ===
	float TargetPitch = FMath::Clamp(-Input.X * MaxPitchAngle, -MaxPitchAngle, MaxPitchAngle);

	// === ROLL (Gauche/Droite) ===
	float TargetRoll = FMath::Clamp(-Input.Y * MaxRollAngle, -MaxRollAngle, MaxRollAngle);

	// === Interpolation de la rotation ===
	FRotator CurrentRot = OwnerCharacter->GetActorRotation();
	FRotator TargetRot = FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll);
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 4.f);
	OwnerCharacter->SetActorRotation(NewRot);

	// === GESTION DE LA VITESSE ===
	float PitchFactor = NewRot.Pitch / MaxPitchAngle;

	if (PitchFactor < -0.1f) // Pique vers le bas
	{
		CurrentDiveSpeed += DiveAcceleration * FMath::Abs(PitchFactor) * DeltaTime;
	}
	else if (PitchFactor > 0.1f) // Monte
	{
		CurrentDiveSpeed -= DiveDeceleration * PitchFactor * 2.f * DeltaTime;
	}
	else // Vol horizontal
	{
		CurrentDiveSpeed -= DiveDeceleration * 0.3f * DeltaTime;
	}

	CurrentDiveSpeed = FMath::Clamp(CurrentDiveSpeed, MinDiveSpeed, MaxDiveSpeed);

	// === CALCUL DE LA VÉLOCITÉ HORIZONTALE ===
	FRotator HorizontalRot = FRotator(0.f, NewRot.Yaw, 0.f);
	FVector HorizontalDir = FRotationMatrix(HorizontalRot).GetUnitAxis(EAxis::X);
	FVector TargetVelocity = HorizontalDir * CurrentDiveSpeed;

	// === VÉLOCITÉ VERTICALE ===
	float VerticalSpeed = PitchFactor * CurrentDiveSpeed * 0.6f;
	float BaseGravity = -400.f;
	TargetVelocity.Z = VerticalSpeed + BaseGravity;

	// --- Gravité renforcée si pitch vers le haut et pas assez de vitesse ---
	if (PitchFactor > 0.2f && CurrentDiveSpeed < MinDiveSpeed * 1.5f)
	{
		float SpeedRatio = CurrentDiveSpeed / (MinDiveSpeed * 1.5f);
		TargetVelocity.Z += FMath::Lerp(-1500.f, 0.f, SpeedRatio);
	}

	// --- Gravité renforcée si vitesse très basse ---
	if (CurrentDiveSpeed < MinDiveSpeed * 0.8f)
	{
		TargetVelocity.Z += FMath::Lerp(-1200.f, 0.f, CurrentDiveSpeed / (MinDiveSpeed * 0.8f));
	}

	// --- Gravité renforcée si vitesse horizontale faible ---
	float HorizontalSpeed = FVector(MoveComp->Velocity.X, MoveComp->Velocity.Y, 0.f).Size();
	float MinHorizontalSpeed = 200.f;
	if (HorizontalSpeed < MinHorizontalSpeed)
	{
		TargetVelocity.Z += FMath::Lerp(-1500.f, 0.f, HorizontalSpeed / MinHorizontalSpeed);
	}

	// === APPLICATION DE LA VÉLOCITÉ ===
	MoveComp->Velocity = FMath::VInterpTo(MoveComp->Velocity, TargetVelocity, DeltaTime, 3.f);

	// === ROTATION DU YAW (Virage) ===
	if (FMath::Abs(TargetRoll) > 5.f)
	{
		float YawDelta = TargetRoll * 1.2f * DeltaTime;
		FRotator NewYawRot = OwnerCharacter->GetActorRotation();
		NewYawRot.Yaw += YawDelta;
		OwnerCharacter->SetActorRotation(NewYawRot);
	}

	// === DEBUG ===
	if (GEngine)
	{
		float CurrentVerticalSpeed = MoveComp->Velocity.Z;
		FString VerticalState = (CurrentVerticalSpeed > 50.f) ? TEXT("↑ MONTÉE") 
							   : (CurrentVerticalSpeed < -50.f) ? TEXT("↓ DESCENTE") 
							   : TEXT("→ STABLE");

		FString Msg = FString::Printf(
			TEXT("Alt: %.0f | VZ: %.0f %s | Speed: %.0f (%.0f%%) | Pitch: %.1f° | HSpeed: %.0f"),
			OwnerCharacter->GetActorLocation().Z,
			CurrentVerticalSpeed,
			*VerticalState,
			CurrentDiveSpeed,
			(CurrentDiveSpeed / MaxDiveSpeed) * 100.f,
			NewRot.Pitch,
			HorizontalSpeed
		);

		FColor DebugColor = FColor::White;
		if (CurrentVerticalSpeed > 50.f) DebugColor = FColor::Green;
		else if (CurrentVerticalSpeed < -50.f) DebugColor = FColor::Red;

		GEngine->AddOnScreenDebugMessage(1, 0.f, DebugColor, Msg);
	}
}
