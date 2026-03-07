/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: AWindStreamZone - Source
 */

#include "World/Environment/WindStreamZone.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"

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

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────

void AWindStreamZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Guard : ne pas reconstruire si l'actor est en cours de destruction
    // (peut arriver quand on déplace la spline rapidement dans l'éditeur)
    if (IsValid(this) && !IsUnreachable())
    {
        RebuildVisualTube();
        RebuildCollisionCapsules();
    }
}

void AWindStreamZone::RebuildVisualTube()
{
    // Unregister d'abord, puis destroy — évite le crash 'still externally referenced'
    // qui arrive quand on détruit un component encore tracké par le TypedElementRegistry
    for (USplineMeshComponent* M : SplineMeshes)
    {
        if (!M) continue;
        M->UnregisterComponent();
        M->DestroyComponent();
    }
    SplineMeshes.Empty();

    if (!StreamMesh || Spline->GetNumberOfSplinePoints() < 2) return;

    const float TotalLength = Spline->GetSplineLength();
    const float SegLen      = TotalLength / FMath::Max(NumVisualSegments, 1);

    for (int32 i = 0; i < NumVisualSegments; i++)
    {
        const float D0 = i       * SegLen;
        const float D1 = (i + 1) * SegLen;

        const FVector P0 = Spline->GetLocationAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World);
        const FVector P1 = Spline->GetLocationAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World);
        const FVector T0 = Spline->GetTangentAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World).GetSafeNormal() * SegLen;
        const FVector T1 = Spline->GetTangentAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World).GetSafeNormal() * SegLen;

        USplineMeshComponent* SMC = NewObject<USplineMeshComponent>(this,
            *FString::Printf(TEXT("WindSeg_%d"), i));
        // Mobilité AVANT SetupAttachment+Register : Unreal vérifie la
        // compatibilité parent/enfant au moment du RegisterComponent.
        // Si le SMC est encore Static à ce moment, le warning 'cannot attach' s'affiche.
        SMC->SetMobility(EComponentMobility::Movable);
        SMC->SetupAttachment(RootComponent);
        SMC->SetAbsolute(true, true, true);
        SMC->RegisterComponent();

        SMC->SetStaticMesh(StreamMesh);
        SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SMC->SetCastShadow(false);
        SMC->SetForwardAxis(ESplineMeshAxis::Y);
        SMC->SetSplineUpDir(FVector::UpVector, false);
        SMC->SetStartAndEnd(P0, T0, P1, T1, true);

        const float S = StreamRadius / 50.f;
        SMC->SetStartScale(FVector2D(S, S));
        SMC->SetEndScale(FVector2D(S, S));

        if (StreamMaterial)
        {
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(StreamMaterial, this);
            Mat->SetScalarParameterValue(TEXT("Opacity"),      TubeOpacity);
            Mat->SetScalarParameterValue(TEXT("UVOffset"),     0.f);
            Mat->SetScalarParameterValue(TEXT("SegmentIndex"), (float)i);
            SMC->SetMaterial(0, Mat);
        }

        SplineMeshes.Add(SMC);
    }
}

void AWindStreamZone::RebuildCollisionCapsules()
{
    for (UCapsuleComponent* C : CollisionCapsules)
    {
        if (!C) continue;
        C->OnComponentBeginOverlap.RemoveAll(this);
        C->OnComponentEndOverlap.RemoveAll(this);
        C->UnregisterComponent();
        C->DestroyComponent();
    }
    CollisionCapsules.Empty();

    const int32 N           = FMath::Max(NumCollisionCapsules, 2);
    const float TotalLength = Spline->GetSplineLength();
    const float Step        = TotalLength / (N - 1);

    // ── TAILLE DE LA CAPSULE ──────────────────────────────────────────────────
    // Rayon = StreamRadius → correspond exactement au tube visuel
    // HalfHeight = juste assez pour couvrir l'espace entre deux capsules voisines,
    //              avec 10% de marge → pas de gap, pas de capsule énorme
    const float HalfHeight = (Step * 0.55f);

    for (int32 i = 0; i < N; i++)
    {
        const float   Dist     = i * Step;
        const FVector WorldPos = Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
        const FVector Tangent  = Spline->GetTangentAtDistanceAlongSpline(Dist,  ESplineCoordinateSpace::World).GetSafeNormal();

        UCapsuleComponent* Cap = NewObject<UCapsuleComponent>(this,
            *FString::Printf(TEXT("WindCapsule_%d"), i));
        Cap->SetupAttachment(RootComponent);
        Cap->SetAbsolute(true, true, true);
        Cap->RegisterComponent();

        // Rayon = StreamRadius, hauteur juste pour couvrir l'inter-capsule
        Cap->SetCapsuleSize(StreamRadius, FMath::Max(HalfHeight, StreamRadius));
        Cap->SetCollisionProfileName(TEXT("Trigger"));
        Cap->SetGenerateOverlapEvents(true);
        Cap->SetMobility(EComponentMobility::Movable);

        // Oriente l'axe long de la capsule selon la tangente de la spline
        const FRotator CapsuleRot = Tangent.ToOrientationRotator();
        Cap->SetWorldLocationAndRotation(WorldPos,
            FRotator(CapsuleRot.Pitch + 90.f, CapsuleRot.Yaw, CapsuleRot.Roll));

        Cap->OnComponentBeginOverlap.AddDynamic(this, &AWindStreamZone::OnCapsuleBeginOverlap);
        Cap->OnComponentEndOverlap.AddDynamic(this,   &AWindStreamZone::OnCapsuleEndOverlap);
        Cap->SetHiddenInGame(!bWindDebugMode);

        CollisionCapsules.Add(Cap);
    }

    WIND_LOG(Log, TEXT("[WindStream] %d capsules | Rayon: %.0f cm | HalfHeight: %.0f cm"),
        N, StreamRadius, HalfHeight);
}

// ─────────────────────────────────────────────────────────────────────────────
// BEGIN PLAY
// ─────────────────────────────────────────────────────────────────────────────

void AWindStreamZone::BeginPlay()
{
    Super::BeginPlay();
    for (UCapsuleComponent* Cap : CollisionCapsules)
    {
        if (!Cap) continue;
        Cap->OnComponentBeginOverlap.AddDynamic(this, &AWindStreamZone::OnCapsuleBeginOverlap);
        Cap->OnComponentEndOverlap.AddDynamic(this,   &AWindStreamZone::OnCapsuleEndOverlap);
        Cap->SetHiddenInGame(!bWindDebugMode);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TICK
// ─────────────────────────────────────────────────────────────────────────────

void AWindStreamZone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ── Animation UV ──────────────────────────────────────────────────────────
    if (UVScrollSpeed > 0.f)
    {
        UVOffset = FMath::Fmod(UVOffset + UVScrollSpeed * DeltaTime, 1.f);
        for (int32 i = 0; i < SplineMeshes.Num(); i++)
        {
            if (!SplineMeshes[i]) continue;
            UMaterialInstanceDynamic* Mat = Cast<UMaterialInstanceDynamic>(SplineMeshes[i]->GetMaterial(0));
            if (Mat) Mat->SetScalarParameterValue(TEXT("UVOffset"), UVOffset + i * 0.05f);
        }
    }

    WIND_SCREEN(20, PlayersInStream.Num() > 0 ? FColor::Green : FColor::Orange,
        TEXT("[WindStream] Joueurs: %d | Capsules: %d | Rayon: %.0f cm"),
        PlayersInStream.Num(), CollisionCapsules.Num(), StreamRadius);

    // ── Draw debug capsules ────────────────────────────────────────────────────
    if (bWindDebugMode)
    {
        for (UCapsuleComponent* Cap : CollisionCapsules)
        {
            if (!Cap) continue;
            DrawDebugCapsule(GetWorld(),
                Cap->GetComponentLocation(),
                Cap->GetScaledCapsuleHalfHeight(),
                Cap->GetScaledCapsuleRadius(),
                Cap->GetComponentQuat(),
                FColor::Yellow, false, 0.f, 0, 2.f);
        }
    }

    if (PlayersInStream.Num() == 0) return;

    for (APlayerCharacter* Player : PlayersInStream)
    {
        if (!Player) continue;

        if (!IsPlayerInDiveMode(Player))
        {
            WIND_SCREEN(21, FColor::Orange,
                TEXT("[WindStream] %s dans trigger — PAS en DiveMode"), *Player->GetName());
            continue;
        }

        const FVector PlayerPos = Player->GetActorLocation();
        const float   Falloff   = GetRadialFalloff(PlayerPos);

        WIND_SCREEN(22, FColor::Cyan,
            TEXT("[WindStream] %s | Falloff: %.2f | BoostBase: %.0f"),
            *Player->GetName(), Falloff, BoostStrength * Falloff);

        if (Falloff <= 0.f)
        {
            WIND_SCREEN(23, FColor::Yellow,
                TEXT("[WindStream] Falloff=0 — joueur hors rayon spline (%.0f cm)"), StreamRadius);
            continue;
        }

        ApplyWindEffect(Player, DeltaTime, Falloff);

        if (bWindDebugMode)
        {
            const float   Alpha   = GetClosestSplineAlpha(PlayerPos);
            const FVector Closest = Spline->GetLocationAtTime(Alpha, ESplineCoordinateSpace::World);
            const FVector Tangent = Spline->GetTangentAtTime(Alpha,  ESplineCoordinateSpace::World).GetSafeNormal();

            DrawDebugSphere(GetWorld(), Closest, 50.f, 12, FColor::Cyan, false, 0.f, 0, 3.f);
            DrawDebugLine(GetWorld(), PlayerPos, Closest, FColor::Yellow, false, 0.f, 0, 2.f);
            DrawDebugDirectionalArrow(GetWorld(), Closest, Closest + Tangent * 400.f,
                60.f, FColor::Blue, false, 0.f, 0, 5.f);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OVERLAP
// ─────────────────────────────────────────────────────────────────────────────

void AWindStreamZone::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player) return;

    const bool bAlreadyIn = PlayersInStream.Contains(Player);
    PlayersInStream.Add(Player);

    if (!bAlreadyIn)
    {
        WIND_LOG(Log, TEXT("[WindStream] ENTER — %s | DiveMode: %s | via: %s"),
            *Player->GetName(),
            IsPlayerInDiveMode(Player) ? TEXT("OUI") : TEXT("NON"),
            *OverlappedComp->GetName());
        WIND_SCREEN(30, FColor::Green, TEXT("[WindStream] ENTER | %s | DiveMode: %s"),
            *Player->GetName(), IsPlayerInDiveMode(Player) ? TEXT("OUI") : TEXT("NON"));
    }
}

void AWindStreamZone::OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent*, int32)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player) return;

    bool bStillIn = false;
    for (UCapsuleComponent* Cap : CollisionCapsules)
    {
        if (!Cap || Cap == OverlappedComp) continue;
        if (Cap->IsOverlappingActor(Player)) { bStillIn = true; break; }
    }

    if (!bStillIn)
    {
        PlayersInStream.Remove(Player);
        WIND_LOG(Log, TEXT("[WindStream] EXIT — %s"), *Player->GetName());
        WIND_SCREEN(30, FColor::Orange, TEXT("[WindStream] EXIT | %s"), *Player->GetName());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

float AWindStreamZone::GetClosestSplineAlpha(const FVector& WorldPosition) const
{
    const float Dist  = Spline->GetDistanceAlongSplineAtLocation(WorldPosition, ESplineCoordinateSpace::World);
    const float Total = Spline->GetSplineLength();
    return Total > 0.f ? FMath::Clamp(Dist / Total, 0.f, 1.f) : 0.f;
}

float AWindStreamZone::GetRadialFalloff(const FVector& WorldPosition) const
{
    const float   Alpha   = GetClosestSplineAlpha(WorldPosition);
    const FVector Closest = Spline->GetLocationAtTime(Alpha, ESplineCoordinateSpace::World);
    const float   Dist    = FVector::Dist(WorldPosition, Closest);

    if (Dist >= StreamRadius) return 0.f;
    return FMath::Clamp(FMath::Cos((Dist / StreamRadius) * PI * 0.5f), 0.f, 1.f);
}

bool AWindStreamZone::IsPlayerInDiveMode(APlayerCharacter* Player) const
{
    if (!Player) return false;
    UFlightComponent* FC = Player->FindComponentByClass<UFlightComponent>();
    return FC && FC->IsInMode(EFlightMode::Dive);
}

UDiveMode* AWindStreamZone::GetPlayerDiveMode(APlayerCharacter* Player) const
{
    if (!Player) return nullptr;
    UFlightComponent* FC = Player->FindComponentByClass<UFlightComponent>();
    if (!FC || !FC->IsInMode(EFlightMode::Dive)) return nullptr;
    return FC->GetDiveMode();
}

void AWindStreamZone::ApplyWindEffect(APlayerCharacter* Player, float DeltaTime, float Falloff)
{
    UDiveMode* Dive = GetPlayerDiveMode(Player);
    if (!Dive) return;

    const FVector PlayerPos = Player->GetActorLocation();
    const float   Alpha     = GetClosestSplineAlpha(PlayerPos);
    const FVector Tangent   = Spline->GetTangentAtTime(Alpha, ESplineCoordinateSpace::World).GetSafeNormal();

    // Point de la spline le plus proche du joueur (position 3D cible)
    const FVector ClosestPoint = Spline->GetLocationAtTime(Alpha, ESplineCoordinateSpace::World);

    // ── Boost de vitesse horizontale ──────────────────────────────────────────
    const float SpeedBoost = BoostStrength * Falloff * DeltaTime;

    // ── Correction verticale ──────────────────────────────────────────────────
    // On calcule l'écart Z entre le joueur et la spline à ce point.
    // On pousse Velocity.Z pour ramener le joueur vers la hauteur de la spline,
    // proportionnellement au Falloff (moins fort sur les bords).
    // Le joueur reste libre de sortir en pitchant fort ou en steerant.
    const float DeltaZ        = ClosestPoint.Z - PlayerPos.Z;
    // Force proportionnelle à l'écart — plus loin = pousse plus fort
    // VerticalCorrectionStrength est en cm/s² par cm d'écart (spring-like)
    const float VerticalForce = DeltaZ * VerticalCorrectionStrength * Falloff;

    WIND_SCREEN(31, FColor::Cyan,
        TEXT("[WindStream] SpeedBoost: +%.0f | DeltaZ: %.0f cm | VertForce: %.0f | Falloff: %.2f"),
        SpeedBoost, DeltaZ, VerticalForce, Falloff);

    Dive->ApplyWindBoost(SpeedBoost, Tangent, DirectionInfluence, VerticalForce, DeltaTime);
}