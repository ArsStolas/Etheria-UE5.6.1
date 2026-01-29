/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ACinematicCameraRigBase" - Source
 */
#include "Core/Cinematics/CinematicCameraRigBase.h"
#include "CineCameraComponent.h"
#include "GameFramework/Actor.h"

ACinematicCameraRigBase::ACinematicCameraRigBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    CineCamera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("CineCamera"));
    CineCamera->SetupAttachment(Root);

    SetActorEnableCollision(false);
}

void ACinematicCameraRigBase::BeginPlay()
{
    Super::BeginPlay();
    StopRig();
}

void ACinematicCameraRigBase::StartRig()
{
    bRunning = true;
    SetActorTickEnabled(true);

    if (FollowMode == ECameraRigFollowMode::Attach && TargetActor.IsValid())
    {
        AttachToActor(TargetActor.Get(), FAttachmentTransformRules::KeepWorldTransform);
    }
}

void ACinematicCameraRigBase::StopRig()
{
    bRunning = false;
    SetActorTickEnabled(false);
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ACinematicCameraRigBase::SetTargetActor(AActor* InTarget)
{
    TargetActor = InTarget;

    if (FollowMode == ECameraRigFollowMode::Attach)
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        if (TargetActor.IsValid())
        {
            AttachToActor(TargetActor.Get(), FAttachmentTransformRules::KeepWorldTransform);
        }
    }
}

void ACinematicCameraRigBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bRunning || !TargetActor.IsValid())
    {
        return;
    }

    UpdateFollow(DeltaSeconds);

    if (bLookAtTarget)
    {
        UpdateLookAt(DeltaSeconds);
    }
}

void ACinematicCameraRigBase::UpdateFollow(float DeltaSeconds)
{
    AActor* Target = TargetActor.Get();
    if (!Target)
    {
        return;
    }

    if (FollowMode == ECameraRigFollowMode::Attach)
    {
        // When attached, we just keep our relative offset.
        SetActorRelativeLocation(RelativeOffset);
        return;
    }

    const FVector DesiredWorldLoc = Target->GetActorLocation() + RelativeOffset;
    const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), DesiredWorldLoc, DeltaSeconds, FollowInterpSpeed);
    SetActorLocation(NewLoc);
}

void ACinematicCameraRigBase::UpdateLookAt(float DeltaSeconds)
{
    AActor* Target = TargetActor.Get();
    if (!Target)
    {
        return;
    }

    const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation());
    if (ToTarget.IsNearlyZero())
    {
        return;
    }

    const FRotator DesiredRot = ToTarget.Rotation();
    const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), DesiredRot, DeltaSeconds, RotationInterpSpeed);
    SetActorRotation(NewRot);
}
