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
        UVOffset = FMath::Fmod(UVOffset + UVScrollSpeed * DeltaTime, 1000.f);

        for (int32 Index = 0; Index < StreamNiagaraComponents.Num(); ++Index)
        {
            if (!StreamNiagaraComponents[Index]) continue;
            StreamNiagaraComponents[Index]->SetVariableFloat(TEXT("User.FlowOffset"), UVOffset + Index * 0.05f);
            StreamNiagaraComponents[Index]->SetVariableFloat(TEXT("User.FlowSpeed"), UVScrollSpeed);
        }

        for (int32 Index = 0; Index < BoundaryNiagaraComponents.Num(); ++Index)
        {
            if (!BoundaryNiagaraComponents[Index]) continue;
            BoundaryNiagaraComponents[Index]->SetVariableFloat(TEXT("User.FlowOffset"), UVOffset + Index * 0.05f);
            BoundaryNiagaraComponents[Index]->SetVariableFloat(TEXT("User.FlowSpeed"), UVScrollSpeed);
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

void AWindStreamZone::DestroyVisualComponents()
{
    for (UNiagaraComponent* NiagaraComponent : StreamNiagaraComponents)
    {
        if (!NiagaraComponent) continue;
        NiagaraComponent->UnregisterComponent();
        NiagaraComponent->DestroyComponent();
    }
    StreamNiagaraComponents.Empty();

    for (UNiagaraComponent* NiagaraComponent : BoundaryNiagaraComponents)
    {
        if (!NiagaraComponent) continue;
        NiagaraComponent->UnregisterComponent();
        NiagaraComponent->DestroyComponent();
    }
    BoundaryNiagaraComponents.Empty();
}

void AWindStreamZone::ConfigureNiagaraComponent(
    UNiagaraComponent* NiagaraComponent,
    const FVector& StartPoint,
    const FVector& EndPoint,
    const FVector& Tangent,
    float Radius,
    float Opacity,
    float SegmentIndex) const
{
    if (!NiagaraComponent) return;

    const FVector SegmentVector = EndPoint - StartPoint;
    const float SegmentLength = SegmentVector.Size();
    const FVector MidPoint = (StartPoint + EndPoint) * 0.5f;
    const FRotator SegmentRotation = Tangent.Rotation();
    
    // ── POSITIONNEMENT DU NIAGARA ────────────────────────────────────────
    // Le component est placé au centre du segment, orienté selon la tangente
    NiagaraComponent->SetWorldLocationAndRotation(MidPoint, SegmentRotation);
    NiagaraComponent->SetWorldScale3D(FVector(1.f, 1.f, 1.f)); // Reset scale
    
    // ── OVERRIDE DES BOX SIZES (pour Shape Location) ────────────────────
    // Box_Size contrôle le spawn des particules Wind_Curved/Wind_Straight
    // On veut spawner dans un cylindre de longueur=SegmentLength, rayon=Radius
    // Dans le référentiel local du component (orienté selon Tangent) :
    // X = forward (longueur), Y/Z = radial (rayon)
    const FVector BoxSize = FVector(
        SegmentLength,      // X = longueur du segment
        Radius * 2.f,       // Y = diamètre
        Radius * 2.f        // Z = diamètre
    );
    NiagaraComponent->SetVariableVec3(TEXT("User.Box_Size"), BoxSize);
    
    // Leaves_Box_Size (pour l'emitter Falling_Leaf si tu le gardes actif)
    const FVector LeavesBoxSize = FVector(
        SegmentLength * 0.8f,  // Un peu plus court
        Radius * 1.8f,
        Radius * 1.8f
    );
    NiagaraComponent->SetVariableVec3(TEXT("User.Leaves_Box_Size"), LeavesBoxSize);
    
    // ── PARAMÈTRES DE COULEUR/SPAWN RATE (optionnels) ───────────────────
    // Ajuste le spawn rate en fonction de la taille du stream
    const float DensityFactor = (Radius / 300.f) * (SegmentLength / 1000.f);
    NiagaraComponent->SetVariableFloat(TEXT("User.Wind_Spawn_Rate"), 
        FMath::Clamp(50.f * DensityFactor, 10.f, 200.f));
    
    // Si tu veux varier les couleurs des feuilles
    NiagaraComponent->SetVariableLinearColor(TEXT("User.Leaves_ColorMin"), 
        FLinearColor(0.8f, 0.6f, 0.2f)); // Orange clair
    NiagaraComponent->SetVariableLinearColor(TEXT("User.Leaves_ColorMax"), 
        FLinearColor(0.9f, 0.8f, 0.4f)); // Jaune pâle
    
    // ── PARAMÈTRES EXISTANTS (conservés) ─────────────────────────────────
    NiagaraComponent->SetVariableVec3(TEXT("User.SegmentStart"), StartPoint);
    NiagaraComponent->SetVariableVec3(TEXT("User.SegmentEnd"), EndPoint);
    NiagaraComponent->SetVariableVec3(TEXT("User.StreamDirection"), Tangent);
    NiagaraComponent->SetVariableFloat(TEXT("User.StreamRadius"), Radius);
    NiagaraComponent->SetVariableFloat(TEXT("User.SegmentLength"), SegmentLength);
    NiagaraComponent->SetVariableFloat(TEXT("User.StreamOpacity"), Opacity);
    NiagaraComponent->SetVariableFloat(TEXT("User.FlowSpeed"), UVScrollSpeed);
    NiagaraComponent->SetVariableFloat(TEXT("User.FlowOffset"), UVOffset + SegmentIndex * 0.05f);
    NiagaraComponent->SetVariableFloat(TEXT("User.SegmentIndex"), SegmentIndex);
    
    // Visuals multipliers (si tu les utilises dans le Niagara)
    const float VisualWidth = FMath::Max((Radius / 100.f) * StreamVisualWidthMultiplier, 0.1f);
    const float VisualLength = FMath::Max((SegmentLength / 100.f) * StreamVisualLengthScale, 0.1f);
    NiagaraComponent->SetVariableFloat(TEXT("User.VisualWidth"), Radius * StreamVisualWidthMultiplier);
    NiagaraComponent->SetVariableFloat(TEXT("User.VisualLength"), SegmentLength * StreamVisualLengthScale);
}

void AWindStreamZone::RebuildVisualTube()
{
    DestroyVisualComponents();

    if (Spline->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    const float TotalLength = Spline->GetSplineLength();
    const int32 SegmentCount = FMath::Max(NumVisualSegments, 1);
    const float SegmentLength = TotalLength / SegmentCount;

    for (int32 Index = 0; Index < SegmentCount; ++Index)
    {
        const float D0 = Index * SegmentLength;
        const float D1 = (Index + 1) * SegmentLength;

        const FVector StartPoint = Spline->GetLocationAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World);
        const FVector EndPoint = Spline->GetLocationAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World);
        const FVector Tangent = (EndPoint - StartPoint).GetSafeNormal();
        if (Tangent.IsNearlyZero())
        {
            continue;
        }

        if (StreamNiagaraSystem)
        {
            UNiagaraComponent* StreamComponent = NewObject<UNiagaraComponent>(this, *FString::Printf(TEXT("WindFx_%d"), Index));
            StreamComponent->SetAsset(StreamNiagaraSystem);
            StreamComponent->SetMobility(EComponentMobility::Movable);
            StreamComponent->SetupAttachment(RootComponent);
            StreamComponent->SetAbsolute(true, true, false);
            StreamComponent->RegisterComponent();
            ConfigureNiagaraComponent(StreamComponent, StartPoint, EndPoint, Tangent, StreamRadius, TubeOpacity, static_cast<float>(Index));
            StreamNiagaraComponents.Add(StreamComponent);
        }

        if (BoundaryNiagaraSystem)
        {
            UNiagaraComponent* BoundaryComponent = NewObject<UNiagaraComponent>(this, *FString::Printf(TEXT("WindBoundaryFx_%d"), Index));
            BoundaryComponent->SetAsset(BoundaryNiagaraSystem);
            BoundaryComponent->SetMobility(EComponentMobility::Movable);
            BoundaryComponent->SetupAttachment(RootComponent);
            BoundaryComponent->SetAbsolute(true, true, false);
            BoundaryComponent->RegisterComponent();
            ConfigureNiagaraComponent(
                BoundaryComponent,
                StartPoint,
                EndPoint,
                Tangent,
                StreamRadius * BoundaryRadiusMultiplier,
                BoundaryOpacity,
                static_cast<float>(Index));
            BoundaryComponent->SetVariableFloat(TEXT("User.BoundaryRadius"), StreamRadius * BoundaryRadiusMultiplier);
            BoundaryComponent->SetVariableFloat(TEXT("User.InnerRadius"), StreamRadius);
            BoundaryComponent->SetVariableFloat(TEXT("User.BoundaryOpacity"), BoundaryOpacity);
            BoundaryNiagaraComponents.Add(BoundaryComponent);
        }
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
    const bool bFullyUsingStream = StreamAlignment >= 0.55f;
    const float AlignmentAssist = bFullyUsingStream
        ? FMath::GetMappedRangeValueClamped(
            FVector2D(0.55f, 0.9f),
            FVector2D(0.35f, 1.f),
            StreamAlignment)
        : CrossingAssistStrength;

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

    const float AlignmentCenteringScale = bFullyUsingStream ? FMath::Square(AlignmentAssist) : 0.f;
    const float SoftCenteringStrength = (EdgeCenteringStrength + IdleCenteringStrength) * AlignmentCenteringScale;
    const FVector CenteringAccel = DistanceToCore > KINDA_SMALL_NUMBER
        ? CenterOffset * SoftCenteringStrength
        : FVector::ZeroVector;

    const float EffectiveInfluence = bFullyUsingStream
        ? DirectionInfluence * Falloff * FMath::Square(AlignmentAssist)
        : 0.f;
    const float CurrentDiveSpeed = DiveMode->GetCurrentSpeed();
    const float TargetSpeed = bFullyUsingStream
        ? FMath::Lerp(CurrentDiveSpeed, FMath::Max(CurrentDiveSpeed, StreamSpeed), AlignmentAssist)
        : FMath::Lerp(CurrentDiveSpeed, FMath::Max(CurrentDiveSpeed, StreamSpeed * 0.35f), CrossingAssistStrength);

    DiveMode->ApplyWindBoost(TargetSpeed, StreamDirection, EffectiveInfluence, CenteringAccel);

    WIND_SCREEN(21, FColor::Cyan,
        TEXT("[WindStream] %s | Assist %.2f | Falloff %.2f | Center %.0f"),
        *Player->GetName(),
        AlignmentAssist,
        Falloff,
        CenterOffset.Size());
}
