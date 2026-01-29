/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCinematicWorldSubsystem" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/Cinematics/Data/CinematicTypes.h"
#include "CinematicWorldSubsystem.generated.h"

class ACinematicCameraRigBase;
class UPlayerCinematicComponent;
class APlayerController;

/**
 * World-level cinematic orchestration:
 * - Camera control via named layers (e.g. "Cinematic", "Dialogue")
 * - Optional cinematic sessions (policy + auto end)
 * This stays Sequencer-free by default (you can still play Level Sequences separately if desired).
 */
UCLASS(BlueprintType)
class ETHERIA_API UCinematicWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    // Session API
    UFUNCTION(BlueprintCallable, Category="Cinematic|Session")
    bool StartCinematicSession(const FCinematicSessionRequest& Request);

    UFUNCTION(BlueprintCallable, Category="Cinematic|Session")
    void EndCinematicSession(ECinematicEndReason Reason);

    UFUNCTION(BlueprintPure, Category="Cinematic|Session")
    bool IsCinematicActive() const { return bCinematicActive; }

    UFUNCTION(BlueprintCallable, Category="Cinematic|Session")
    void SkipCinematic();

    // Camera layer API (usable by Dialogue subsystem or by BP)
    UFUNCTION(BlueprintCallable, Category="Cinematic|Camera")
    bool PushCameraRigLayer(FName LayerName, TSubclassOf<ACinematicCameraRigBase> RigClass, AActor* TargetActor, float BlendInTime);

    UFUNCTION(BlueprintCallable, Category="Cinematic|Camera")
    bool PushCameraActorLayer(FName LayerName, AActor* CameraActor, float BlendInTime);

    UFUNCTION(BlueprintCallable, Category="Cinematic|Camera")
    void PopCameraLayer(FName LayerName, float BlendOutTime);

    UFUNCTION(BlueprintPure, Category="Cinematic|Camera")
    bool HasCameraLayer(FName LayerName) const;

    UFUNCTION(BlueprintPure, Category="Cinematic|Camera")
    ACinematicCameraRigBase* GetActiveRigForLayer(FName LayerName) const;

    // Delegates
    UPROPERTY(BlueprintAssignable, Category="Cinematic|Events")
    FOnCinematicSessionStarted OnCinematicSessionStarted;

    UPROPERTY(BlueprintAssignable, Category="Cinematic|Events")
    FOnCinematicSessionEnded OnCinematicSessionEnded;

private:
    bool bCinematicActive = false;
    FCinematicSessionRequest ActiveRequest;

    FTimerHandle AutoEndTimer;

    // Layer stack (top-most controls the camera)
    TArray<FName> CameraLayerStack;

    struct FCameraLayerData
    {
        TWeakObjectPtr<AActor> ViewTarget;
        TWeakObjectPtr<ACinematicCameraRigBase> Rig; // if ViewTarget is a rig
    };

    TMap<FName, FCameraLayerData> LayerData;

    void ApplyTopCamera(float BlendTime);
    APlayerController* ResolvePlayerController() const;

    void ClearAutoEndTimer();

    void ApplyCinematicPolicy();
    void RestoreCinematicPolicy();

    UPlayerCinematicComponent* ResolvePlayerCinematicComponent() const;
};
