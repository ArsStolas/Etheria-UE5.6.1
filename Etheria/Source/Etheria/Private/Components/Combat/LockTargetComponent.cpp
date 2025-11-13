/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "LockTargetComponent" - Source
 */
#include "Components/Combat/LockTargetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

ULockTargetComponent::ULockTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULockTargetComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool ULockTargetComponent::ToggleLock(AActor* Preferred)
{
    if (CurrentTarget.IsValid())
    {
        ClearLock();
        return false;
    }
    CurrentTarget = FindBestTarget(Preferred);
    OnLockChanged.Broadcast(CurrentTarget.Get());
    return CurrentTarget.IsValid();
}

void ULockTargetComponent::ClearLock()
{
    CurrentTarget = nullptr;
    OnLockChanged.Broadcast(nullptr);
}

bool ULockTargetComponent::SwitchTarget(bool bRight)
{
    if (!CurrentTarget.IsValid()) return false;

    AActor* Owner = GetOwner();
    if (!Owner) return false;

    TArray<AActor*> Candidates = GatherCandidates();
    if (Candidates.Num() == 0) return false;

    FVector EyeLoc = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();

    float BestScore = -FLT_MAX;
    AActor* Best = nullptr;

    const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();

    for (AActor* C : Candidates)
    {
        if (!C || C == CurrentTarget.Get()) continue;
        FVector Dir = (C->GetActorLocation() - EyeLoc).GetSafeNormal();
        float side = FVector::DotProduct(Dir, Right);
        if (bRight && side < 0.f) continue;
        if (!bRight && side > 0.f) continue;

        float score = ScoreCandidate(C, EyeLoc, Forward);
        if (score > BestScore) { BestScore = score; Best = C; }
    }

    if (Best)
    {
        CurrentTarget = Best;
        OnLockChanged.Broadcast(Best);
        return true;
    }
    return false;
}

AActor* ULockTargetComponent::FindBestTarget(AActor* Preferred) const
{
    if (Preferred) return Preferred;

    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    TArray<AActor*> Candidates = GatherCandidates();
    if (Candidates.Num() == 0) return nullptr;

    FVector EyeLoc = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();

    float BestScore = -FLT_MAX;
    AActor* Best = nullptr;
    for (AActor* C : Candidates)
    {
        float Score = ScoreCandidate(C, EyeLoc, Forward);
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = C;
        }
    }
    return Best;
}

TArray<AActor*> ULockTargetComponent::GatherCandidates() const
{
    TArray<AActor*> Result;

    AActor* Owner = GetOwner();
    if (!Owner) return Result;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(TargetChannel));

    TArray<AActor*> Ignore;
    Ignore.Add(Owner);

    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
        Owner,
        Owner->GetActorLocation(),
        MaxDistance,
        ObjTypes,
        AActor::StaticClass(),
        Ignore,
        OutActors
    );

    if (!bHit) return Result;

    FVector EyeLoc = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();

    for (AActor* A : OutActors)
    {
        if (!A || A == Owner) continue;

        const float DistSq = FVector::DistSquared(EyeLoc, A->GetActorLocation());
        if (DistSq > MaxDistance * MaxDistance) continue;

        const FVector Dir = (A->GetActorLocation() - EyeLoc).GetSafeNormal();
        const float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Dir, Forward)));
        if (Angle <= MaxAngleDeg)
        {
            Result.Add(A);
        }
    }
    return Result;
}

float ULockTargetComponent::ScoreCandidate(AActor* Candidate, const FVector& EyeLoc, const FVector& Forward) const
{
    const FVector Dir = (Candidate->GetActorLocation() - EyeLoc).GetSafeNormal();
    const float Dot = FVector::DotProduct(Dir, Forward);
    const float Dist = FVector::Dist(EyeLoc, Candidate->GetActorLocation());
    return Dot * 2.0f + (1.0f - FMath::Clamp(Dist / MaxDistance, 0.f, 1.f));
}
