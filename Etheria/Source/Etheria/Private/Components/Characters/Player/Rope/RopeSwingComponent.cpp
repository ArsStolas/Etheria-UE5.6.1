#include "Components/Characters/Player/Rope/RopeSwingComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "DrawDebugHelpers.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Kismet/KismetMathLibrary.h"

URopeSwingComponent::URopeSwingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URopeSwingComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        MoveComp = OwnerCharacter->GetCharacterMovement();
        AttachComponent = OwnerCharacter->GetRopeAttachComponent();
        LockComponent = OwnerCharacter->GetRopeLockComponent();
        ConstraintComponent = OwnerCharacter->GetRopeConstraintComponent();
    }
    
    if (ConstraintComponent)
    {
       ConstraintComponent->OnRopeTensioned.AddDynamic(this, &URopeSwingComponent::OnRopeTensioned);
    }
}

void URopeSwingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bIsSwinging) UpdateSwing(DeltaTime);
}

void URopeSwingComponent::StartSwing()
{
    if (bIsSwinging || !LockComponent) return;

    SwingPoint = LockComponent->GetLockedPoint();
    if (!SwingPoint.IsValid()) return;

    InitialSwingLocation = OwnerCharacter->GetActorLocation(); // On verrouille la pos
    const FVector Anchor = SwingPoint->GetActorLocation();
    
    RopeLength = FVector::Dist(InitialSwingLocation, Anchor);
    SwingVelocity = MoveComp->Velocity;
    
    MaxVelocityReached = SwingVelocity.Size();
    bIsSwinging = true;
    bFirstFrame = true;
    
    // On passe en MOVE_Custom pour qu'Unreal n'applique aucune force de son côté
    MoveComp->SetMovementMode(MOVE_Custom); 
    
    if(ConstraintComponent) ConstraintComponent->DeactivateConstraint();

    PrimaryComponentTick.SetTickFunctionEnable(true);
    
    SWING_LOG(LogTemp, Warning, TEXT("SWING START: Length = %f | Initial Vel = %f | Player Pos = %s"), 
        RopeLength, SwingVelocity.Size(), *OwnerCharacter->GetActorLocation().ToString());
}

void URopeSwingComponent::StopSwing()
{
    if (!bIsSwinging) return;

    bIsSwinging = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);
    
    bFirstFrame = true; 

    if (MoveComp)
    {
        MoveComp->SetMovementMode(MOVE_Falling); // On redonne la main à Unreal
        MoveComp->Velocity = SwingVelocity;
    }

    if (ConstraintComponent && AttachComponent && AttachComponent->IsAttached())
    {
        if (ARopeAttachPoint* Point = LockComponent->GetLockedPoint())
        {
            // On réactive la contrainte de distance simple pour le swing "statique"
            ConstraintComponent->ActivateConstraint(Point, RopeLength);
        }
    }
    
    SWING_LOG(LogTemp, Warning, TEXT("SWING STOP: Final Vel = %f | Player Pos = %s"), 
        SwingVelocity.Size(), *OwnerCharacter->GetActorLocation().ToString());
}

void URopeSwingComponent::UpdateSwing(float DeltaTime)
{
    if (!SwingPoint.IsValid() || HasTouchedGround())
    {
        StopSwing();
        return;
    }

    const FVector AnchorLoc = SwingPoint->GetActorLocation();
    
    if(bFirstFrame)
    {
        OwnerCharacter->SetActorLocation(InitialSwingLocation, false);
        bFirstFrame = false;
    }

    FVector CurrentPos = OwnerCharacter->GetActorLocation();
    
    // --- CALCUL DES FORCES ---
    ApplyGravity(SwingVelocity, DeltaTime);
    ApplyAirResistance(SwingVelocity, DeltaTime);
    
    // On réduit encore l'input pour éviter l'effet "fusée"
    FVector RopeDir = (CurrentPos - AnchorLoc).GetSafeNormal();
    ApplyPlayerInputForce(SwingVelocity, RopeDir, DeltaTime);

    // --- INTEGRATION ---
    FVector NextPos = CurrentPos + (SwingVelocity * DeltaTime);

    // --- CONTRAINTE ---
    // On augmente la raideur pour éviter que le joueur s'éloigne
    SolveRopeConstraint(NextPos, SwingVelocity, AnchorLoc, DeltaTime);

    // --- APPLICATION ---
    FHitResult Hit;
    OwnerCharacter->SetActorLocation(NextPos, true, &Hit);
    
    if (Hit.IsValidBlockingHit())
    {
        SwingVelocity = FVector::VectorPlaneProject(SwingVelocity, Hit.Normal);
    }

    MoveComp->Velocity = SwingVelocity;
    
    float CurrentSpeed = SwingVelocity.Size();
    if (CurrentSpeed > MaxVelocityReached)
    {
        MaxVelocityReached = CurrentSpeed;
    }
    
    if (bShowDebug) DrawVisualDebug(AnchorLoc, NextPos, GetCameraInputDirection());
}

#pragma region "FORCES APPLICATION"

void URopeSwingComponent::ApplyGravity(FVector& CurrentVelocity, float DeltaTime)
{
    CurrentVelocity += FVector(0.f, 0.f, GetWorld()->GetGravityZ() * GravityScale) * DeltaTime;
}

void URopeSwingComponent::ApplyAirResistance(FVector& CurrentVelocity, float DeltaTime)
{
    CurrentVelocity *= FMath::Clamp(1.f - (AirDrag * DeltaTime), 0.f, 1.f);
}

void URopeSwingComponent::ApplyPlayerInputForce(
    FVector& CurrentVelocity,
    const FVector& RopeDirection,
    float DeltaTime)
{
    FVector InputDir = GetCameraInputDirection();
    const bool bHasInput = !InputDir.IsNearlyZero();
    if (!bHasInput)
        return;

    FVector TangentDir =
        FVector::VectorPlaneProject(InputDir, RopeDirection).GetSafeNormal();

    if (TangentDir.IsNearlyZero())
        return;

    const bool bGoingUp   = CurrentVelocity.Z > 0.f;
    const bool bGoingDown = CurrentVelocity.Z < 0.f;

    float AppliedForce = SwingForce;

    /* ===============================
       MODULATION EXISTANTE (SAFE)
       =============================== */

    if (bGoingUp)
    {
        AppliedForce *= 0.35f;
    }
    else if (bGoingDown)
    {
        AppliedForce *= 1.1f;
    }

    /* ===============================
       PUMP TIMING (BONUS PONCTUEL)
       =============================== */

    TryConsumePump(CurrentVelocity, TangentDir);

    /* ===============================
       CLAMP VITESSE MAX
       =============================== */

    const float SpeedAlongTangent =
        FVector::DotProduct(CurrentVelocity, TangentDir);

    if (SpeedAlongTangent > MaxSwingVelocity)
        return;

    CurrentVelocity += TangentDir * AppliedForce * DeltaTime;
}

#pragma endregion

#pragma region "PUMP CALCULATION"

bool URopeSwingComponent::TryConsumePump(
    const FVector& CurrentVelocity,
    const FVector& TangentDir)
{
    bPumpActive = false;

    // Reset quand on remonte clairement
    if (CurrentVelocity.Z > PumpResetVerticalSpeed)
    {
        bPumpConsumedThisSwing = false;
    }

    // Détection du passage du bas :
    const bool bPassedBottom =
        LastVerticalSpeed < 0.f &&
        CurrentVelocity.Z >= 0.f;

    LastVerticalSpeed = CurrentVelocity.Z;

    if (!bPassedBottom)
        return false;

    if (bPumpConsumedThisSwing)
        return false;

    // PUMP VALIDÉ
    bPumpConsumedThisSwing = true;
    bPumpActive = true;

    // Impulsion directe (PAS * DeltaTime)
    SwingVelocity += TangentDir * PumpImpulseStrength;

    return true;
}

#pragma endregion

#pragma region "CONSTRAINT SOLVER"

void URopeSwingComponent::SolveRopeConstraint(FVector& CurrentPosition, FVector& CurrentVelocity, const FVector& AnchorLocation, float DeltaTime)
{
    FVector ToPlayer = CurrentPosition - AnchorLocation;
    float CurrentDist = ToPlayer.Size();

    // Tolerance : si on est PRESQUE à la bonne distance, on ne force pas
    const float Tolerance = 5.f;

    if (CurrentDist > RopeLength + Tolerance)
    {
        FVector RopeDir = ToPlayer / CurrentDist;

        // MOINS agressif : utilise une interpolation moins forte
        // ConstraintStiffness = 20-30 est meilleur (au lieu de 60)
        // Plus bas = plus "mou" et naturel, plus haut = plus "sec"
        FVector TargetPos = AnchorLocation + (RopeDir * RopeLength);
        CurrentPosition = FMath::VInterpTo(CurrentPosition, TargetPos, DeltaTime, ConstraintStiffness);

        // Amortit la vélocité radiale quand la corde se tend
        float RadialSpeed = FVector::DotProduct(CurrentVelocity, RopeDir);
        if (RadialSpeed > 0.f)
        {
            // Réduit l'énergie : 0.8f = 20% de perte, 1.1f = 10% de perte
            CurrentVelocity -= RopeDir * RadialSpeed * 0.8f; 
        }
    }
}

#pragma endregion

/* ================= CONDITIONS ================= */

#pragma region "SWING CONDITIONS"

bool URopeSwingComponent::ShouldStartSwing() const
{
    // 1. Checks de base sur les composants
    if (!AttachComponent || !AttachComponent->IsAttached()) return false;
    if (!LockComponent || !LockComponent->HasLockedPoint()) return false;
    if (!MoveComp) return false;

    // 2. On vérifie que le point verrouillé est bien un point de type "Swing"
    const ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
    if (!Point || Point->AttachType != ERopeAttachType::Swing) return false;
    
    // 3. On ne swing pas si on est au sol (Walking) ou si on remonte trop vite (Jump initial)
    // On veut commencer le swing quand on tombe ou quand on est au sommet d'un saut
    if (MoveComp->IsMovingOnGround()) return false;
    
    // On vérifie que la vélocité Z n'est pas trop positive (on évite de trigger le swing en plein saut montant)
    if (MoveComp->Velocity.Z > FallingSpeedToStartSwing) return false; 

    // 4. Raycast de sécurité pour ne pas swinguer à 10cm du sol
    if (!IsFarEnoughFromGround()) return false;

    return true;
}

bool URopeSwingComponent::IsFarEnoughFromGround() const
{
    if(!OwnerCharacter) return false;
    const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
    if(!Capsule) return false;

    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    
    // On part du centre du perso vers le bas
    const FVector Start = OwnerCharacter->GetActorLocation();
    const FVector End   = Start - FVector(0,0, HalfHeight + MinHeightAboveGround);

    FHitResult Hit;
    FCollisionQueryParams Params; 
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    
    // Debug visuel du check de hauteur (Jaune si ok, Rouge si trop bas)
    if (bShowDebug)
    {
        SWING_DEBUG_LINE(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Yellow);
    }

    return !bHit;
}

bool URopeSwingComponent::HasTouchedGround() const
{
    const float HalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FHitResult Hit;
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start - FVector(0,0, HalfHeight + GroundStopDistance);
    
    FCollisionQueryParams Params; Params.AddIgnoredActor(OwnerCharacter);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && SwingVelocity.Z < 50.f;

}

FVector URopeSwingComponent::GetCameraInputDirection() const
{
    if (!OwnerCharacter || !OwnerCharacter->GetController()) return FVector::ZeroVector;
    
    FVector2D MoveInput(OwnerCharacter->GetHorizontalInput(), OwnerCharacter->GetVerticalInput());
    if (MoveInput.IsNearlyZero()) return FVector::ZeroVector;

    FRotator CamRot = OwnerCharacter->GetController()->GetControlRotation();
    CamRot.Pitch = 0.f; CamRot.Roll = 0.f;

    return (UKismetMathLibrary::GetForwardVector(CamRot) * MoveInput.Y + UKismetMathLibrary::GetRightVector(CamRot) * MoveInput.X).GetSafeNormal();
}

void URopeSwingComponent::OnRopeTensioned()
{
    // C'est ici que le signal envoyé par le ConstraintComponent arrive
    if(!bIsSwinging && ShouldStartSwing())
    {
       SWING_LOG(LogTemp, Log, TEXT("Rope Tensioned -> Starting Swing"));
       StartSwing();
    }
}

#pragma endregion

#pragma region "DEBUG VISUALS"

void URopeSwingComponent::DrawVisualDebug(const FVector& Anchor, const FVector& PlayerPos, const FVector& InputDir)
{
    // Lignes de debug dans le monde
    SWING_DEBUG_LINE(GetWorld(), Anchor, PlayerPos, FColor::Green); // Corde
    SWING_DEBUG_LINE(GetWorld(), PlayerPos, PlayerPos + SwingVelocity * 0.2f, FColor::Cyan); // Vecteur Vélocité
    
    if (!InputDir.IsNearlyZero())
    {
        SWING_DEBUG_LINE(GetWorld(), PlayerPos, PlayerPos + InputDir * 100.f, FColor::Yellow); // Direction Input
    }

    // --- UI SCREEN DEBUG ---
    FColor VelocityColor = (SwingVelocity.Size() > MaxSwingVelocity * 0.9f) ? FColor::Red : FColor::Cyan;

    SWING_SCREEN_MSG(1, FColor::White,  TEXT("=== ROPE SWING DEBUG ==="));
    SWING_SCREEN_MSG(2, VelocityColor, TEXT("Current Speed: %0.2f / %0.2f"), SwingVelocity.Size(), MaxSwingVelocity);
    SWING_SCREEN_MSG(3, FColor::Orange, TEXT("Max Speed This Swing: %0.2f"), MaxVelocityReached);
    SWING_SCREEN_MSG(4, FColor::Green,  TEXT("Rope Length: %0.2f"), RopeLength);
    SWING_SCREEN_MSG(5, FColor::Yellow, TEXT("Input Active: %s"), !InputDir.IsNearlyZero() ? TEXT("YES") : TEXT("NO"));
    
    FString ModeStr = (MoveComp->MovementMode == MOVE_Custom) ? TEXT("CUSTOM (Swing)") : TEXT("OTHER");
    SWING_SCREEN_MSG(6, FColor::White,  TEXT("Movement Mode: %s"), *ModeStr);
    
    // Pump status
    SWING_SCREEN_MSG(
        8,
        bPumpActive ? FColor::Green : FColor::Silver,
        TEXT("Pump Window: %s"),
        bPumpConsumedThisSwing ? TEXT("USED") : TEXT("READY")
    );

    if (bPumpActive)
    {
        SWING_DEBUG_LINE(
            GetWorld(),
            PlayerPos,
            PlayerPos + FVector(0,0,150.f),
            FColor::Green
        );
    }
}

#pragma endregion