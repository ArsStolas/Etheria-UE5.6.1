/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "LockVisualComponent" - Source
 * Notes: Implements the visual side of the lock-on system by reacting to lock changes, applying/removing overlay materials, updating DOF and driving UI events.
 */

#include "Components/Combat/LockTarget/LockVisualComponent.h"

#include "Components/Combat/LockTarget/LockTargetComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

ULockVisualComponent::ULockVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // DOF update is only needed while locked or restoring base DOF.
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveOwnerRefs();

    if (LockComp.IsValid())
    {
        LockComp->OnLockChanged.AddDynamic(this, &ULockVisualComponent::HandleLockChanged);
    }

    if (CameraComp.IsValid())
    {
        const FPostProcessSettings& PPS = CameraComp->PostProcessSettings;
        BaseFocalDistance = PPS.DepthOfFieldFocalDistance;
        BaseFStop         = PPS.DepthOfFieldFstop;
        bHadFStopOverride = PPS.bOverride_DepthOfFieldFstop;
        bHadFocusOverride = PPS.bOverride_DepthOfFieldFocalDistance;
    }

    SetComponentTickEnabled(false);
}

void ULockVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Make sure to clear any overlay on shutdown.
    if (CurrentTarget.IsValid())
    {
        ApplyOverlayToActor(CurrentTarget.Get(), false);
    }

    Super::EndPlay(EndPlayReason);
}

void ULockVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableDOF || !CameraComp.IsValid())
    {
        SetComponentTickEnabled(false);
        return;
    }

    UpdateDepthOfField(DeltaTime);
}

void ULockVisualComponent::HandleLockChanged(AActor* NewTarget)
{
    AActor* OldTarget = CurrentTarget.Get();

    // If there was no target before and still no target now, nothing really changed.
    // Avoid touching overlay or DOF in that case.
    if (!OldTarget && !NewTarget)
    {
        return;
    }

    // Handle overlay material swap.
    if (bUseOverlayMaterial)
    {
        if (OldTarget && OldTarget != NewTarget)
        {
            ApplyOverlayToActor(OldTarget, false);
        }
        if (NewTarget)
        {
            ApplyOverlayToActor(NewTarget, true);
        }
    }
    else
    {
        // If overlay use has been disabled, ensure we restore any previous override.
        if (OldTarget)
        {
            ApplyOverlayToActor(OldTarget, false);
        }
    }

    CurrentTarget = NewTarget;

    const bool bHasLock = (NewTarget != nullptr);

    // Drive DOF tick state.
    if (bEnableDOF)
    {
        if (bHasLock)
        {
            bRestoringBaseDOF = false;
            SetComponentTickEnabled(true);
        }
        else
        {
            bRestoringBaseDOF = bRestoreBaseDOFOnUnlock;
            if (bRestoringBaseDOF)
            {
                SetComponentTickEnabled(true);
            }
            else
            {
                SetComponentTickEnabled(false);
            }
        }
    }
    else
    {
        SetComponentTickEnabled(false);
    }

    OnCinematicLockChanged.Broadcast(bHasLock);
}

void ULockVisualComponent::ApplyOverlayToActor(AActor* Target, bool bEnable)
{
    if (!Target)
    {
        return;
    }

    if (!bUseOverlayMaterial && bEnable)
    {
        return;
    }

    // --- DISABLE :
    if (!bEnable)
    {
        if (OverlayBackups.Num() > 0)
        {
            for (FEEOverlayBackup& Backup : OverlayBackups)
            {
                if (!Backup.Mesh.IsValid())
                {
                    continue;
                }

                UMeshComponent* Mesh = Backup.Mesh.Get();
                // Remove Overlay Material
                Mesh->SetOverlayMaterial(Backup.PreviousOverlay);
            }

            OverlayBackups.Reset();
        }
        return;
    }

    // --- ENABLE :
    if (!OverlayMaterial)
    {
        return;
    }

    // Clean the target
    OverlayBackups.Reset();

    TArray<UMeshComponent*> Meshes;
    Target->GetComponents<UMeshComponent>(Meshes);

    for (UMeshComponent* Mesh : Meshes)
    {
        if (!Mesh)
        {
            continue;
        }

        FEEOverlayBackup Backup;
        Backup.Mesh           = Mesh;
        Backup.PreviousOverlay = Mesh->GetOverlayMaterial();

        OverlayBackups.Add(Backup);

        // Apply Overlay Material
        Mesh->SetOverlayMaterial(OverlayMaterial);
    }
}

void ULockVisualComponent::UpdateDepthOfField(float DeltaTime)
{
    if (!CameraComp.IsValid())
    {
        return;
    }

    FPostProcessSettings& PPS = CameraComp->PostProcessSettings;

    const bool bHasTarget = CurrentTarget.IsValid();

    float TargetFocalDistance = BaseFocalDistance;
    float TargetFStop         = BaseFStop;

    if (bHasTarget)
    {
        const FVector CamLoc    = CameraComp->GetComponentLocation();
        const FVector TargetLoc = CurrentTarget->GetActorLocation();
        TargetFocalDistance     = FVector::Dist(CamLoc, TargetLoc);
        TargetFStop             = LockedFStop;
    }

    PPS.bOverride_DepthOfFieldFocalDistance = true;
    PPS.bOverride_DepthOfFieldFstop         = true;

    PPS.DepthOfFieldFocalDistance = FMath::FInterpTo(
        PPS.DepthOfFieldFocalDistance,
        TargetFocalDistance,
        DeltaTime,
        DOFFocusInterpSpeed
    );

    PPS.DepthOfFieldFstop = FMath::FInterpTo(
        PPS.DepthOfFieldFstop,
        TargetFStop,
        DeltaTime,
        DOFFocusInterpSpeed
    );

    // When no target and we are restoring base DOF, stop ticking once we are close enough.
    if (!bHasTarget && bRestoringBaseDOF)
    {
        const bool bCloseToBaseDistance = FMath::IsNearlyEqual(PPS.DepthOfFieldFocalDistance, BaseFocalDistance, 1.f);
        const bool bCloseToBaseFStop    = FMath::IsNearlyEqual(PPS.DepthOfFieldFstop, BaseFStop, 0.05f);

        if (bCloseToBaseDistance && bCloseToBaseFStop)
        {
            bRestoringBaseDOF = false;
            SetComponentTickEnabled(false);

            // Restore original override flags so we do not permanently hijack DOF settings.
            PPS.bOverride_DepthOfFieldFocalDistance = bHadFocusOverride;
            PPS.bOverride_DepthOfFieldFstop         = bHadFStopOverride;
        }
    }
}

void ULockVisualComponent::ResolveOwnerRefs()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    LockComp   = Owner->FindComponentByClass<ULockTargetComponent>();
    CameraComp = Owner->FindComponentByClass<UCameraComponent>();
}
