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

    DETECTION_LOG(LogTemp, Log, TEXT("[RopeDetection] Component initialized"));
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

    DEBUG_ONLY(
        Stats.TotalPointsChecked = 0;
        Stats.FailedBroadPhase = 0;
        Stats.FailedValidation = 0;
    );

    ARopeAttachPoint* PreviousPoint = CurrentPoint.Get();

    float BestScore = -FLT_MAX;
    ARopeAttachPoint* BestPoint = nullptr;

    // === Player state check ===
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

            DEBUG_ONLY(Stats.TotalPointsChecked++);

            // === BROAD PHASE ===
            const FVector PointLoc = Point->GetActorLocation();
            const float DistSq = FVector::DistSquared(CachedCameraLocation, PointLoc);

            if (DistSq > CachedMaxDistSq)
            {
                DEBUG_ONLY(Stats.FailedBroadPhase++);
                continue;
            }

            // === VALIDATION ===
            FString FailReason;
            if (!IsValidPoint(Point, FailReason, PlayerLoc))
            {
                DEBUG_ONLY(
                    Stats.FailedValidation++;
                    
                    if (DebugVerbosity >= 2)
                    {
                        DETECTION_LOG(LogTemp, Verbose,
                            TEXT("[RopeDetection] Point '%s' invalid: %s"),
                            *Point->GetName(),
                            *FailReason);
                    }
                );
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

    // === STATE CHANGE ===
    if (BestPoint != PreviousPoint)
    {
        CurrentPoint = BestPoint;

        DETECTION_LOG(LogTemp, Log,
            TEXT("[RopeDetection] Detected point changed: %s"),
            BestPoint ? *BestPoint->GetName() : TEXT("None"));

        DEBUG_ONLY(
            if (DebugVerbosity >= 0)
            {
                DETECTION_SCREEN_MSG(150, FColor::Yellow,
                    TEXT("Rope Target: %s (Score: %.1f)"),
                    BestPoint ? *BestPoint->GetName() : TEXT("None"),
                    BestScore);
            }
        );

        OnDetectedPointChanged.Broadcast(BestPoint);
    }

    // === DEBUG VISUALS ===
    DEBUG_ONLY(
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
        }
        else if (DebugVerbosity >= 1)
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

        if (DebugVerbosity >= 2)
        {
            DETECTION_SCREEN_MSG(151, FColor::White,
                TEXT("[Detection Stats] Checked: %d | BroadPhase Failed: %d | Validation Failed: %d"),
                Stats.TotalPointsChecked,
                Stats.FailedBroadPhase,
                Stats.FailedValidation);
        }
    );
}

bool URopeDetectionComponent::IsValidPoint(ARopeAttachPoint* Point, FString& OutFailReason, const FVector& PlayerLoc) const
{
    if (!Point)
    {
        return false;
    }

    // Check 0: Player state (very fast)
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

    // Check 1: Direction (fast)
    const FVector ToPoint = (PointLoc - CachedCameraLocation).GetSafeNormal();
    const float DotProduct = FVector::DotProduct(CachedCameraForward, ToPoint);
    
    if (DotProduct < CachedMinCameraDot)
    {
        OutFailReason = "Outside Cone";
        return false;
    }

    // Check 2: Height (very fast) - except for Pull type (can be anywhere)
    const float HeightDelta = PointLoc.Z - PlayerLoc.Z;
    if (Point->GetAttachType() != ERopeAttachType::Pull && HeightDelta < MinHeightAbovePlayer)
    {
        OutFailReason = "Too Low";
        return false;
    }

    // Check 3: Line trace (most expensive - check last)
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
