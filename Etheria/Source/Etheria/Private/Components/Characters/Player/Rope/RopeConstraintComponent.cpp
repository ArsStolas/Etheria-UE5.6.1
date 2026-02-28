/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeConstraintComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Rope/Pulling/RopePullComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeConstraintComponent::URopeConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics; // Execute before physics
}

void URopeConstraintComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    MoveComp = OwnerCharacter->GetCharacterMovement();
    AttachComponent = OwnerCharacter->GetRopeAttachComponent();
}

void URopeConstraintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsActive || !Anchor.IsValid()) return;

    ApplyConstraint(DeltaTime);
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
    
    if (InAnchor->GetAttachType() == ERopeAttachType::Pull)
    {
        UPrimitiveComponent* PullTarget = Cast<UPrimitiveComponent>(InAnchor->GetMeshComponent());
        if (PullTarget)
        {
            CurrentPullRopeLength = FVector::Distance(OwnerCharacter->GetActorLocation(), PullTarget->GetComponentLocation());
            // Initialiser LastObjectLocation avec la vraie position dès l'activation
            // Évite le faux ObjectMoveDist énorme au premier frame (ZeroVector → position réelle)
            LastObjectLocation = PullTarget->GetComponentLocation();
        }
        else
        {
            CurrentPullRopeLength = InRopeLength;
        }
    }

    // Réinitialiser le timer de grâce pour la latence physique
    PullForceGraceTimer = 0.f;
    bIsObjectBlocked = false;
    BlockedAccumulator = 0.f;
    UnblockedAccumulator = 0.f;

    Anchor = InAnchor;
    RopeLength = InRopeLength;
    bIsActive = true;

    PrimaryComponentTick.SetTickFunctionEnable(true);
    
    // Sync visual cable length immediately
    if (AttachComponent)
    {
        AttachComponent->UpdateVisualCableLength(RopeLength, 0.f);
    }
    
    CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Activated on %s with length: %.2f"), 
       *InAnchor->GetName(), RopeLength);
}

void URopeConstraintComponent::DeactivateConstraint()
{
    bIsActive = false;
    Anchor.Reset();
    RopeLength = 0.f;
    LastPlayerObjectDistance = 0.f;
    LastObjectLocation = FVector::ZeroVector;
    CurrentPullRopeLength = 0.f;
    BlockedAccumulator = 0.f;
    UnblockedAccumulator = 0.f;
    bIsObjectBlocked = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);

    CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Deactivated"));
}

void URopeConstraintComponent::SetRopeLength(float NewLength)
{
    if (!AttachComponent || !Anchor.IsValid())
        return;

    float OldLength = RopeLength;

    const ERopeAttachType AttachType = Anchor->GetAttachType();

    float MinLength = AttachComponent->GetMinRopeLength();
    float MaxLength = AttachComponent->GetMaxRopeLength();

    // Override min for Pull
    if (AttachType == ERopeAttachType::Pull)
    {
        MinLength = AttachComponent->GetPullMinRopeLength();
    }

    MinLength = FMath::Max(10.f, MinLength);
    MaxLength = FMath::Max(MinLength + 10.f, MaxLength);

    RopeLength = FMath::Clamp(NewLength, MinLength, MaxLength);

    AttachComponent->UpdateVisualCableLength(RopeLength, 0.f);

    CONSTRAINT_LOG(LogTemp, Verbose,
        TEXT("[RopeConstraint] Length changed: %.2f -> %.2f (Min=%.2f Max=%.2f)"),
        OldLength,
        RopeLength,
        MinLength,
        MaxLength);
}

void URopeConstraintComponent::ResetPullState()
{
    PullForceGraceTimer = 0.f;
    BlockedAccumulator = 0.f;
    UnblockedAccumulator = 0.f;
    bIsObjectBlocked = false;
}

void URopeConstraintComponent::ApplyConstraint(float DeltaTime)
{
    ARopeAttachPoint* AnchorPtr = Anchor.Get();
    if (!AnchorPtr || !OwnerCharacter)
        return;

    CONSTRAINT_SCREEN_MSG(
        999,
        FColor::White,
        TEXT("MODE = %d"),
        Anchor->GetAttachType()
    );

    switch (Anchor->GetAttachType())
    {
    case ERopeAttachType::Pull:
        HandlePullConstraint(DeltaTime);
        break;

    case ERopeAttachType::Swing:
        ResetPullState();
        HandleSwingConstraint(DeltaTime);
    default:
        break;
    }
}

void URopeConstraintComponent::HandlePullConstraint(float DeltaTime)
{
    const FVector AnchorLoc = Anchor->GetActorLocation();
    const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

    FVector ToPlayer = PlayerLoc - AnchorLoc;
    const float CurrentDist = ToPlayer.Size();

    if (CurrentPullRopeLength <= 0.f)
        CurrentPullRopeLength = CurrentDist;

    float EffectiveRopeLength = CurrentPullRopeLength;

    UPrimitiveComponent* PullTarget =
        Cast<UPrimitiveComponent>(Anchor->GetMeshComponent());

    if (!PullTarget || !PullTarget->IsSimulatingPhysics())
        return;

    const bool bRopeIsTaut =
        CurrentDist > EffectiveRopeLength + KINDA_SMALL_NUMBER;

    if (!bRopeIsTaut)
    {
        ResetPullState();
        return;
    }

    const FVector RopeDir = ToPlayer / CurrentDist;

    float ObjectMass = PullTarget->GetMass();
    float PlayerMass = OwnerCharacter->GetCharacterMovement()->Mass;
    float MassRatio = ObjectMass / FMath::Max(PlayerMass, 1.f);

    float HeavyRatio = 1.5f;
    if (URopePullComponent* PullComp = OwnerCharacter->GetRopePullComponent())
        HeavyRatio = PullComp->GetHeavyObjectRatio();

    // ======================================
    // Block Detection
    // ======================================

    PullForceGraceTimer += DeltaTime;

    float DynamicGrace =
        PullForceGraceDuration * FMath::Clamp(MassRatio, 1.f, 3.f);

    const bool bGraceExpired =
        PullForceGraceTimer >= DynamicGrace;

    FVector ObjVelocity =
        PullTarget->GetPhysicsLinearVelocity();

    float RadialSpeed =
        FVector::DotProduct(ObjVelocity, -RopeDir);

    const bool bObjectNotMoving =
        bGraceExpired && RadialSpeed < 5.f;

    if (bObjectNotMoving)
    {
        BlockedAccumulator += DeltaTime;
        UnblockedAccumulator = 0.f;
    }
    else
    {
        UnblockedAccumulator += DeltaTime;
        BlockedAccumulator = 0.f;
        PullForceGraceTimer = 0.f;
    }

    if (bIsObjectBlocked &&
        UnblockedAccumulator >= UnblockedConfirmDelay)
        bIsObjectBlocked = false;

    if (!bIsObjectBlocked &&
        BlockedAccumulator >= BlockedConfirmDelay)
        bIsObjectBlocked = true;

    // ======================================
    // OBJECT FOLLOWS
    // ======================================

    if (!bIsObjectBlocked && MassRatio <= HeavyRatio)
    {
        const FVector TargetObjLoc =
            PlayerLoc - RopeDir * EffectiveRopeLength;

        const FVector Correction =
            TargetObjLoc - PullTarget->GetComponentLocation();

        FVector DesiredVel =
            Correction / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);

        FVector CurrentVel =
            PullTarget->GetPhysicsLinearVelocity();

        FVector NewVel =
            FMath::VInterpTo(CurrentVel, DesiredVel, DeltaTime, 8.f);

        PullTarget->SetPhysicsLinearVelocity(NewVel);

        if (CurrentDist < CurrentPullRopeLength)
            CurrentPullRopeLength = CurrentDist;

        CONSTRAINT_SCREEN_MSG(
            401,
            FColor::Green,
            TEXT("PULL FOLLOW | Vel=%.1f"),
            NewVel.Size()
        );
    }
    else
    {
        // ==================================
        // PLAYER CONSTRAINED
        // ==================================

        const FVector TargetLoc =
            AnchorLoc + RopeDir * EffectiveRopeLength;

        const FVector SmoothedLoc =
            FMath::Lerp(PlayerLoc, TargetLoc, ConstraintSmoothness);

        OwnerCharacter->SetActorLocation(SmoothedLoc, true);

        if (MoveComp)
        {
            FVector Vel = MoveComp->Velocity;
            float PlayerRadialSpeed =
                FVector::DotProduct(Vel, RopeDir);

            if (PlayerRadialSpeed > 0.f)
            {
                Vel -= RopeDir * PlayerRadialSpeed;
                MoveComp->Velocity = Vel;
            }
        }

        CONSTRAINT_SCREEN_MSG(
            402,
            FColor::Red,
            TEXT("PLAYER CONSTRAINED")
        );
    }
}

void URopeConstraintComponent::HandleSwingConstraint(float DeltaTime)
{
    
    const FVector AnchorLoc = Anchor->GetActorLocation();
    const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

    FVector ToPlayer = PlayerLoc - AnchorLoc;
    const float CurrentDist = ToPlayer.Size();

    float EffectiveRopeLength = RopeLength;
    
    if (CurrentDist <= EffectiveRopeLength || CurrentDist <= KINDA_SMALL_NUMBER)
        return;

    const FVector RopeDir = ToPlayer / CurrentDist;

    OnRopeTensioned.Broadcast();

    const FVector TargetLoc = AnchorLoc + RopeDir * EffectiveRopeLength;
    const FVector SmoothedLoc = FMath::Lerp(PlayerLoc, TargetLoc, ConstraintSmoothness);
    OwnerCharacter->SetActorLocation(SmoothedLoc, true);

    if (MoveComp)
    {
        FVector Vel = MoveComp->Velocity;
        float RadialSpeed = FVector::DotProduct(Vel, RopeDir);
        if (RadialSpeed > 0.f)
        {
            Vel -= RopeDir * RadialSpeed;
            MoveComp->Velocity = Vel;
        }
    }

    CONSTRAINT_SCREEN_MSG(402, FColor::Cyan,
        TEXT("SWING/STATIC | Dist=%.1f Rope=%.1f"), CurrentDist, EffectiveRopeLength);
    CONSTRAINT_LOG(LogTemp, VeryVerbose,
        TEXT("[RopeConstraint] Applied - Distance: %.2f | Rope Length: %.2f"),
        CurrentDist, EffectiveRopeLength);
}
