/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GliderComponent - Source
*/

#include "Components/Characters/Player/Glider/GliderComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Core/System/EtheriaGameplayTags.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
			StopGliding(false);
		break;

	case EGliderMode::Diving:
		HandleDive(DeltaTime);
		if (IsGrounded())
		{
			StopDiving(false);

			if (OwnerCharacter && OwnerCharacter->GetStateComponent())
			{
				int Hor = OwnerCharacter->GetHorizontalAxis();
				int Ver = OwnerCharacter->GetVerticalAxis();

				if (Hor != 0 || Ver != 0)
					OwnerCharacter->GetStateComponent()->SetMovementState(EtheriaTags::State_Movement_Grounded_Walking);
				else
					OwnerCharacter->GetStateComponent()->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
			}
		}
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
            StopGliding(true);
            break;
        case EGliderMode::Diving:
            StopDiving(true);
            break;
    }
}

void UGliderComponent::ToggleDiving()
{
	if (!OwnerCharacter) return;

	switch (CurrentMode)
	{
	case EGliderMode::Gliding:
		StartDiving();
		break;

	case EGliderMode::Diving:
		StopDiving(false);
		break;

	default:
		break;
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
	MoveComp->bOrientRotationToMovement = false; // Désactiver la rotation automatique
	MoveComp->RotationRate = FRotator(0.f, 250.f, 0.f);
	MoveComp->GravityScale = 0.0f;
	MoveComp->AirControl = 0.9f;
	MoveComp->BrakingDecelerationFalling = 350.f;
	MoveComp->MaxAcceleration = 1024.f;
	MoveComp->MaxWalkSpeed = 640.f;
	MoveComp->bUseControllerDesiredRotation = true;

	OnGlideStart.Broadcast();
}

void UGliderComponent::StopGliding(bool bManualStop)
{
	if (!OwnerCharacter) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Glider Deactivated"));

	if (OwnerCharacter->GetGliderVisual())
		OwnerCharacter->GetGliderVisual()->SetVisibility(false);

	ApplyOriginalSettings();
	if (bManualStop)
	{
		OnGlideStop.Broadcast();
	}
}

void UGliderComponent::StartDiving()
{
	if (!OwnerCharacter || CurrentMode != EGliderMode::Gliding) return;

	CurrentMode = EGliderMode::Diving;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

	if (OwnerCharacter->GetGliderVisual())
		OwnerCharacter->GetGliderVisual()->SetVisibility(false);
	
	MoveComp->bUseControllerDesiredRotation = false;
	MoveComp->bOrientRotationToMovement = false;

	MoveComp->GravityScale = 0.0f;
	MoveComp->AirControl = 1.0f;
	MoveComp->BrakingDecelerationFalling = 0.f;

	MoveComp->SetMovementMode(MOVE_Flying);
	OnDiveStart.Broadcast();
}

void UGliderComponent::StopDiving(bool bGoToGlide, bool bManualStop)
{
	if (!OwnerCharacter || CurrentMode != EGliderMode::Diving) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
		bGoToGlide ? TEXT("Dive → Glide") : TEXT("Dive → Normal"));

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

	// Remettre le joueur droit
	FRotator CurrentRot = OwnerCharacter->GetActorRotation();
	OwnerCharacter->SetActorRotation(FRotator(0.f, CurrentRot.Yaw, 0.f));

	// === Toujours reset les settings de base avant quoi que ce soit ===
	ApplyOriginalSettings();

	if (bGoToGlide)
	{
		// Passer en Gliding proprement
		StartGliding();
	}
	else
	{
		// Retour complet à la gravité naturelle
		MoveComp->SetMovementMode(MOVE_Falling);
		MoveComp->GravityScale = 1.0f;
		MoveComp->AirControl = 0.35f;
		MoveComp->BrakingDecelerationFalling = 200.f;
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->bOrientRotationToMovement = true;

		CurrentMode = EGliderMode::None;

		if (OwnerCharacter->GetGliderVisual())
			OwnerCharacter->GetGliderVisual()->SetVisibility(false);
	}

	CurrentDiveSpeed = MinDiveSpeed;
	if (bManualStop)
	{
		OnDiveStop.Broadcast();
	}
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

bool UGliderComponent::IsGrounded() const
{
	if (!OwnerCharacter) return false;

	FHitResult Hit;
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 100.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	bool bHit = OwnerCharacter->GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, QueryParams
	);

	return bHit;
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

	int Hor = OwnerCharacter->GetHorizontalAxis();
	int Ver = OwnerCharacter->GetVerticalAxis();

	const FRotator CamRot = OwnerCharacter->GetControlRotation();
	const FRotator YawRot(0.f, CamRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	FVector DesiredDir = (Forward * Ver + Right * Hor).GetSafeNormal();

	FVector TargetVel = MoveComp->Velocity;

	if (!DesiredDir.IsNearlyZero())
	{
		FVector TargetHorizontal = DesiredDir * GlideSpeed;
		FVector CurrentHorizontal = FVector(MoveComp->Velocity.X, MoveComp->Velocity.Y, 0.f);
		FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, TargetHorizontal, DeltaTime, GlideAccelInterp);

		TargetVel.X = NewHorizontal.X;
		TargetVel.Y = NewHorizontal.Y;

		// Orienter le joueur vers la direction de vol
		FRotator TargetRot = DesiredDir.Rotation();
		FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 4.f);
		OwnerCharacter->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f)); // yaw-only
	}
	else
	{
		// Pas d’input : amortir progressivement l’horizontale
		FVector CurrentHorizontal = FVector(MoveComp->Velocity.X, MoveComp->Velocity.Y, 0.f);
		FVector NewHorizontal = FMath::VInterpTo(CurrentHorizontal, FVector::ZeroVector, DeltaTime, 1.5f);
		TargetVel.X = NewHorizontal.X;
		TargetVel.Y = NewHorizontal.Y;
	}

	// Vertical : descente progressive
	TargetVel.Z = FMath::FInterpTo(MoveComp->Velocity.Z, -DescendingRate, DeltaTime, GlideDescentInterp);

	MoveComp->Velocity = TargetVel;
}

void UGliderComponent::HandleDive(float DeltaTime)
{
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

    // Lire les axes
    const int Hor = OwnerCharacter->GetHorizontalAxis();
    const int Ver = OwnerCharacter->GetVerticalAxis();

    // === PITCH / ROLL ===
    float TargetPitch = FMath::Clamp(-Ver * MaxPitchAngle, -MaxPitchAngle, MaxPitchAngle);
    float TargetRoll  = FMath::Clamp(Hor * MaxRollAngle, -MaxRollAngle, MaxRollAngle);

    // Interpolation de la rotation
    FRotator CurrentRot = OwnerCharacter->GetActorRotation();
    FRotator TargetRot  = FRotator(TargetPitch, CurrentRot.Yaw, TargetRoll);
    FRotator NewRot     = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 3.f);
    OwnerCharacter->SetActorRotation(NewRot);

    // === Gestion de la vitesse scalaire ===
    float PitchFactor = NewRot.Pitch / MaxPitchAngle;

    if (PitchFactor < -0.1f) // pique → accélère
        CurrentDiveSpeed += DiveAcceleration * (1.0f + 0.2f * FMath::Abs(PitchFactor)) * DeltaTime;
    else if (PitchFactor > 0.1f) // cabré → perd de la vitesse
        CurrentDiveSpeed -= DiveDeceleration * (0.6f + 0.4f * PitchFactor) * DeltaTime;
    else // à plat → légère perte
        CurrentDiveSpeed -= DiveDeceleration * 0.15f * DeltaTime;

    CurrentDiveSpeed = FMath::Clamp(CurrentDiveSpeed, MinDiveSpeed, MaxDiveSpeed * 1.3f);

    // === Yaw influencé par le Roll ===
    float RollFactor = NewRot.Roll / MaxRollAngle;
    if (FMath::Abs(RollFactor) > 0.1f)
    {
        float SpeedScale = FMath::Clamp(CurrentDiveSpeed / MaxDiveSpeed, 0.6f, 2.0f);
        float TurnRate   = RollFactor * TurnRateDive * SpeedScale;

        FRotator YawRot = OwnerCharacter->GetActorRotation();
        YawRot.Yaw += TurnRate * DeltaTime;
        OwnerCharacter->SetActorRotation(FRotator(NewRot.Pitch, YawRot.Yaw, NewRot.Roll));
        NewRot = OwnerCharacter->GetActorRotation();
    }

    // === Direction horizontale ===
    FRotator HorizontalRot(0.f, NewRot.Yaw, 0.f);
    FVector ForwardDir = FRotationMatrix(HorizontalRot).GetUnitAxis(EAxis::X);

    FVector Velocity = MoveComp->Velocity;
    Velocity.X = ForwardDir.X * CurrentDiveSpeed;
    Velocity.Y = ForwardDir.Y * CurrentDiveSpeed;

    // === Gravité arcade + portance ===
    const float GravityStrength = 400.f;        // gravité réduite pour arcade
    const float StallSpeed      = MaxDiveSpeed * 0.4f; // seuil de décrochage
    const float MaxLiftCoeff    = LiftFactor * 1.8f;   // portance boostée

    float LiftCoeff = 0.f;
    if (CurrentDiveSpeed > StallSpeed)
    {
        float SpeedRatio = (CurrentDiveSpeed - StallSpeed) / (MaxDiveSpeed - StallSpeed);
        SpeedRatio = FMath::Clamp(SpeedRatio, 0.f, 1.f);
        LiftCoeff = MaxLiftCoeff * SpeedRatio;
    }

    float LiftAccelZ = 0.f;
    if (PitchFactor > 0.f && LiftCoeff > 0.f)
    {
        float PitchGain = FMath::Clamp(PitchFactor, 0.f, 1.f);
        LiftAccelZ = LiftCoeff * PitchGain * (CurrentDiveSpeed * 0.5f);
    }

    // Accélération verticale finale
    float AccelZ = LiftAccelZ - GravityStrength;
    Velocity.Z += AccelZ * DeltaTime;

    // Clamp pour éviter les abus
    Velocity.Z = FMath::Clamp(Velocity.Z, -2400.f, 900.f);

    MoveComp->Velocity = Velocity;

    // === Debug ===
    if (GEngine)
    {
        FString Msg = FString::Printf(
            TEXT("Alt: %.0f | VZ: %.0f | Speed: %.0f | Stall: %.0f | LiftCoeff: %.2f | Pitch: %.1f°"),
            OwnerCharacter->GetActorLocation().Z,
            Velocity.Z,
            CurrentDiveSpeed,
            StallSpeed,
            LiftCoeff,
            NewRot.Pitch
        );

        FColor DebugColor = (Velocity.Z > 50.f) ? FColor::Green :
                            (Velocity.Z < -50.f) ? FColor::Red : FColor::White;

        GEngine->AddOnScreenDebugMessage(1, 0.f, DebugColor, Msg);
    }
}