/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: AWindStreamZone - Source
 */

#include "World/Environment/WindStreamZone.h"

#include "Characters/Players/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogWindStream, Log, All);

#if UE_BUILD_SHIPPING
    #define WIND_LOG(Verbosity, Format, ...)
    #define WIND_SCREEN(Key, Color, Format, ...)
#else
    #define WIND_LOG(Verbosity, Format, ...) \
        if (bWindDebugMode) UE_LOG(LogWindStream, Verbosity, Format, ##__VA_ARGS__)
    #define WIND_SCREEN(Key, Color, Format, ...) \
        if (bWindDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

AWindStreamZone::AWindStreamZone()
{
    PrimaryActorTick.bCanEverTick = true;

    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    Spline->SetMobility(EComponentMobility::Movable);
    RootComponent = Spline;
}

void AWindStreamZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (IsValid(this) && !IsUnreachable())
    {
        RebuildVisuals();
        RebuildCollisionCapsules();
    }
}

void AWindStreamZone::BeginPlay()
{
    Super::BeginPlay();

    RebuildVisuals();
    RebuildCollisionCapsules();

    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule) continue;
        Capsule->SetHiddenInGame(!bWindDebugMode);
    }

    WIND_LOG(Log, TEXT("[WindStream] Debug enabled | Radius=%.1f Speed=%.1f SplinePoints=%d"),
        StreamRadius,
        StreamSpeed,
        Spline ? Spline->GetNumberOfSplinePoints() : 0);
}

void AWindStreamZone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    WIND_SCREEN(20, PlayersInStream.Num() > 0 ? FColor::Green : FColor::Orange,
        TEXT("[WindStream] Players:%d | Caps:%d | FX:%d/%d | Speed:%.0f | Radius:%.0f"),
        PlayersInStream.Num(),
        CollisionCapsules.Num(),
        StreamNiagaraComponents.Num(),
        BoundaryNiagaraComponents.Num(),
        StreamSpeed,
        StreamRadius);

    if (bWindDebugMode)
    {
        DrawWindDebug();
    }

    if (PlayersInStream.Num() == 0)
    {
        return;
    }

    TArray<APlayerCharacter*> InvalidPlayers;
    for (APlayerCharacter* Player : PlayersInStream)
    {
        if (!IsValid(Player))
        {
            InvalidPlayers.Add(Player);
            continue;
        }

        if (!IsPlayerInDiveMode(Player))
        {
            PlayerWindUseTimes.FindOrAdd(Player) = 0.f;
            PlayerSplineDistances.Remove(Player);
            PlayerStreamDirectionSigns.Remove(Player);
            continue;
        }

        ApplyWindEffect(Player, DeltaTime);

        if (bWindDebugMode)
        {
            const float SplineDistance = PlayerSplineDistances.Contains(Player)
                ? PlayerSplineDistances[Player]
                : GetClosestSplineDistance(Player->GetActorLocation());
            const FVector ClosestPoint = Spline->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
            const FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World).GetSafeNormal();

            DrawDebugSphere(GetWorld(), ClosestPoint, 40.f, 12, FColor::Cyan, false, 0.f, 0, 2.f);
            DrawDebugLine(GetWorld(), Player->GetActorLocation(), ClosestPoint, FColor::Blue, false, 0.f, 0, 1.f);
            DrawDebugDirectionalArrow(GetWorld(), ClosestPoint, ClosestPoint + Tangent * 250.f, 50.f, FColor::Green, false, 0.f, 0, 3.f);
        }
    }

    for (APlayerCharacter* Player : InvalidPlayers)
    {
        PlayersInStream.Remove(Player);
        PlayerWindUseTimes.Remove(Player);
        PlayerSplineDistances.Remove(Player);
        PlayerStreamDirectionSigns.Remove(Player);
    }
}

void AWindStreamZone::DestroyVisualComponents()
{
    for (UNiagaraComponent* NiagaraComponent : StreamNiagaraComponents)
    {
        if (!NiagaraComponent) continue;
        NiagaraComponent->DeactivateImmediate();
        NiagaraComponent->UnregisterComponent();
        RemoveInstanceComponent(NiagaraComponent);
        NiagaraComponent->DestroyComponent();
    }

    for (UNiagaraComponent* NiagaraComponent : BoundaryNiagaraComponents)
    {
        if (!NiagaraComponent) continue;
        NiagaraComponent->DeactivateImmediate();
        NiagaraComponent->UnregisterComponent();
        RemoveInstanceComponent(NiagaraComponent);
        NiagaraComponent->DestroyComponent();
    }

    for (UNiagaraComponent* NiagaraComponent : BoundaryRingNiagaraComponents)
    {
        if (!NiagaraComponent) continue;
        NiagaraComponent->DeactivateImmediate();
        NiagaraComponent->UnregisterComponent();
        RemoveInstanceComponent(NiagaraComponent);
        NiagaraComponent->DestroyComponent();
    }

    StreamNiagaraComponents.Empty();
    BoundaryNiagaraComponents.Empty();
    BoundaryRingNiagaraComponents.Empty();
}

void AWindStreamZone::RebuildVisuals()
{
    DestroyVisualComponents();

    if (Spline->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    if (!StreamNiagaraSystem && !BoundaryNiagaraSystem && !BoundaryRingNiagaraSystem)
    {
        return;
    }

    const float TotalLength = Spline->GetSplineLength();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    const float SegmentLength = TotalLength / FMath::Max(NumVisualSegments, 1);

    for (int32 Index = 0; Index < NumVisualSegments; ++Index)
    {
        const float D0 = Index * SegmentLength;
        const float D1 = (Index + 1) * SegmentLength;

        if (StreamNiagaraSystem)
        {
            UNiagaraComponent* StreamNiagara = NewObject<UNiagaraComponent>(this, *FString::Printf(TEXT("WindStreamFX_%d"), Index));
            StreamNiagara->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            StreamNiagara->SetMobility(EComponentMobility::Movable);
            StreamNiagara->SetupAttachment(RootComponent);
            StreamNiagara->SetAbsolute(true, true, true);
            StreamNiagara->SetAsset(StreamNiagaraSystem);
            StreamNiagara->SetAutoActivate(false);
            StreamNiagara->SetHiddenInGame(false);
            StreamNiagara->SetVisibility(true, true);
            AddInstanceComponent(StreamNiagara);
            StreamNiagara->RegisterComponent();
            ConfigureNiagaraComponent(StreamNiagara, Index, D0, D1, false);
            StreamNiagara->Activate(true);
            StreamNiagaraComponents.Add(StreamNiagara);
        }

        if (BoundaryNiagaraSystem)
        {
            UNiagaraComponent* BoundaryNiagara = NewObject<UNiagaraComponent>(this, *FString::Printf(TEXT("WindBoundaryFX_%d"), Index));
            BoundaryNiagara->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            BoundaryNiagara->SetMobility(EComponentMobility::Movable);
            BoundaryNiagara->SetupAttachment(RootComponent);
            BoundaryNiagara->SetAbsolute(true, true, true);
            BoundaryNiagara->SetAsset(BoundaryNiagaraSystem);
            BoundaryNiagara->SetAutoActivate(false);
            BoundaryNiagara->SetHiddenInGame(false);
            BoundaryNiagara->SetVisibility(true, true);
            AddInstanceComponent(BoundaryNiagara);
            BoundaryNiagara->RegisterComponent();
            ConfigureNiagaraComponent(BoundaryNiagara, Index, D0, D1, true);
            BoundaryNiagara->Activate(true);
            BoundaryNiagaraComponents.Add(BoundaryNiagara);
        }
    }

    if (BoundaryRingNiagaraSystem)
    {
        TArray<float> RingDistances;

        if (bPlaceBoundaryRingsAtSplinePoints && Spline->GetNumberOfSplinePoints() > 0)
        {
            const int32 FirstPoint = bIncludeBoundaryRingAtStreamEnds ? 0 : 1;
            const int32 LastPointExclusive = bIncludeBoundaryRingAtStreamEnds
                ? Spline->GetNumberOfSplinePoints()
                : FMath::Max(Spline->GetNumberOfSplinePoints() - 1, 1);

            for (int32 PointIndex = FirstPoint; PointIndex < LastPointExclusive; ++PointIndex)
            {
                RingDistances.Add(Spline->GetDistanceAlongSplineAtSplinePoint(PointIndex));
            }
        }

        if (RingDistances.Num() == 0 && NumBoundaryRings > 0)
        {
            const int32 RingCount = FMath::Max(NumBoundaryRings, 1);
            for (int32 Index = 0; Index < RingCount; ++Index)
            {
                const float Alpha = RingCount == 1
                    ? 0.5f
                    : static_cast<float>(Index) / static_cast<float>(RingCount - 1);
                RingDistances.Add(TotalLength * Alpha);
            }
        }

        for (int32 Index = 0; Index < RingDistances.Num(); ++Index)
        {
            const float Distance = RingDistances[Index];

            UNiagaraComponent* RingNiagara = NewObject<UNiagaraComponent>(this, *FString::Printf(TEXT("WindBoundaryRingFX_%d"), Index));
            RingNiagara->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            RingNiagara->SetMobility(EComponentMobility::Movable);
            RingNiagara->SetupAttachment(RootComponent);
            RingNiagara->SetAbsolute(true, true, true);
            RingNiagara->SetAsset(BoundaryRingNiagaraSystem);
            RingNiagara->SetAutoActivate(false);
            RingNiagara->SetHiddenInGame(false);
            RingNiagara->SetVisibility(true, true);
            AddInstanceComponent(RingNiagara);
            RingNiagara->RegisterComponent();
            ConfigureBoundaryRingComponent(RingNiagara, Index, Distance);
            RingNiagara->Activate(true);
            BoundaryRingNiagaraComponents.Add(RingNiagara);
        }

        WIND_LOG(Log, TEXT("[WindStream] Rebuilt %d boundary rings | AutoSplinePoints=%s | IncludeEnds=%s"),
            BoundaryRingNiagaraComponents.Num(),
            bPlaceBoundaryRingsAtSplinePoints ? TEXT("true") : TEXT("false"),
            bIncludeBoundaryRingAtStreamEnds ? TEXT("true") : TEXT("false"));
    }
}

void AWindStreamZone::ConfigureNiagaraComponent(UNiagaraComponent* NiagaraComponent, int32 SegmentIndex,
    float DistanceStart, float DistanceEnd, bool bBoundary) const
{
    if (!NiagaraComponent || !Spline)
    {
        return;
    }

    const float MidDistance = (DistanceStart + DistanceEnd) * 0.5f;
    const FVector SegmentStart = Spline->GetLocationAtDistanceAlongSpline(DistanceStart, ESplineCoordinateSpace::World);
    const FVector SegmentEnd = Spline->GetLocationAtDistanceAlongSpline(DistanceEnd, ESplineCoordinateSpace::World);
    const FVector SegmentCenter = Spline->GetLocationAtDistanceAlongSpline(MidDistance, ESplineCoordinateSpace::World);
    FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(MidDistance, ESplineCoordinateSpace::World).GetSafeNormal();
    if (Tangent.IsNearlyZero())
    {
        Tangent = GetActorForwardVector();
    }
    const float SegmentLength = FMath::Max(FVector::Dist(SegmentStart, SegmentEnd), 1.f);
    const float Radius = bBoundary ? StreamRadius * BoundaryRadiusMultiplier : StreamRadius * StreamVisualWidthMultiplier;
    const float Diameter = FMath::Max(Radius * 2.f, 1.f);
    const FVector BoxSize(SegmentLength * StreamVisualLengthMultiplier, Diameter, Diameter);

    NiagaraComponent->SetWorldLocation(SegmentCenter);
    NiagaraComponent->SetWorldRotation(Tangent.IsNearlyZero() ? GetActorRotation() : Tangent.Rotation());

    NiagaraComponent->SetWorldScale3D(FVector(StreamNiagaraComponentScale));
    NiagaraComponent->SetVariablePosition(TEXT("User.SegmentStart"), SegmentStart);
    NiagaraComponent->SetVariablePosition(TEXT("User.SegmentEnd"), SegmentEnd);
    NiagaraComponent->SetVariableVec3(TEXT("User.StreamDirection"), Tangent);
    NiagaraComponent->SetVariableFloat(TEXT("User.StreamRadius"), Radius);
    NiagaraComponent->SetVariableFloat(TEXT("User.InnerRadius"), StreamRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.SegmentLength"), SegmentLength);
    NiagaraComponent->SetVariableFloat(TEXT("User.FlowSpeed"), StreamSpeed);
    NiagaraComponent->SetVariableFloat(TEXT("User.SegmentIndex"), static_cast<float>(SegmentIndex));
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryOpacity"), BoundaryOpacity);
    NiagaraComponent->SetVariableFloat(TEXT("User.StreamOpacity"), bBoundary ? BoundaryOpacity : 1.f);

    NiagaraComponent->SetVariableVec3(TEXT("User.Box Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Box_Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.BoxSize"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Wind_Box Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Wind_Box_Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.WindBoxSize"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Leaves_Box Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Leaves_Box_Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.LeavesBoxSize"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Boundary_Box Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Boundary_Box_Size"), BoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.BoundaryBoxSize"), BoxSize);

    NiagaraComponent->SetVariableFloat(TEXT("User.Wind_Spawn Rate"), bBoundary ? 0.f : StreamWindSpawnRate);
    NiagaraComponent->SetVariableFloat(TEXT("User.Wind_Spawn_Rate"), bBoundary ? 0.f : StreamWindSpawnRate);
    NiagaraComponent->SetVariableFloat(TEXT("User.WindSpawnRate"), bBoundary ? 0.f : StreamWindSpawnRate);
    NiagaraComponent->SetVariableFloat(TEXT("User.Leaves_Spawn Rate"), bBoundary ? 0.f : StreamLeavesSpawnRate);
    NiagaraComponent->SetVariableFloat(TEXT("User.Leaves_Spawn_Rate"), bBoundary ? 0.f : StreamLeavesSpawnRate);
    NiagaraComponent->SetVariableFloat(TEXT("User.LeavesSpawnRate"), bBoundary ? 0.f : StreamLeavesSpawnRate);
}

void AWindStreamZone::ConfigureBoundaryRingComponent(UNiagaraComponent* NiagaraComponent, int32 RingIndex, float Distance) const
{
    if (!NiagaraComponent || !Spline)
    {
        return;
    }

    const FVector Center = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
    if (Tangent.IsNearlyZero())
    {
        Tangent = GetActorForwardVector();
    }

    const float RingRadius = StreamRadius * BoundaryRingRadiusMultiplier;
    const float RingDiameter = FMath::Max(RingRadius * 2.f, 1.f);
    const FVector RingBoxSize(20.f, RingDiameter, RingDiameter);
    const float RingScale = BoundaryRingNiagaraScaleMultiplier * FMath::Max(StreamRadius / 300.f, 0.01f);

    const FRotator BaseRotation = Tangent.Rotation();
    NiagaraComponent->SetWorldLocation(Center);
    NiagaraComponent->SetWorldRotation(FRotator(
        BaseRotation.Pitch + BoundaryRingRotationOffset.Pitch,
        BaseRotation.Yaw + BoundaryRingRotationOffset.Yaw,
        BaseRotation.Roll + BoundaryRingRotationOffset.Roll));
    NiagaraComponent->SetWorldScale3D(FVector(StreamNiagaraComponentScale * RingScale));

    NiagaraComponent->SetVariableFloat(TEXT("User.RingRadius"), RingRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.RingDiameter"), RingDiameter);
    NiagaraComponent->SetVariableFloat(TEXT("User.StreamRadius"), StreamRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.InnerRadius"), StreamRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRadius"), RingRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingRadius"), RingRadius);
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingDiameter"), RingDiameter);
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryOpacity"), BoundaryOpacity);
    NiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingOpacity"), BoundaryOpacity);
    NiagaraComponent->SetVariableFloat(TEXT("User.RingIndex"), static_cast<float>(RingIndex));
    NiagaraComponent->SetVariableFloat(TEXT("User.RingDistance"), Distance);
    NiagaraComponent->SetVariableFloat(TEXT("User.RingScale"), StreamNiagaraComponentScale * RingScale);
    NiagaraComponent->SetVariableFloat(TEXT("User.RingRadiusMultiplier"), BoundaryRingRadiusMultiplier);
    NiagaraComponent->SetVariableVec3(TEXT("User.StreamDirection"), Tangent);
    NiagaraComponent->SetVariablePosition(TEXT("User.RingCenter"), Center);
    NiagaraComponent->SetVariablePosition(TEXT("User.BoundaryRingCenter"), Center);
    NiagaraComponent->SetVariableVec3(TEXT("User.Box Size"), RingBoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Box_Size"), RingBoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.BoxSize"), RingBoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Ring_Box Size"), RingBoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.Ring_Box_Size"), RingBoxSize);
    NiagaraComponent->SetVariableVec3(TEXT("User.RingBoxSize"), RingBoxSize);

    WIND_LOG(Log, TEXT("[WindStream] Ring[%d] Distance=%.1f Radius=%.1f Scale=%.2f"),
        RingIndex,
        Distance,
        RingRadius,
        StreamNiagaraComponentScale * RingScale);
}

void AWindStreamZone::DrawWindDebug() const
{
    if (!GetWorld() || !Spline)
    {
        return;
    }

    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule) continue;

        DrawDebugCapsule(
            GetWorld(),
            Capsule->GetComponentLocation(),
            Capsule->GetScaledCapsuleHalfHeight(),
            Capsule->GetScaledCapsuleRadius(),
            Capsule->GetComponentQuat(),
            FColor::Yellow,
            false,
            0.f,
            0,
            2.f);
    }

    const int32 SampleCount = FMath::Max(DebugSplineSamples, 4);
    if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    const float TotalLength = Spline->GetSplineLength();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    FVector PreviousCenter = FVector::ZeroVector;
    bool bHasPreviousCenter = false;

    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const float Alpha = SampleCount > 1 ? static_cast<float>(Index) / static_cast<float>(SampleCount - 1) : 0.f;
        const float Distance = TotalLength * Alpha;
        const FVector Center = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
        if (Tangent.IsNearlyZero())
        {
            Tangent = GetActorForwardVector();
        }
        const float CurveStrength = GetCurveStrength(Distance);
        const FColor SampleColor = CurveStrength > 0.65f
            ? FColor::Red
            : (CurveStrength > 0.25f ? FColor::Orange : FColor::Cyan);

        FVector UpAxis = FVector::UpVector;
        if (FMath::Abs(FVector::DotProduct(Tangent, UpAxis)) > 0.92f)
        {
            UpAxis = FVector::RightVector;
        }

        const FVector RightAxis = FVector::CrossProduct(UpAxis, Tangent).GetSafeNormal();
        const FVector VerticalAxis = FVector::CrossProduct(Tangent, RightAxis).GetSafeNormal();

        DrawDebugSphere(GetWorld(), Center, 22.f, 8, SampleColor, false, 0.f, 0, 1.5f);
        DrawDebugCircle(GetWorld(), Center, StreamRadius, 32, SampleColor, false, 0.f, 0, 1.5f, RightAxis, VerticalAxis, false);
        DrawDebugDirectionalArrow(GetWorld(), Center, Center + Tangent * 220.f, 45.f, FColor::Green, false, 0.f, 0, 2.f);

        if (bHasPreviousCenter)
        {
            DrawDebugLine(GetWorld(), PreviousCenter, Center, FColor::Cyan, false, 0.f, 0, 3.f);
        }

        PreviousCenter = Center;
        bHasPreviousCenter = true;
    }

    const FString DebugText = FString::Printf(
        TEXT("WindStream\nNiagara:%s FX:%d Boundary:%d Rings:%d Leaves:%.1f\nCaps:%d/%d Radius:%.0f Speed:%.0f Ramp:%.2fs"),
        StreamNiagaraSystem ? TEXT("OK") : TEXT("NONE"),
        StreamNiagaraComponents.Num(),
        BoundaryNiagaraComponents.Num(),
        BoundaryRingNiagaraComponents.Num(),
        StreamLeavesSpawnRate,
        CollisionCapsules.Num(),
        MaxGeneratedCollisionCapsules,
        StreamRadius,
        StreamSpeed,
        StreamSpeedRampTime);
    DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, StreamRadius + 120.f), DebugText, nullptr, FColor::White, 0.f, false);
}

void AWindStreamZone::RebuildCollisionCapsules()
{
    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule) continue;
        Capsule->OnComponentBeginOverlap.RemoveAll(this);
        Capsule->OnComponentEndOverlap.RemoveAll(this);
        Capsule->UnregisterComponent();
        RemoveInstanceComponent(Capsule);
        Capsule->DestroyComponent();
    }
    CollisionCapsules.Empty();
    PlayersInStream.Empty();
    PlayerWindUseTimes.Empty();
    PlayerSplineDistances.Empty();
    PlayerStreamDirectionSigns.Empty();

    if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    const float TotalLength = Spline->GetSplineLength();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const int32 AutoCapsuleCount = StreamRadius > KINDA_SMALL_NUMBER
        ? FMath::CeilToInt(TotalLength / FMath::Max(StreamRadius * 0.85f, 100.f)) + 1
        : NumCollisionCapsules;
    const int32 CapsuleCount = FMath::Clamp(
        FMath::Max(NumCollisionCapsules, AutoCapsuleCount),
        2,
        FMath::Max(MaxGeneratedCollisionCapsules, 2));
    const float Step = TotalLength / (CapsuleCount - 1);
    const float HalfHeight = Step * CollisionCapsuleOverlap;

    for (int32 Index = 0; Index < CapsuleCount; ++Index)
    {
        const float Distance = Index * Step;
        const FVector WorldPos = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
        if (Tangent.IsNearlyZero())
        {
            Tangent = GetActorForwardVector();
        }

        UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(this, *FString::Printf(TEXT("WindCapsule_%d"), Index));
        Capsule->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Capsule->SetMobility(EComponentMobility::Movable);
        Capsule->SetupAttachment(RootComponent);
        Capsule->SetAbsolute(true, true, true);
        AddInstanceComponent(Capsule);
        Capsule->RegisterComponent();
        Capsule->SetCapsuleSize(StreamRadius, FMath::Max(HalfHeight, StreamRadius));
        Capsule->SetCollisionProfileName(TEXT("Trigger"));
        Capsule->SetGenerateOverlapEvents(true);

        const FRotator CapsuleRot = Tangent.ToOrientationRotator();
        Capsule->SetWorldLocationAndRotation(
            WorldPos,
            FRotator(CapsuleRot.Pitch + 90.f, CapsuleRot.Yaw, CapsuleRot.Roll));

        Capsule->OnComponentBeginOverlap.AddDynamic(this, &AWindStreamZone::OnCapsuleBeginOverlap);
        Capsule->OnComponentEndOverlap.AddDynamic(this, &AWindStreamZone::OnCapsuleEndOverlap);
        Capsule->SetHiddenInGame(!bWindDebugMode);

        CollisionCapsules.Add(Capsule);
    }

    WIND_LOG(Log, TEXT("[WindStream] Rebuilt %d capsules | Radius %.0f | HalfHeight %.0f"),
        CapsuleCount, StreamRadius, HalfHeight);
}

void AWindStreamZone::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    const bool bAlreadyIn = PlayersInStream.Contains(Player);
    PlayersInStream.Add(Player);
    PlayerWindUseTimes.FindOrAdd(Player);

    if (!bAlreadyIn)
    {
        // Detect travel direction immediately on entry so early frames do not fight the player.
        const float InitialSplineDistance = GetClosestSplineDistance(Player->GetActorLocation());
        PlayerSplineDistances.FindOrAdd(Player, InitialSplineDistance);

        const FVector SplineTangent = Spline->GetTangentAtDistanceAlongSpline(
            InitialSplineDistance, ESplineCoordinateSpace::World).GetSafeNormal();
        const FVector PlayerVelocity = Player->GetVelocity();
        int32& DirectionSign = PlayerStreamDirectionSigns.FindOrAdd(Player, 0);

        if (!SplineTangent.IsNearlyZero() && PlayerVelocity.SizeSquared() > 100.f * 100.f)
        {
            const float VelocityAlongSpline = FVector::DotProduct(PlayerVelocity.GetSafeNormal(), SplineTangent);
            // Use a low threshold on entry so we commit immediately — avoids the
            // 2–3 frame window where DirectionSign is 0 and the stream pushes back.
            DirectionSign = VelocityAlongSpline >= 0.f ? 1 : -1;
        }
        else
        {
            // Default to forward if velocity is too low to determine direction.
            DirectionSign = 1;
        }

        WIND_LOG(Log, TEXT("[WindStream] ENTER %s via %s | Sign=%d | Dist=%.1f | Vel=(%.1f,%.1f,%.1f)"),
            *Player->GetName(),
            *OverlappedComp->GetName(),
            DirectionSign,
            InitialSplineDistance,
            PlayerVelocity.X,
            PlayerVelocity.Y,
            PlayerVelocity.Z);
    }
}

void AWindStreamZone::OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent*, int32)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    bool bStillInAnotherCapsule = false;
    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule || Capsule == OverlappedComp) continue;
        if (Capsule->IsOverlappingActor(Player))
        {
            bStillInAnotherCapsule = true;
            break;
        }
    }

    if (!bStillInAnotherCapsule)
    {
        PlayersInStream.Remove(Player);
        PlayerWindUseTimes.Remove(Player);
        PlayerSplineDistances.Remove(Player);
        PlayerStreamDirectionSigns.Remove(Player);
        WIND_LOG(Log, TEXT("[WindStream] EXIT %s"), *Player->GetName());
    }
}

float AWindStreamZone::GetClosestSplineDistance(const FVector& WorldPosition) const
{
    return Spline->GetDistanceAlongSplineAtLocation(WorldPosition, ESplineCoordinateSpace::World);
}

float AWindStreamZone::GetTrackedSplineDistance(APlayerCharacter* Player, const FVector& WorldPosition, float DeltaTime)
{
    if (!Player || !Spline)
    {
        return GetClosestSplineDistance(WorldPosition);
    }

    const float RawDistance = GetClosestSplineDistance(WorldPosition);
    const float TotalLength = Spline->GetSplineLength();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return RawDistance;
    }

    float& TrackedDistance = PlayerSplineDistances.FindOrAdd(Player, RawDistance);
    const float PreviousTrackedDistance = TrackedDistance;
    const float MaxDistanceStep = FMath::Max(Player->GetVelocity().Size() * DeltaTime * 1.8f, StreamRadius * 0.55f);

    const float SearchWindow = FMath::Max(MaxDistanceStep * 2.f, StreamRadius * 1.5f);
    const float SearchStart = FMath::Clamp(PreviousTrackedDistance - SearchWindow, 0.f, TotalLength);
    const float SearchEnd = FMath::Clamp(PreviousTrackedDistance + SearchWindow, 0.f, TotalLength);

    float BestDistance = PreviousTrackedDistance;
    float BestDistanceSq = FVector::DistSquared(
        WorldPosition,
        Spline->GetLocationAtDistanceAlongSpline(PreviousTrackedDistance, ESplineCoordinateSpace::World));

    if (SearchEnd > SearchStart + KINDA_SMALL_NUMBER)
    {
        const int32 SampleCount = FMath::Clamp(
            FMath::CeilToInt((SearchEnd - SearchStart) / FMath::Max(StreamRadius * 0.35f, 120.f)) + 1,
            5,
            17);

        for (int32 Index = 0; Index < SampleCount; ++Index)
        {
            const float Alpha = SampleCount > 1
                ? static_cast<float>(Index) / static_cast<float>(SampleCount - 1)
                : 0.f;
            const float CandidateDistance = FMath::Lerp(SearchStart, SearchEnd, Alpha);
            const FVector CandidatePoint = Spline->GetLocationAtDistanceAlongSpline(CandidateDistance, ESplineCoordinateSpace::World);
            const float CandidateDistanceSq = FVector::DistSquared(WorldPosition, CandidatePoint);

            if (CandidateDistanceSq < BestDistanceSq)
            {
                BestDistanceSq = CandidateDistanceSq;
                BestDistance = CandidateDistance;
            }
        }
    }

    const bool bRawDistanceIsLocal = RawDistance >= SearchStart - KINDA_SMALL_NUMBER
        && RawDistance <= SearchEnd + KINDA_SMALL_NUMBER;
    if (bRawDistanceIsLocal)
    {
        const float RawDistanceSq = FVector::DistSquared(
            WorldPosition,
            Spline->GetLocationAtDistanceAlongSpline(RawDistance, ESplineCoordinateSpace::World));
        if (RawDistanceSq < BestDistanceSq)
        {
            BestDistanceSq = RawDistanceSq;
            BestDistance = RawDistance;
        }
    }

    const float DeltaToBestDistance = BestDistance - PreviousTrackedDistance;
    TrackedDistance = FMath::Clamp(
        PreviousTrackedDistance + FMath::Clamp(DeltaToBestDistance, -MaxDistanceStep, MaxDistanceStep),
        0.f,
        TotalLength);

    // Travel direction is locked for the whole stream pass.
    // This only initializes it if the player started diving while already inside.
    const FVector TrackedTangent = Spline->GetTangentAtDistanceAlongSpline(TrackedDistance, ESplineCoordinateSpace::World).GetSafeNormal();
    if (!TrackedTangent.IsNearlyZero())
    {
        int32& DirectionSign = PlayerStreamDirectionSigns.FindOrAdd(Player, 0);
        const float TrackedDelta = TrackedDistance - PreviousTrackedDistance;

        if (DirectionSign == 0)
        {
            // Resolve the direction quickly only while it has not been committed yet.
            if (FMath::Abs(TrackedDelta) > 0.5f)
            {
                DirectionSign = TrackedDelta >= 0.f ? 1 : -1;
            }
            else
            {
                const float VelocityAlongSpline = FVector::DotProduct(Player->GetVelocity(), TrackedTangent);
                if (FMath::Abs(VelocityAlongSpline) > 50.f)
                {
                    DirectionSign = VelocityAlongSpline >= 0.f ? 1 : -1;
                }
            }
        }
    }

    WIND_LOG(Log, TEXT("[WindStream] TRACK %s | Raw=%.1f Best=%.1f PrevTracked=%.1f Tracked=%.1f Delta=%.1f Window=%.1f"),
        Player ? *Player->GetName() : TEXT("None"),
        RawDistance,
        BestDistance,
        PreviousTrackedDistance,
        TrackedDistance,
        DeltaToBestDistance,
        SearchWindow);

    return TrackedDistance;
}

float AWindStreamZone::GetRadialFalloff(const FVector& WorldPosition, float SplineDistance) const
{
    const FVector ClosestPoint = Spline->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
    const float DistanceToCore = FVector::Dist(WorldPosition, ClosestPoint);

    if (DistanceToCore >= StreamRadius)
    {
        return 0.f;
    }

    return FMath::Clamp(FMath::Cos((DistanceToCore / StreamRadius) * PI * 0.5f), 0.f, 1.f);
}

float AWindStreamZone::GetCurveStrength(float SplineDistance) const
{
    if (!Spline)
    {
        return 0.f;
    }

    const float TotalLength = Spline->GetSplineLength();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return 0.f;
    }

    const float SampleDistance = FMath::Clamp(CurveLookAheadDistance, 100.f, TotalLength * 0.5f);
    const float PrevDistance = FMath::Clamp(SplineDistance - SampleDistance, 0.f, TotalLength);
    const float NextDistance = FMath::Clamp(SplineDistance + SampleDistance, 0.f, TotalLength);
    const FVector PrevTangent = Spline->GetTangentAtDistanceAlongSpline(PrevDistance, ESplineCoordinateSpace::World).GetSafeNormal();
    const FVector NextTangent = Spline->GetTangentAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::World).GetSafeNormal();

    if (PrevTangent.IsNearlyZero() || NextTangent.IsNearlyZero())
    {
        return 0.f;
    }

    const float Dot = FMath::Clamp(FVector::DotProduct(PrevTangent, NextTangent), -1.f, 1.f);
    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
    return FMath::GetMappedRangeValueClamped(FVector2D(8.f, 70.f), FVector2D(0.f, 1.f), AngleDegrees);
}

bool AWindStreamZone::IsPlayerInDiveMode(APlayerCharacter* Player) const
{
    if (!Player)
    {
        return false;
    }

    UFlightComponent* FlightComponent = Player->FindComponentByClass<UFlightComponent>();
    return FlightComponent && FlightComponent->IsInMode(EFlightMode::Dive);
}

UDiveMode* AWindStreamZone::GetPlayerDiveMode(APlayerCharacter* Player) const
{
    if (!Player)
    {
        return nullptr;
    }

    UFlightComponent* FlightComponent = Player->FindComponentByClass<UFlightComponent>();
    if (!FlightComponent || !FlightComponent->IsInMode(EFlightMode::Dive))
    {
        return nullptr;
    }

    return FlightComponent->GetDiveMode();
}

FVector AWindStreamZone::GetPreferredStreamDirection(APlayerCharacter* Player, float SplineDistance)
{
    const FVector SplineDirection = Spline->GetTangentAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World).GetSafeNormal();
    if (!Player || SplineDirection.IsNearlyZero())
    {
        return SplineDirection;
    }

    // Usually initialized on entry; this fallback handles teleports or diving inside a stream.
    int32 DirectionSign = PlayerStreamDirectionSigns.FindRef(Player);
    if (DirectionSign == 0)
    {
        const FVector PlayerVelocity = Player->GetVelocity();
        if (PlayerVelocity.SizeSquared() > 50.f * 50.f)
        {
            const float VelocityAlongSpline = FVector::DotProduct(PlayerVelocity.GetSafeNormal(), SplineDirection);
            DirectionSign = VelocityAlongSpline >= 0.f ? 1 : -1;
            // Commit it so subsequent calls are consistent.
            PlayerStreamDirectionSigns.FindOrAdd(Player) = DirectionSign;
        }
        else
        {
            // Cannot determine direction yet — don't apply stream influence this frame.
            return FVector::ZeroVector;
        }
    }

    return DirectionSign < 0 ? -SplineDirection : SplineDirection;
}

void AWindStreamZone::ApplyWindEffect(APlayerCharacter* Player, float DeltaTime)
{
    UDiveMode* DiveMode = GetPlayerDiveMode(Player);
    if (!DiveMode)
    {
        return;
    }

    const FVector PlayerPos = Player->GetActorLocation();
    const float RawSplineDistance = GetClosestSplineDistance(PlayerPos);
    const float SplineDistance = GetTrackedSplineDistance(Player, PlayerPos, DeltaTime);
    const FVector ClosestPoint = Spline->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
    const FVector StreamDirection = GetPreferredStreamDirection(Player, SplineDistance);
    const float CurveStrength = GetCurveStrength(SplineDistance);
    const float Falloff = GetRadialFalloff(PlayerPos, SplineDistance);

    // FIX: StreamDirection can now be ZeroVector when the direction is not yet
    // committed — treat this the same as leaving the stream to avoid any push.
    if (Falloff <= 0.f || StreamDirection.IsNearlyZero())
    {
        PlayerWindUseTimes.FindOrAdd(Player) = 0.f;
        WIND_LOG(Log, TEXT("[WindStream] SKIP %s | Falloff=%.2f | DirZero=%s | Raw=%.1f Tracked=%.1f"),
            *Player->GetName(),
            Falloff,
            StreamDirection.IsNearlyZero() ? TEXT("true") : TEXT("false"),
            RawSplineDistance,
            SplineDistance);
        return;
    }

    FVector CenterOffset = ClosestPoint - PlayerPos;
    CenterOffset = FVector::VectorPlaneProject(CenterOffset, StreamDirection);
    const float DistanceToCore = CenterOffset.Size();
    const float RadialRatio = StreamRadius > KINDA_SMALL_NUMBER
        ? FMath::Clamp(DistanceToCore / StreamRadius, 0.f, 1.f)
        : 0.f;
    const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
        FVector2D(FreeMovementRadiusRatio, 1.f),
        FVector2D(0.f, 1.f),
        RadialRatio);

    const float SteeringInput = FMath::Clamp(
        FMath::Abs(Player->GetHorizontalInput()) + (FMath::Abs(Player->GetVerticalInput()) * 0.35f),
        0.f,
        1.f);
    const bool bIdleInStream = SteeringInput < 0.1f;

    FVector ReferenceDirection = Player->GetVelocity().GetSafeNormal();
    if (ReferenceDirection.IsNearlyZero())
    {
        ReferenceDirection = Player->GetActorForwardVector().GetSafeNormal();
    }

    // Use the signed stream direction so reverse travel receives normal alignment.
    const float StreamAlignment = FMath::Abs(FVector::DotProduct(ReferenceDirection, StreamDirection));
    const bool bFullyUsingStream = StreamAlignment >= 0.55f;
    const float AlignmentAssist = bFullyUsingStream
        ? FMath::GetMappedRangeValueClamped(
            FVector2D(0.55f, 0.9f),
            FVector2D(0.35f, 1.f),
            StreamAlignment)
        : CrossingAssistStrength;

    float& StreamUseTime = PlayerWindUseTimes.FindOrAdd(Player);
    if (bFullyUsingStream)
    {
        StreamUseTime += DeltaTime * FMath::Lerp(0.7f, 1.25f, AlignmentAssist) * FMath::Clamp(Falloff, 0.25f, 1.f);
    }
    else
    {
        StreamUseTime = FMath::Max(0.f, StreamUseTime - DeltaTime * 2.f);
    }

    const float RampAlpha = StreamSpeedRampTime > KINDA_SMALL_NUMBER
        ? FMath::Clamp(StreamUseTime / StreamSpeedRampTime, 0.f, 1.f)
        : 1.f;
    const float SmoothedRampAlpha = FMath::InterpEaseInOut(0.f, 1.f, RampAlpha, 2.f);
    const float CurveGripScale = 1.f + CurveStrength * CurveGripBoost;
    const float CurveSpeedScale = 1.f - CurveStrength * CurveSpeedReduction;
    const float StreamSpeedAtEntry = StreamSpeed * StreamEntrySpeedRatio;
    const float RampedStreamSpeed = FMath::Lerp(StreamSpeedAtEntry, StreamSpeed * CurveSpeedScale, SmoothedRampAlpha);

    const float EdgeCenteringStrength = CenteringStrength
        * EdgeAlpha
        * EdgeAlpha
        * Falloff
        * FMath::Lerp(0.09f, 0.025f, SteeringInput)
        * CurveGripScale;

    const float IdleCenteringStrength = CenteringStrength
        * RadialRatio
        * RadialRatio
        * Falloff
        * (bIdleInStream ? 0.75f : 0.f)
        * CurveGripScale;

    const float AlignmentCenteringScale = bFullyUsingStream ? FMath::Square(AlignmentAssist) : 0.f;
    const float SoftCenteringStrength = (EdgeCenteringStrength + IdleCenteringStrength) * AlignmentCenteringScale;
    const FVector CenteringAccel = DistanceToCore > KINDA_SMALL_NUMBER
        ? CenterOffset * SoftCenteringStrength
        : FVector::ZeroVector;

    const float EffectiveInfluence = bFullyUsingStream
        ? FMath::Clamp(DirectionInfluence * CurveGripScale, 0.f, 1.f) * Falloff * FMath::Square(AlignmentAssist)
        : 0.f;
    const float CurrentDiveSpeed = DiveMode->GetCurrentSpeed();
    const float TargetSpeed = bFullyUsingStream
        ? FMath::Lerp(CurrentDiveSpeed, FMath::Max(CurrentDiveSpeed, RampedStreamSpeed), AlignmentAssist)
        : FMath::Lerp(CurrentDiveSpeed, FMath::Max(CurrentDiveSpeed, StreamSpeedAtEntry), CrossingAssistStrength);

    DiveMode->ApplyWindBoost(TargetSpeed, StreamDirection, EffectiveInfluence, CenteringAccel);

    WIND_SCREEN(21, FColor::Cyan,
        TEXT("[WindStream] %s | Sign:%d | Assist %.2f | Ramp %.2f | Curve %.2f | Center %.0f"),
        *Player->GetName(),
        PlayerStreamDirectionSigns.FindRef(Player),
        AlignmentAssist,
        SmoothedRampAlpha,
        CurveStrength,
        CenterOffset.Size());

    WIND_LOG(Log, TEXT("[WindStream] APPLY %s | Raw=%.1f Tracked=%.1f Sign=%d Falloff=%.2f Align=%.2f Center=%.1f Speed=%.1f Target=%.1f Input=(%.2f,%.2f) Pos=(%.1f,%.1f,%.1f) Vel=(%.1f,%.1f,%.1f)"),
        *Player->GetName(),
        RawSplineDistance,
        SplineDistance,
        PlayerStreamDirectionSigns.FindRef(Player),
        Falloff,
        StreamAlignment,
        DistanceToCore,
        CurrentDiveSpeed,
        TargetSpeed,
        Player->GetHorizontalInput(),
        Player->GetVerticalInput(),
        PlayerPos.X,
        PlayerPos.Y,
        PlayerPos.Z,
        Player->GetVelocity().X,
        Player->GetVelocity().Y,
        Player->GetVelocity().Z);
}
