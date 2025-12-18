/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeDetectionComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeDetectionComponent.h"

#include "Characters/Players/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeDetectionComponent::URopeDetectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void URopeDetectionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
    StateComponent = OwnerCharacter->GetStateComponent();

    UpdateCachedValues();
}

void URopeDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CurrentPoint.Reset();
    Super::EndPlay(EndPlayReason);
}

void URopeDetectionComponent::UpdateCachedValues()
{
    if (!OwnerCharacter || !Camera)
    {
        return;
    }

    CachedMaxDistSq = MaxDetectionDistance * MaxDetectionDistance;
    CachedMinCameraDot = MinCameraDot;
}

void URopeDetectionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeSinceLastScan += DeltaTime;
    if (TimeSinceLastScan < DetectionInterval)
    {
        return;
    }
    TimeSinceLastScan = 0.f;

    if (!OwnerCharacter || !Camera || !StateComponent)
    {
        return;
    }

    DetectAttachPoint();
}

void URopeDetectionComponent::DetectAttachPoint()
{
    CachedCameraLocation = Camera->GetComponentLocation();
    CachedCameraForward = Camera->GetForwardVector();

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
    if (bDebugMode && DebugVerbosity >= 2)
    {
        Stats.TotalPointsChecked = 0;
        Stats.FailedBroadPhase = 0;
        Stats.FailedValidation = 0;
    }
#endif

    ARopeAttachPoint* PreviousPoint = CurrentPoint.Get();

    float BestScore = -FLT_MAX;
    ARopeAttachPoint* BestPoint = nullptr;

    // === État joueur ===
    if (StateComponent->IsInLifeState(EtheriaTags::State_Life_Dead))
    {
        BestPoint = nullptr;
    }
    else
    {
        const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

        for (const TWeakObjectPtr<ARopeAttachPoint>& WeakPoint : ARopeAttachPoint::AllAttachPoints)
        {
            ARopeAttachPoint* Point = WeakPoint.Get();
            if (!Point)
            {
                continue;
            }

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
            if (bDebugMode && DebugVerbosity >= 2)
            {
                Stats.TotalPointsChecked++;
            }
#endif

            // === BROAD PHASE ===
            const FVector PointLoc = Point->GetActorLocation();
            const float DistSq = FVector::DistSquared(CachedCameraLocation, PointLoc);

            if (DistSq > CachedMaxDistSq)
            {
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
                if (bDebugMode && DebugVerbosity >= 2)
                {
                    Stats.FailedBroadPhase++;
                }
#endif
                continue;
            }

            // === VALIDATION ===
            FString FailReason;
            if (!IsValidPoint(Point, FailReason, PlayerLoc))
            {
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
                if (bDebugMode && DebugVerbosity >= 2)
                {
                    Stats.FailedValidation++;
                    
                    UE_LOG(LogTemp, Log,
                    TEXT("[RopeDetection] Point '%s' invalid: %s"),
                    *Point->GetName(),
                    *FailReason);
                }
#endif
                continue;
            }

            // === SCORING ===
            const FVector ToPoint = (PointLoc - CachedCameraLocation).GetSafeNormal();
            const float DotProduct = FVector::DotProduct(CachedCameraForward, ToPoint);

            const float Distance = FMath::Sqrt(DistSq);
            const float DistanceFactor = 1.f - (Distance / MaxDetectionDistance);

            const float DirectionScore = DotProduct * DotProduct;
            const float DistanceScore = DistanceFactor * DistanceFactor;

            const float Score =
                (DirectionScore * ScoringDirectionWeight * 1000.f) +
                (DistanceScore * ScoringDistanceWeight * 500.f);

            if (Score > BestScore)
            {
                BestScore = Score;
                BestPoint = Point;
            }
        }
    }

    // === CHANGEMENT D'ÉTAT ===
    if (BestPoint != PreviousPoint)
    {
        CurrentPoint = BestPoint;

        if (bDebugMode && DebugVerbosity >= 1)
        {
            UE_LOG(LogTemp, Log,
                TEXT("[RopeDetection] Detected point changed: %s"),
                BestPoint ? *BestPoint->GetName() : TEXT("None"));
        }

        OnDetectedPointChanged.Broadcast(BestPoint);
    }

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
    if (bDebugMode)
    {
        if (BestPoint && DebugVerbosity >= 0)
        {
            DrawDebugSphere(
                GetWorld(),
                BestPoint->GetActorLocation(),
                BestPoint->DetectionRadius,
                16,
                FColor::Green,
                false,
                DetectionInterval * 1.5f
            );
        } else if (DebugVerbosity >= 1)
        {
            DrawDebugCone(
                GetWorld(),
                CachedCameraLocation,
                CachedCameraForward,
                MaxDetectionDistance,
                FMath::DegreesToRadians(DetectionHalfAngle),
                FMath::DegreesToRadians(DetectionHalfAngle),
                16,
                FColor::Cyan,
                false,
                DetectionInterval * 1.5f
            );
        }
    }
#endif
}

bool URopeDetectionComponent::IsValidPoint(ARopeAttachPoint* Point, FString& OutFailReason, const FVector& PlayerLoc) const
{
    if (!Point)
    {
        return false;
    }

    // Check 0: État du joueur (très rapide)
    if (StateComponent)
    {
        if (StateComponent->IsInLifeState(EtheriaTags::State_Life_Dead))
        {
            OutFailReason = "Dead";
            return false;
        }

        if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
        {
            OutFailReason = "Diving";
            return false;
        }
    }

    const FVector PointLoc = Point->GetActorLocation();

    // Check 1: Direction (rapide)
    const FVector ToPoint = (PointLoc - CachedCameraLocation).GetSafeNormal();
    const float DotProduct = FVector::DotProduct(CachedCameraForward, ToPoint);
    
    if (DotProduct < CachedMinCameraDot)
    {
        OutFailReason = "Outside Cone";
        return false;
    }

    // Check 2: Hauteur (très rapide) - sauf pour les Pull (peuvent être n'importe où)
    const float HeightDelta = PointLoc.Z - PlayerLoc.Z;
    if (Point->GetAttachType() != ERopeAttachType::Pull && HeightDelta < MinHeightAbovePlayer)
    {
        OutFailReason = "Too Low";
        return false;
    }

    // Check 3: Line trace (plus coûteux - faire en dernier)
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter);
    Params.AddIgnoredActor(Point);

    const FVector TargetLoc = PointLoc + FVector(0, 0, Point->DetectionRadius * 0.5f);

    if (GetWorld()->LineTraceSingleByChannel(
        Hit,
        CachedCameraLocation,
        TargetLoc,
        ECC_Visibility,
        Params))
    {
        OutFailReason = "LOS Blocked";
        return false;
    }

    return true;
}
