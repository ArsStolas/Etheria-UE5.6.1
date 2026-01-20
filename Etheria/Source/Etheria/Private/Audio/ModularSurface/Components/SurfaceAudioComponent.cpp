/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USurfaceAudioComponent" - Source
 */

#include "Audio/ModularSurface/Components/SurfaceAudioComponent.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

USurfaceAudioComponent::USurfaceAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USurfaceAudioComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bEnableLandingVelocityCache)
    {
        // Optional: user can start/stop this manually.
        // We do NOT auto-start here because we want perf: only run while Falling.
    }
}

void USurfaceAudioComponent::SetWetness(float NewWetness)
{
    Wetness = FMath::Clamp(NewWetness, 0.0f, 1.0f);
}

bool USurfaceAudioComponent::ResolveFootWorldLocation(EFootstepFoot Foot, FName FootSocketName, bool bTryIK, FVector& OutWorldLocation) const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    // IK provider (optional)
    if (bTryIK && Owner->GetClass()->ImplementsInterface(UFootPlacementProviderInterface::StaticClass()))
    {
        FVector IKLoc = FVector::ZeroVector;
        const bool bGotIK = IFootPlacementProviderInterface::Execute_GetFootWorldLocation(Owner, Foot, IKLoc);
        if (bGotIK)
        {
            OutWorldLocation = IKLoc;
            return true;
        }
    }

    // Socket location (recommended)
    if (const ACharacter* Char = Cast<ACharacter>(Owner))
    {
        if (const USkeletalMeshComponent* Mesh = Char->GetMesh())
        {
            if (Mesh->DoesSocketExist(FootSocketName))
            {
                OutWorldLocation = Mesh->GetSocketLocation(FootSocketName);
                return true;
            }
        }
    }

    OutWorldLocation = Owner->GetActorLocation();
    return true;
}

EPhysicalSurface USurfaceAudioComponent::GetSurfaceTypeAtLocation(const FVector& WorldLocation, FHitResult* OutHit) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return SurfaceType_Default;
    }

    const FVector Start = WorldLocation + FVector(0.0f, 0.0f, 30.0f);
    const FVector End = WorldLocation - FVector(0.0f, 0.0f, TraceDistance);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ModularSurfaceAudioTrace), bTraceComplex);
    Params.bReturnPhysicalMaterial = true;
    Params.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

    if (bDebugDraw)
    {
        const FColor Color = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(World, Start, End, Color, false, DebugDrawDuration, 0, 1.0f);
        if (bHit)
        {
            DrawDebugPoint(World, Hit.ImpactPoint, 8.0f, Color, false, DebugDrawDuration);
        }
    }

    if (OutHit)
    {
        *OutHit = Hit;
    }

    if (!bHit)
    {
        return SurfaceType_Default;
    }

    return UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
}

const FSurfaceSoundList& USurfaceAudioComponent::SelectFootstepList(const FFootstepAudioSet& Set, EFootstepGait Gait) const
{
    switch (Gait)
    {
        case EFootstepGait::Run:
            return Set.Run;
        case EFootstepGait::Sprint:
            return Set.Sprint;
        case EFootstepGait::Walk:
        default:
            return Set.Walk;
    }
}

USoundBase* USurfaceAudioComponent::PickRandomSound(const FSurfaceSoundList& List, int32& InOutLastIndex) const
{
    const bool bIsWet = (WetPlaybackMode == EWetPlaybackMode::Switch) ? (Wetness >= WetnessThreshold) : (Wetness > 0.0f);

    const TArray<TObjectPtr<USoundBase>>& Pool = (bIsWet && List.Wet.Num() > 0) ? List.Wet : List.Dry;
    if (Pool.Num() == 0)
    {
        return nullptr;
    }

    if (Pool.Num() == 1)
    {
        InOutLastIndex = 0;
        return Pool[0];
    }

    int32 Index = FMath::RandRange(0, Pool.Num() - 1);
    if (List.bAvoidImmediateRepeat && Index == InOutLastIndex)
    {
        Index = (Index + 1 + FMath::RandRange(0, Pool.Num() - 2)) % Pool.Num();
    }

    InOutLastIndex = Index;
    return Pool[Index];
}

void USurfaceAudioComponent::SpawnOneShotAtLocation(USoundBase* Sound, const FVector& Location, float Volume, float Pitch) const
{
    if (!Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Volume, Pitch);
}

void USurfaceAudioComponent::PlayOneShotFromList(const FSurfaceSoundList& List, int32& InOutLastIndex, const FVector& Location, float VolumeMul, float PitchMul) const
{
    // Switch mode: play only one list (dry or wet)
    if (WetPlaybackMode == EWetPlaybackMode::Switch)
    {
        USoundBase* Picked = PickRandomSound(List, InOutLastIndex);
        if (!Picked)
        {
            return;
        }

        const float Vol = FMath::RandRange(List.VolumeMin, List.VolumeMax) * VolumeMul;
        const float Pit = FMath::RandRange(List.PitchMin, List.PitchMax) * PitchMul;
        SpawnOneShotAtLocation(Picked, Location, Vol, Pit);
        return;
    }

    // Crossfade mode: optionally play both dry and wet with weights.
    // Perf note: this can play 2 one-shots per event (enable only if you want the richer feel).
    const float WetW = FMath::Clamp(Wetness, 0.0f, 1.0f);
    const float DryW = 1.0f - WetW;

    if (DryW > 0.001f && List.Dry.Num() > 0)
    {
        int32 DummyLast = InOutLastIndex;
        const FSurfaceSoundList DryList = [] (const FSurfaceSoundList& In) { FSurfaceSoundList Out = In; Out.Wet.Reset(); return Out; }(List);
        USoundBase* PickedDry = PickRandomSound(DryList, DummyLast);
        if (PickedDry)
        {
            const float Vol = FMath::RandRange(List.VolumeMin, List.VolumeMax) * VolumeMul * DryW;
            const float Pit = FMath::RandRange(List.PitchMin, List.PitchMax) * PitchMul;
            SpawnOneShotAtLocation(PickedDry, Location, Vol, Pit);
        }
    }

    if (WetW > 0.001f && List.Wet.Num() > 0)
    {
        int32 DummyLast = InOutLastIndex;
        const FSurfaceSoundList WetList = [] (const FSurfaceSoundList& In) { FSurfaceSoundList Out = In; Out.Dry.Reset(); return Out; }(List);
        USoundBase* PickedWet = PickRandomSound(WetList, DummyLast);
        if (PickedWet)
        {
            const float Vol = FMath::RandRange(List.VolumeMin, List.VolumeMax) * VolumeMul * WetW;
            const float Pit = FMath::RandRange(List.PitchMin, List.PitchMax) * PitchMul;
            SpawnOneShotAtLocation(PickedWet, Location, Vol, Pit);
        }
    }
}

void USurfaceAudioComponent::PlayFootstepFromNotify(EFootstepFoot Foot, EFootstepGait Gait, FName FootSocketName, bool bTryIK)
{
    if (!Library)
    {
        return;
    }

    FVector FootWorld = FVector::ZeroVector;
    if (!ResolveFootWorldLocation(Foot, FootSocketName, bTryIK, FootWorld))
    {
        return;
    }

    FHitResult Hit;
    const EPhysicalSurface Surface = GetSurfaceTypeAtLocation(FootWorld, &Hit);
    const FSurfaceAudioEntry& Entry = Library->GetEntry(Surface);

    const FSurfaceSoundList& List = SelectFootstepList(Entry.Footsteps, Gait);
    if (List.Dry.Num() == 0 && List.Wet.Num() == 0)
    {
        return;
    }

    const FVector Location = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : FootWorld;
    const FVector Normal = Hit.bBlockingHit ? Hit.ImpactNormal : FVector::UpVector;

    // In Switch mode, the wet/dry decision is inside PlayOneShotFromList.
    PlayOneShotFromList(List, LastFootstepIndex, Location, 1.0f, 1.0f);

    const bool bIsWet = (WetPlaybackMode == EWetPlaybackMode::Switch) ? (Wetness >= WetnessThreshold) : (Wetness > 0.0f);
    OnFootstepPlayed.Broadcast(Foot, Gait, TEnumAsByte<EPhysicalSurface>(Surface), Location, Normal, bIsWet);
}

void USurfaceAudioComponent::PlayLandingFromHit(const FHitResult& Hit, float ImpactSpeedAbs)
{
    if (!Library)
    {
        return;
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    EPhysicalSurface Surface = SurfaceType_Default;
    if (Hit.bBlockingHit)
    {
        Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
    }

    const FSurfaceAudioEntry& Entry = Library->GetEntry(Surface);
    const FLandingAudioConfig& Landing = Entry.Landing;

    if (ImpactSpeedAbs < Landing.MinImpactSpeed)
    {
        return;
    }

    if ((Now - LastLandingPlayTime) < Landing.CooldownSeconds)
    {
        return;
    }

    LastLandingPlayTime = Now;

    float VolMul = 1.0f;
    float PitMul = 1.0f;

    if (Landing.VolumeCurve)
    {
        VolMul *= FMath::Max(0.0f, Landing.VolumeCurve->GetFloatValue(ImpactSpeedAbs));
    }

    if (Landing.PitchCurve)
    {
        PitMul *= FMath::Max(0.0f, Landing.PitchCurve->GetFloatValue(ImpactSpeedAbs));
    }

    FVector Location = GetOwner()->GetActorLocation();
    if (Hit.bBlockingHit)
    {
        Location = FVector(Hit.ImpactPoint);
    }

    PlayOneShotFromList(Landing.Sounds, LastLandingIndex, Location, VolMul, PitMul);
}

void USurfaceAudioComponent::PlayLandingFromCache(const FHitResult& Hit)
{
    float Impact = 0.0f;

    if (bLandingCacheActive)
    {
        Impact = CachedAbsVelocityZ;
    }
    else
    {
        if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
        {
            if (const UCharacterMovementComponent* Move = Char->GetCharacterMovement())
            {
                Impact = FMath::Abs(Move->Velocity.Z);
            }
        }
    }

    PlayLandingFromHit(Hit, Impact);
}

void USurfaceAudioComponent::StartLandingVelocityCache()
{
    if (!bEnableLandingVelocityCache)
    {
        return;
    }

    if (bLandingCacheActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bLandingCacheActive = true;
    CachedAbsVelocityZ = 0.0f;

    World->GetTimerManager().SetTimer(LandingVelocityTimerHandle, this, &USurfaceAudioComponent::HandleLandingVelocitySample, LandingVelocitySampleInterval, true);
}

void USurfaceAudioComponent::StopLandingVelocityCache()
{
    if (!bLandingCacheActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(LandingVelocityTimerHandle);
    }

    bLandingCacheActive = false;
}

void USurfaceAudioComponent::HandleLandingVelocitySample()
{
    const ACharacter* Char = Cast<ACharacter>(GetOwner());
    if (!Char)
    {
        return;
    }

    const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
    if (!Move)
    {
        return;
    }

    CachedAbsVelocityZ = FMath::Max(CachedAbsVelocityZ, FMath::Abs(Move->Velocity.Z));
}
