/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCinematicWorldSubsystem" - Source
 */
#include "Core/Cinematics/CinematicWorldSubsystem.h"
#include "Core/Cinematics/CinematicCameraRigBase.h"
#include "Core/Cinematics/Components/PlayerCinematicComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Core/Cinematics/CinematicSystemSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UCinematicWorldSubsystem::Deinitialize()
{
    EndCinematicSession(ECinematicEndReason::Cancelled);

    // Pop all layers
    TArray<FName> Layers = CameraLayerStack;
    for (const FName& Layer : Layers)
    {
        PopCameraLayer(Layer, 0.0f);
    }

    Super::Deinitialize();
}

bool UCinematicWorldSubsystem::StartCinematicSession(const FCinematicSessionRequest& Request)
{
    if (bCinematicActive)
    {
        EndCinematicSession(ECinematicEndReason::Interrupted);
    }

ActiveRequest = Request;

const UCinematicSystemSettings* Settings = UCinematicSystemSettings::Get();
if (Settings)
{
    if (!ActiveRequest.CameraRigClass && Settings->DefaultRigClass)
    {
        ActiveRequest.CameraRigClass = Settings->DefaultRigClass;
    }
    if (ActiveRequest.BlendInTime <= 0.0f)
    {
        ActiveRequest.BlendInTime = Settings->DefaultBlendInTime;
    }
    if (ActiveRequest.BlendOutTime <= 0.0f)
    {
        ActiveRequest.BlendOutTime = Settings->DefaultBlendOutTime;
    }
}

    bCinematicActive = true;

    ApplyCinematicPolicy();
    OnCinematicSessionStarted.Broadcast(ActiveRequest);

    // Camera: rig first, else static camera actor
    AActor* Target = ActiveRequest.Target.Get();
    AActor* StaticCam = ActiveRequest.StaticCameraActor.Get();

    if (ActiveRequest.CameraRigClass && Target)
    {
        PushCameraRigLayer(TEXT("Cinematic"), ActiveRequest.CameraRigClass, Target, ActiveRequest.BlendInTime);
    }
    else if (StaticCam)
    {
        PushCameraActorLayer(TEXT("Cinematic"), StaticCam, ActiveRequest.BlendInTime);
    }

    if (ActiveRequest.AutoEndAfter > 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(AutoEndTimer, FTimerDelegate::CreateUObject(this, &UCinematicWorldSubsystem::EndCinematicSession, ECinematicEndReason::Completed), ActiveRequest.AutoEndAfter, false);
        }
    }

    return true;
}

void UCinematicWorldSubsystem::EndCinematicSession(ECinematicEndReason Reason)
{
    if (!bCinematicActive)
    {
        return;
    }

    ClearAutoEndTimer();

    // Release cinematic camera
    PopCameraLayer(TEXT("Cinematic"), ActiveRequest.BlendOutTime);

    RestoreCinematicPolicy();

    bCinematicActive = false;
    OnCinematicSessionEnded.Broadcast(Reason);
}

void UCinematicWorldSubsystem::SkipCinematic()
{
    if (!bCinematicActive)
    {
        return;
    }

    if (!ActiveRequest.Policy.bAllowSkip)
    {
        return;
    }

    EndCinematicSession(ECinematicEndReason::Skipped);
}

bool UCinematicWorldSubsystem::PushCameraRigLayer(FName LayerName, TSubclassOf<ACinematicCameraRigBase> RigClass, AActor* TargetActor, float BlendInTime)
{
    if (!RigClass || !TargetActor)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // Replace existing layer if present
    if (FCameraLayerData* Existing = LayerData.Find(LayerName))
    {
        if (Existing->Rig.IsValid())
        {
            Existing->Rig->SetTargetActor(TargetActor);
            ApplyTopCamera(BlendInTime);
            return true;
        }

        // If previously a static camera, remove it and recreate as rig
        PopCameraLayer(LayerName, 0.0f);
    }

    ACinematicCameraRigBase* Rig = World->SpawnActor<ACinematicCameraRigBase>(RigClass);
    if (!Rig)
    {
        return false;
    }

    Rig->SetTargetActor(TargetActor);
    Rig->StartRig();

    FCameraLayerData Data;
    Data.ViewTarget = Rig;
    Data.Rig = Rig;
    LayerData.Add(LayerName, Data);

    CameraLayerStack.Remove(LayerName);
    CameraLayerStack.Add(LayerName);

    ApplyTopCamera(BlendInTime);
    return true;
}

bool UCinematicWorldSubsystem::PushCameraActorLayer(FName LayerName, AActor* CameraActor, float BlendInTime)
{
    if (!CameraActor)
    {
        return false;
    }

    if (LayerData.Contains(LayerName))
    {
        PopCameraLayer(LayerName, 0.0f);
    }

    FCameraLayerData Data;
    Data.ViewTarget = CameraActor;
    Data.Rig = nullptr;
    LayerData.Add(LayerName, Data);

    CameraLayerStack.Remove(LayerName);
    CameraLayerStack.Add(LayerName);

    ApplyTopCamera(BlendInTime);
    return true;
}

void UCinematicWorldSubsystem::PopCameraLayer(FName LayerName, float BlendOutTime)
{
    if (!LayerData.Contains(LayerName))
    {
        return;
    }

    // Destroy rig if any
    if (FCameraLayerData* Data = LayerData.Find(LayerName))
    {
        if (Data->Rig.IsValid())
        {
            Data->Rig->StopRig();
            Data->Rig->Destroy();
        }
    }

    LayerData.Remove(LayerName);
    CameraLayerStack.Remove(LayerName);

    ApplyTopCamera(BlendOutTime);
}

bool UCinematicWorldSubsystem::HasCameraLayer(FName LayerName) const
{
    return LayerData.Contains(LayerName);
}

ACinematicCameraRigBase* UCinematicWorldSubsystem::GetActiveRigForLayer(FName LayerName) const
{
    if (const FCameraLayerData* Data = LayerData.Find(LayerName))
    {
        return Data->Rig.Get();
    }
    return nullptr;
}

void UCinematicWorldSubsystem::ApplyTopCamera(float BlendTime)
{
    APlayerController* PC = ResolvePlayerController();
    if (!PC)
    {
        return;
    }

    AActor* DesiredViewTarget = nullptr;

    if (CameraLayerStack.Num() > 0)
    {
        const FName TopLayer = CameraLayerStack.Last();
        if (const FCameraLayerData* Data = LayerData.Find(TopLayer))
        {
            DesiredViewTarget = Data->ViewTarget.Get();
        }
    }

    if (!DesiredViewTarget)
    {
        // Fallback to gameplay target
        DesiredViewTarget = PC->GetPawn();
    }

    if (DesiredViewTarget)
    {
        PC->SetViewTargetWithBlend(DesiredViewTarget, BlendTime);
    }
}

APlayerController* UCinematicWorldSubsystem::ResolvePlayerController() const
{
    if (UWorld* World = GetWorld())
    {
        return UGameplayStatics::GetPlayerController(World, 0);
    }
    return nullptr;
}

void UCinematicWorldSubsystem::ClearAutoEndTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoEndTimer);
    }
}

void UCinematicWorldSubsystem::ApplyCinematicPolicy()
{
    if (UPlayerCinematicComponent* PlayerComp = ResolvePlayerCinematicComponent())
    {
        PlayerComp->ApplyCinematicPolicy(ActiveRequest.Policy);
    }
}

void UCinematicWorldSubsystem::RestoreCinematicPolicy()
{
    if (UPlayerCinematicComponent* PlayerComp = ResolvePlayerCinematicComponent())
    {
        PlayerComp->RestoreCinematicPolicy();
    }
}

UPlayerCinematicComponent* UCinematicWorldSubsystem::ResolvePlayerCinematicComponent() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC)
    {
        if (UPlayerCinematicComponent* CompOnPC = PC->FindComponentByClass<UPlayerCinematicComponent>())
        {
            return CompOnPC;
        }

        if (APawn* Pawn = PC->GetPawn())
        {
            return Pawn->FindComponentByClass<UPlayerCinematicComponent>();
        }
    }

    return nullptr;
}
