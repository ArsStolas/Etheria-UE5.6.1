/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeSwingComponent - Source
*/

#include "Components/Characters/Player/Rope/Swinging//RopeSwingComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "DrawDebugHelpers.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Core/System/EtheriaGameplayTags.h"
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

    // Guard : si le swing s'est arrêté mais que le tick tourne encore, on se coupe proprement
    if (!bIsSwinging)
    {
        PrimaryComponentTick.SetTickFunctionEnable(false);
        SWING_LOG(LogTemp, Warning, TEXT("[RopeSwing] Tick DISABLED (guard) — bIsSwinging=false, shutting down"));
        return;
    }

    UpdateSwing(DeltaTime);
}

void URopeSwingComponent::StartSwing()
{
    if (bIsSwinging || !LockComponent) return;

    SwingPoint = LockComponent->GetLockedPoint();
    if (!SwingPoint.IsValid()) return;
    
    if (OwnerCharacter)
    {
        OwnerCharacter->GetStateComponent()->SetMovementState(EtheriaTags::State_Movement_Rope_Swinging);
    }

    InitialSwingLocation = OwnerCharacter->GetActorLocation();
    const FVector Anchor = SwingPoint->GetActorLocation();
    
    BaseRopeLength = FVector::Dist(InitialSwingLocation, Anchor);
    EffectiveRopeLength = BaseRopeLength;
    RopeLength = EffectiveRopeLength;

    SwingVelocity = MoveComp->Velocity;
    
    MaxVelocityReached = SwingVelocity.Size();
    bIsSwinging = true;
    bFirstFrame = true;
    
    MoveComp->SetMovementMode(MOVE_Custom); 
    
    if(ConstraintComponent) ConstraintComponent->DeactivateConstraint();

    PrimaryComponentTick.SetTickFunctionEnable(true);
    SWING_LOG(LogTemp, Log, TEXT("[RopeSwing] Tick ENABLED — swing started on %s | Length=%.2f | InitVel=%.2f"),
        *SwingPoint->GetName(), RopeLength, SwingVelocity.Size());
    
    OnSwingStarted.Broadcast();
    
    SWING_LOG(LogTemp, Warning, TEXT("SWING START: Length = %f | Initial Vel = %f | Player Pos = %s"), 
        RopeLength, SwingVelocity.Size(), *OwnerCharacter->GetActorLocation().ToString());
}

void URopeSwingComponent::StopSwing(bool bDetaching)
{
    if (!bIsSwinging) return;
    
    if (OwnerCharacter)
    {
        OwnerCharacter->GetStateComponent()->SetMovementState(EtheriaTags::State_Movement_Rope_Attached);
    }

    bIsSwinging = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);
    SWING_LOG(LogTemp, Log, TEXT("[RopeSwing] Tick DISABLED — swing stopped | FinalVel=%.2f | PlayerPos=%s"),
        SwingVelocity.Size(), *OwnerCharacter->GetActorLocation().ToString());
    
    bFirstFrame = true; 

    if (MoveComp)
    {
        MoveComp->SetMovementMode(MOVE_Falling);
        MoveComp->Velocity = SwingVelocity;
    }

    // Ne pas réactiver le constraint si on est en train de se détacher complètement
    // (ex: TryJumpOffRope) — DetachRope s'occupera de le désactiver juste après,
    // ce qui évite une frame fantôme où le constraint bloque la vélocité de lancement
    if (!bDetaching && ConstraintComponent && AttachComponent && AttachComponent->IsAttached())
    {
        if (ARopeAttachPoint* Point = LockComponent->GetLockedPoint())
        {
            ConstraintComponent->ActivateConstraint(Point, RopeLength);
        }
    }
    
    OnSwingStopped.Broadcast();
    
    SWING_LOG(LogTemp, Warning, TEXT("SWING STOP: Final Vel = %f | Player Pos = %s"), 
        SwingVelocity.Size(), *OwnerCharacter->GetActorLocation().ToString());
}

#pragma region "SWING UPDATE"

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
    
    // --- FORCE CALCULATIONS ---
    ApplyGravity(SwingVelocity, DeltaTime);
    ApplyAirResistance(SwingVelocity, DeltaTime);
    
    FVector RopeDir = (CurrentPos - AnchorLoc).GetSafeNormal();
    ApplyPlayerInputForce(SwingVelocity, RopeDir, DeltaTime);

    // --- INTEGRATION ---
    FVector NextPos = CurrentPos + (SwingVelocity * DeltaTime);
    
    // --- DYNAMIC SLACK ---
    UpdateDynamicSlack(DeltaTime, RopeDir);
    
    // --- CONSTRAINT ---
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
    
    DrawVisualDebug(AnchorLoc, NextPos, GetCameraInputDirection());
}

void URopeSwingComponent::UpdateDynamicSlack(float DeltaTime, const FVector& RopeDir)
{
    if (bClimbInputActive)
    {
        EffectiveRopeLength = BaseRopeLength;
        RopeLength = EffectiveRopeLength;
        return;
    }
    
    const float VerticalSpeed = SwingVelocity.Z;

    const float DotDown =
        FVector::DotProduct(-RopeDir, FVector::UpVector);

    const float AngleFactor = FMath::Clamp(DotDown, 0.f, 1.f);

    float TargetSlack = 0.f;

    if (VerticalSpeed > SlackVerticalSpeedThreshold)
    {
        TargetSlack =
            MaxSlackLength *
            AngleFactor *
            FMath::Clamp(VerticalSpeed / 600.f, 0.f, 1.f);
    }

    const float TargetEffectiveLength = BaseRopeLength + TargetSlack;

    const float InterpSpeed =
        (TargetEffectiveLength > EffectiveRopeLength)
            ? SlackInterpSpeed
            : SlackReleaseSpeed;

    EffectiveRopeLength = FMath::FInterpTo(
        EffectiveRopeLength,
        TargetEffectiveLength,
        DeltaTime,
        InterpSpeed
    );

    RopeLength = EffectiveRopeLength;
}

#pragma endregion

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

    if (bGoingUp)
    {
        AppliedForce *= 0.35f;
    }
    else if (bGoingDown)
    {
        AppliedForce *= 1.1f;
    }

    TryConsumePump(CurrentVelocity, TangentDir);

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

    if (CurrentVelocity.Z > PumpResetVerticalSpeed)
    {
        bPumpConsumedThisSwing = false;
    }

    const bool bPassedBottom =
        LastVerticalSpeed < 0.f &&
        CurrentVelocity.Z >= 0.f;

    LastVerticalSpeed = CurrentVelocity.Z;

    if (!bPassedBottom)
        return false;

    if (bPumpConsumedThisSwing)
        return false;

    bPumpConsumedThisSwing = true;
    bPumpActive = true;

    SwingVelocity += TangentDir * PumpImpulseStrength;

    return true;
}

#pragma endregion

#pragma region "SLACK DYNAMIQUE"

void URopeSwingComponent::SetBaseRopeLength(float NewBaseLength)
{
    BaseRopeLength = NewBaseLength;

    EffectiveRopeLength = FMath::Max(
        EffectiveRopeLength,
        BaseRopeLength
    );
}

bool URopeSwingComponent::TryJumpOffRope(FVector& OutLaunchVelocity)
{
    if (!bIsSwinging)
        return false;

    const float CurrentSpeed = SwingVelocity.Size();

    if (CurrentSpeed < MinSpeedToDetach)
    {
        SWING_LOG(LogTemp, Warning, TEXT("Jump blocked: not enough swing speed (%.0f)"), CurrentSpeed);
        return false;
    }

    FVector LaunchDir = SwingVelocity.GetSafeNormal();

    FVector BoostVelocity = SwingVelocity * JumpBoostMultiplier;

    BoostVelocity += FVector::UpVector * JumpUpwardBoost;

    OutLaunchVelocity = BoostVelocity;

    StopSwing(true);

    SWING_LOG(LogTemp, Warning, TEXT("Jump off rope! Speed=%.0f"), CurrentSpeed);

    return true;
}

#pragma endregion

#pragma region "CONSTRAINT SOLVER"

void URopeSwingComponent::SolveRopeConstraint(FVector& CurrentPosition, FVector& CurrentVelocity, const FVector& AnchorLocation, float DeltaTime)
{
    FVector ToPlayer = CurrentPosition - AnchorLocation;
    float CurrentDist = ToPlayer.Size();

    const float Tolerance = 5.f;

    if (CurrentDist > RopeLength + Tolerance)
    {
        FVector RopeDir = ToPlayer / CurrentDist;

        FVector TargetPos = AnchorLocation + (RopeDir * RopeLength);
        CurrentPosition = FMath::VInterpTo(CurrentPosition, TargetPos, DeltaTime, ConstraintStiffness);

        float RadialSpeed = FVector::DotProduct(CurrentVelocity, RopeDir);
        if (RadialSpeed > 0.f)
        {
            CurrentVelocity -= RopeDir * RadialSpeed * 0.8f; 
        }
    }
}

#pragma endregion

/* ================= CONDITIONS ================= */

#pragma region "SWING CONDITIONS"

bool URopeSwingComponent::ShouldStartSwing() const
{
    if (!AttachComponent || !AttachComponent->IsAttached()) return false;
    if (!LockComponent || !LockComponent->HasLockedPoint()) return false;
    if (!MoveComp) return false;

    const ARopeAttachPoint* Point = LockComponent->GetLockedPoint();
    if (!Point || Point->AttachType != ERopeAttachType::Swing) return false;
    
    if (MoveComp->IsMovingOnGround()) return false;
    
    if (MoveComp->Velocity.Z > FallingSpeedToStartSwing) return false; 

    if (!IsFarEnoughFromGround()) return false;

    return true;
}

bool URopeSwingComponent::IsFarEnoughFromGround() const
{
    if(!OwnerCharacter) return false;
    const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
    if(!Capsule) return false;

    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    
    const FVector Start = OwnerCharacter->GetActorLocation();
    const FVector End   = Start - FVector(0,0, HalfHeight + MinHeightAboveGround);

    FHitResult Hit;
    FCollisionQueryParams Params; 
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    
    SWING_DEBUG_LINE(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Yellow);

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
    SWING_DEBUG_LINE(GetWorld(), Anchor, PlayerPos, FColor::Green);
    SWING_DEBUG_LINE(GetWorld(), PlayerPos, PlayerPos + SwingVelocity * 0.2f, FColor::Cyan);
    
    if (!InputDir.IsNearlyZero())
    {
        SWING_DEBUG_LINE(GetWorld(), PlayerPos, PlayerPos + InputDir * 100.f, FColor::Yellow);
    }

    FColor VelocityColor = (SwingVelocity.Size() > MaxSwingVelocity * 0.9f) ? FColor::Red : FColor::Cyan;

    SWING_SCREEN_MSG(1, FColor::White,  TEXT("=== ROPE SWING DEBUG ==="));
    SWING_SCREEN_MSG(2, VelocityColor, TEXT("Current Speed: %0.2f / %0.2f"), SwingVelocity.Size(), MaxSwingVelocity);
    SWING_SCREEN_MSG(3, FColor::Orange, TEXT("Max Speed This Swing: %0.2f"), MaxVelocityReached);
    SWING_SCREEN_MSG(4, FColor::Green,  TEXT("Rope Length: %0.2f"), RopeLength);
    SWING_SCREEN_MSG(5, FColor::Yellow, TEXT("Input Active: %s"), !InputDir.IsNearlyZero() ? TEXT("YES") : TEXT("NO"));
    
    FString ModeStr = (MoveComp->MovementMode == MOVE_Custom) ? TEXT("CUSTOM (Swing)") : TEXT("OTHER");
    SWING_SCREEN_MSG(6, FColor::White,  TEXT("Movement Mode: %s"), *ModeStr);
    
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
    
    SWING_SCREEN_MSG(
        9,
        FColor::Purple,
        TEXT("Slack: %+0.1f"),
        RopeLength - BaseRopeLength
    );
}

#pragma endregion