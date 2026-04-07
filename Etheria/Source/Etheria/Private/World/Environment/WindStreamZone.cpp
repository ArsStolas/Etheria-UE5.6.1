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
#include "Components/SplineMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

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
        RebuildVisualTube();
        RebuildCollisionCapsules();
    }
}

void AWindStreamZone::BeginPlay()
{
    Super::BeginPlay();

    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule) continue;
        Capsule->SetHiddenInGame(!bWindDebugMode);
    }
}

void AWindStreamZone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (UVScrollSpeed > 0.f)
    {
        UVOffset = FMath::Fmod(UVOffset + UVScrollSpeed * DeltaTime, 1.f);
        for (int32 Index = 0; Index < SplineMeshes.Num(); ++Index)
        {
            if (!SplineMeshes[Index]) continue;

            if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(SplineMeshes[Index]->GetMaterial(0)))
            {
                Material->SetScalarParameterValue(TEXT("UVOffset"), UVOffset + Index * 0.05f);
            }
        }
    }

    WIND_SCREEN(20, PlayersInStream.Num() > 0 ? FColor::Green : FColor::Orange,
        TEXT("[WindStream] Players: %d | Capsules: %d | Speed: %.0f"),
        PlayersInStream.Num(), CollisionCapsules.Num(), StreamSpeed);

    if (bWindDebugMode)
    {
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
            continue;
        }

        const float SplineDistance = GetClosestSplineDistance(Player->GetActorLocation());
        const float Falloff = GetRadialFalloff(Player->GetActorLocation(), SplineDistance);
        if (Falloff <= 0.f)
        {
            continue;
        }

        ApplyWindEffect(Player, DeltaTime, Falloff);

        if (bWindDebugMode)
        {
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
    }
}

void AWindStreamZone::RebuildVisualTube()
{
    for (USplineMeshComponent* MeshComponent : SplineMeshes)
    {
        if (!MeshComponent) continue;
        MeshComponent->UnregisterComponent();
        MeshComponent->DestroyComponent();
    }
    SplineMeshes.Empty();

    if (!StreamMesh || Spline->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    const float TotalLength = Spline->GetSplineLength();
    const float SegmentLength = TotalLength / FMath::Max(NumVisualSegments, 1);

    for (int32 Index = 0; Index < NumVisualSegments; ++Index)
    {
        const float D0 = Index * SegmentLength;
        const float D1 = (Index + 1) * SegmentLength;

        const FVector P0 = Spline->GetLocationAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World);
        const FVector P1 = Spline->GetLocationAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World);
        const FVector T0 = Spline->GetTangentAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World).GetSafeNormal() * SegmentLength;
        const FVector T1 = Spline->GetTangentAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World).GetSafeNormal() * SegmentLength;

        USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this, *FString::Printf(TEXT("WindSeg_%d"), Index));
        SplineMesh->SetMobility(EComponentMobility::Movable);
        SplineMesh->SetupAttachment(RootComponent);
        SplineMesh->SetAbsolute(true, true, true);
        SplineMesh->RegisterComponent();

        SplineMesh->SetStaticMesh(StreamMesh);
        SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SplineMesh->SetCastShadow(false);
        SplineMesh->SetForwardAxis(ESplineMeshAxis::Y);
        SplineMesh->SetSplineUpDir(FVector::UpVector, false);
        SplineMesh->SetStartAndEnd(P0, T0, P1, T1, true);

        const float Scale = StreamRadius / 50.f;
        SplineMesh->SetStartScale(FVector2D(Scale, Scale));
        SplineMesh->SetEndScale(FVector2D(Scale, Scale));

        if (StreamMaterial)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StreamMaterial, this);
            Material->SetScalarParameterValue(TEXT("Opacity"), TubeOpacity);
            Material->SetScalarParameterValue(TEXT("UVOffset"), 0.f);
            Material->SetScalarParameterValue(TEXT("SegmentIndex"), static_cast<float>(Index));
            SplineMesh->SetMaterial(0, Material);
        }

        SplineMeshes.Add(SplineMesh);
    }
}

void AWindStreamZone::RebuildCollisionCapsules()
{
    for (UCapsuleComponent* Capsule : CollisionCapsules)
    {
        if (!Capsule) continue;
        Capsule->OnComponentBeginOverlap.RemoveAll(this);
        Capsule->OnComponentEndOverlap.RemoveAll(this);
        Capsule->UnregisterComponent();
        Capsule->DestroyComponent();
    }
    CollisionCapsules.Empty();
    PlayersInStream.Empty();

    const int32 CapsuleCount = FMath::Max(NumCollisionCapsules, 2);
    const float TotalLength = Spline->GetSplineLength();
    const float Step = TotalLength / (CapsuleCount - 1);
    const float HalfHeight = Step * 0.55f;

    for (int32 Index = 0; Index < CapsuleCount; ++Index)
    {
        const float Distance = Index * Step;
        const FVector WorldPos = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        const FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();

        UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(this, *FString::Printf(TEXT("WindCapsule_%d"), Index));
        Capsule->SetMobility(EComponentMobility::Movable);
        Capsule->SetupAttachment(RootComponent);
        Capsule->SetAbsolute(true, true, true);
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

    if (!bAlreadyIn)
    {
        WIND_LOG(Log, TEXT("[WindStream] ENTER %s via %s"), *Player->GetName(), *OverlappedComp->GetName());
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
        WIND_LOG(Log, TEXT("[WindStream] EXIT %s"), *Player->GetName());
    }
}

float AWindStreamZone::GetClosestSplineDistance(const FVector& WorldPosition) const
{
    return Spline->GetDistanceAlongSplineAtLocation(WorldPosition, ESplineCoordinateSpace::World);
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

FVector AWindStreamZone::GetPreferredStreamDirection(APlayerCharacter* Player, float SplineDistance) const
{
    const FVector SplineDirection = Spline->GetTangentAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World).GetSafeNormal();
    if (!Player || SplineDirection.IsNearlyZero())
    {
        return SplineDirection;
    }

    FVector ReferenceDirection = Player->GetVelocity().GetSafeNormal();
    if (ReferenceDirection.IsNearlyZero())
    {
        ReferenceDirection = Player->GetActorForwardVector().GetSafeNormal();
    }

    return (FVector::DotProduct(ReferenceDirection, SplineDirection) >= 0.f)
        ? SplineDirection
        : -SplineDirection;
}

void AWindStreamZone::ApplyWindEffect(APlayerCharacter* Player, float /*DeltaTime*/, float Falloff)
{
    UDiveMode* DiveMode = GetPlayerDiveMode(Player);
    if (!DiveMode)
    {
        return;
    }

    const FVector PlayerPos = Player->GetActorLocation();
    const float SplineDistance = GetClosestSplineDistance(PlayerPos);
    const FVector ClosestPoint = Spline->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
    const FVector StreamDirection = GetPreferredStreamDirection(Player, SplineDistance);

    if (StreamDirection.IsNearlyZero())
    {
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

    const float StreamAlignment = FMath::Abs(FVector::DotProduct(ReferenceDirection, StreamDirection));
    const float AlignmentAssist = FMath::GetMappedRangeValueClamped(
        FVector2D(0.15f, 0.8f),
        FVector2D(CrossingAssistStrength, 1.f),
        StreamAlignment);

    const float EdgeCenteringStrength = CenteringStrength
        * EdgeAlpha
        * EdgeAlpha
        * Falloff
        * FMath::Lerp(0.09f, 0.025f, SteeringInput);

    const float IdleCenteringStrength = CenteringStrength
        * RadialRatio
        * RadialRatio
        * Falloff
        * (bIdleInStream ? 0.75f : 0.f);

    const float SoftCenteringStrength = EdgeCenteringStrength + IdleCenteringStrength;
    const FVector CenteringAccel = DistanceToCore > KINDA_SMALL_NUMBER
        ? CenterOffset * SoftCenteringStrength
        : FVector::ZeroVector;

    const float EffectiveInfluence = DirectionInfluence * Falloff * AlignmentAssist;
    const float CurrentDiveSpeed = DiveMode->GetCurrentSpeed();
    const float TargetSpeed = FMath::Lerp(CurrentDiveSpeed, FMath::Max(CurrentDiveSpeed, StreamSpeed), AlignmentAssist);

    DiveMode->ApplyWindBoost(TargetSpeed, StreamDirection, EffectiveInfluence, CenteringAccel);

    WIND_SCREEN(21, FColor::Cyan,
        TEXT("[WindStream] %s | Assist %.2f | Falloff %.2f | Center %.0f"),
        *Player->GetName(),
        AlignmentAssist,
        Falloff,
        CenterOffset.Size());
}

