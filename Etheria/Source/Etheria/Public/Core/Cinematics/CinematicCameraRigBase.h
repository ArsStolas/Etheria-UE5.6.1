/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ACinematicCameraRigBase" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CinematicCameraRigBase.generated.h"

class UCineCameraComponent;

UENUM(BlueprintType)
enum class ECameraRigFollowMode : uint8
{
    Attach      UMETA(DisplayName="Attach"),
    Interp      UMETA(DisplayName="Interp")
};

/**
 * Minimal dynamic camera rig:
 * - Can follow a target actor (attach or interpolated)
 * - Optional look-at with rotation smoothing
 * Meant to be subclassed in BP for shakes, offsets, extra logic.
 */
UCLASS(BlueprintType)
class ETHERIA_API ACinematicCameraRigBase : public AActor
{
    GENERATED_BODY()

public:
    ACinematicCameraRigBase();

    UFUNCTION(BlueprintCallable, Category="Cinematic|Rig")
    void StartRig();

    UFUNCTION(BlueprintCallable, Category="Cinematic|Rig")
    void StopRig();

    UFUNCTION(BlueprintCallable, Category="Cinematic|Rig")
    void SetTargetActor(AActor* InTarget);

    UFUNCTION(BlueprintPure, Category="Cinematic|Rig")
    AActor* GetTargetActor() const { return TargetActor.Get(); }

    UFUNCTION(BlueprintPure, Category="Cinematic|Rig")
    UCineCameraComponent* GetCineCamera() const { return CineCamera; }

    UFUNCTION(BlueprintCallable, Category="Cinematic|Rig")
    void SetRelativeOffset(const FVector& Offset) { RelativeOffset = Offset; }

    UFUNCTION(BlueprintCallable, Category="Cinematic|Rig")
    void SetLookAtEnabled(bool bEnabled) { bLookAtTarget = bEnabled; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Cinematic|Rig")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category="Cinematic|Rig")
    TObjectPtr<UCineCameraComponent> CineCamera;

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    bool bRunning = false;

    UPROPERTY(EditAnywhere, Category="Cinematic|Rig")
    ECameraRigFollowMode FollowMode = ECameraRigFollowMode::Interp;

    UPROPERTY(EditAnywhere, Category="Cinematic|Rig")
    FVector RelativeOffset = FVector(-350.f, 0.f, 220.f);

    UPROPERTY(EditAnywhere, Category="Cinematic|Rig")
    bool bLookAtTarget = true;

    UPROPERTY(EditAnywhere, Category="Cinematic|Rig", meta=(ClampMin="0.0"))
    float FollowInterpSpeed = 7.5f;

    UPROPERTY(EditAnywhere, Category="Cinematic|Rig", meta=(ClampMin="0.0"))
    float RotationInterpSpeed = 10.0f;

    void UpdateFollow(float DeltaSeconds);
    void UpdateLookAt(float DeltaSeconds);
};
