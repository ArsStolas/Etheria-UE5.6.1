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
            CurrentPullRopeLength = FVector::Distance(OwnerCharacter->GetActorLocation(), PullTarget->GetComponentLocation());
        else
            CurrentPullRopeLength = InRopeLength;
    }

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

void URopeConstraintComponent::ApplyConstraint(float DeltaTime)
{
    const FVector AnchorLoc = Anchor->GetActorLocation();
    const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

    FVector ToPlayer = PlayerLoc - AnchorLoc;
    const float CurrentDist = ToPlayer.Size();

    float EffectiveRopeLength = RopeLength;
    if (Anchor->GetAttachType() == ERopeAttachType::Pull)
        EffectiveRopeLength = CurrentPullRopeLength;

    // =========================================================
    // MODE PULL
    // =========================================================
    if (Anchor->GetAttachType() == ERopeAttachType::Pull)
    {
        UPrimitiveComponent* PullTarget = Cast<UPrimitiveComponent>(Anchor->GetMeshComponent());
        if (!PullTarget || !PullTarget->IsSimulatingPhysics())
            return;

        float ObjectMass = PullTarget->GetMass();
        float PlayerMass = OwnerCharacter->GetCharacterMovement()->Mass;
        float MassRatio = ObjectMass / FMath::Max(PlayerMass, 1.f);

        float HeavyRatio = 1.5f;
        if (URopePullComponent* PullComp = OwnerCharacter->GetRopePullComponent())
            HeavyRatio = PullComp->GetHeavyObjectRatio();

        const bool bRopeIsTaut = CurrentDist > EffectiveRopeLength + KINDA_SMALL_NUMBER;
        const FVector RopeDir = bRopeIsTaut ? (ToPlayer / CurrentDist) : FVector::ZeroVector;

        if (MassRatio <= HeavyRatio)
        {
            const FVector CurrentObjectLoc = PullTarget->GetComponentLocation();
            const float ObjectMoveDist = FVector::Distance(CurrentObjectLoc, LastObjectLocation);

            // Bloqué = corde tendue ET objet quasi immobile
            const bool bObjectNotMoving = bRopeIsTaut && ObjectMoveDist < BlockDetectionSensitivity;

            if (bObjectNotMoving)
            {
                BlockedAccumulator += DeltaTime;
                UnblockedAccumulator = 0.f;
            }
            else
            {
                UnblockedAccumulator += DeltaTime;
                BlockedAccumulator = FMath::Max(0.f, BlockedAccumulator - DeltaTime * 2.f);
            }

            if (bIsObjectBlocked && UnblockedAccumulator >= UnblockedConfirmDelay)
                bIsObjectBlocked = false;

            if (!bIsObjectBlocked && BlockedAccumulator >= BlockedConfirmDelay)
                bIsObjectBlocked = true;

            LastObjectLocation = CurrentObjectLoc;

            CONSTRAINT_SCREEN_MSG(401,
                bIsObjectBlocked ? FColor::Orange : FColor::Green,
                TEXT("PULL | %s | ObjMove=%.2f Acc=%.2f Dist=%.1f Limit=%.1f"),
                bIsObjectBlocked ? TEXT("BLOCKED") : TEXT("Following"),
                ObjectMoveDist, BlockedAccumulator, CurrentDist, EffectiveRopeLength);

            if (!bRopeIsTaut)
                return;

            if (!bIsObjectBlocked)
            {
                // Objet suit le joueur — le ramener dans la limite
                const FVector TargetObjLoc = PlayerLoc - RopeDir * EffectiveRopeLength;
                PullTarget->SetWorldLocation(TargetObjLoc, true);

                FVector ObjVel = PullTarget->GetPhysicsLinearVelocity();
                float RadialSpeed = FVector::DotProduct(ObjVel, -RopeDir);
                if (RadialSpeed < 0.f)
                {
                    ObjVel -= (-RopeDir) * RadialSpeed;
                    PullTarget->SetPhysicsLinearVelocity(ObjVel);
                }
            }
            else
            {
                // Objet bloqué — contraindre le joueur
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
            }
        }
        else
        {
            // Trop lourd — bloquer le joueur directement
            LastObjectLocation = PullTarget->GetComponentLocation();

            if (!bRopeIsTaut)
                return;

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

            CONSTRAINT_SCREEN_MSG(401, FColor::Red,
                TEXT("PULL | TOO HEAVY | Dist=%.1f Limit=%.1f"),
                CurrentDist, EffectiveRopeLength);
        }

        return;
    }

    // =========================================================
    // MODE SWING / STATIC
    // =========================================================
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
