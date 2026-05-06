/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: WindStreamZone - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindStreamZone.generated.h"

class USplineComponent;
class UCapsuleComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class APlayerCharacter;
class UDiveMode;

UCLASS()
class ETHERIA_API AWindStreamZone : public AActor
{
    GENERATED_BODY()

public:
    AWindStreamZone();
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WindStream|Components")
    USplineComponent* Spline;

    // Maximum target speed reached after staying in the stream long enough.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="500", ClampMax="12000"))
    float StreamSpeed = 4500.f;

    // Time in seconds needed to ramp from entry speed to StreamSpeed.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.1", ClampMax="5.0"))
    float StreamSpeedRampTime = 1.35f;

    // Percentage of StreamSpeed used when entering or crossing the stream.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float StreamEntrySpeedRatio = 0.42f;

    // Gameplay radius of the tunnel; player must stay inside this radius.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="50", ClampMax="2000"))
    float StreamRadius = 300.f;

    // How much the stream can align the player velocity toward the spline.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirectionInfluence = 0.9f;

    // Small boost/assist when the player crosses the stream instead of using it.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CrossingAssistStrength = 0.02f;

    // Pull toward the center line when the player is using the stream.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="25.0"))
    float CenteringStrength = 9.5f;

    // Extra centering/steering grip in curved sections.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="3.0"))
    float CurveGripBoost = 0.85f;

    // Speed reduction applied in strong curves to help the player stay inside.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.6"))
    float CurveSpeedReduction = 0.22f;

    // Distance used to detect how sharp the upcoming/previous curve is.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="100", ClampMax="2500"))
    float CurveLookAheadDistance = 650.f;

    // Inner area where the player can move freely before edge centering increases.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.95"))
    float FreeMovementRadiusRatio = 0.55f;

    // Minimum amount of trigger capsules placed along the spline.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128"))
    int32 NumCollisionCapsules = 16;

    // Hard cap for auto-generated trigger capsules on long streams.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128"))
    int32 MaxGeneratedCollisionCapsules = 48;

    // Capsule half-height multiplier; higher values overlap capsules more.
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.5", ClampMax="1.5"))
    float CollisionCapsuleOverlap = 0.8f;

    // Main wind Niagara repeated in segments along the spline.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* StreamNiagaraSystem;

    // Optional soft boundary Niagara repeated in segments along the spline.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* BoundaryNiagaraSystem;

    // Optional checkpoint/ring Niagara placed along the stream boundary.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* BoundaryRingNiagaraSystem;

    // Number of Niagara segments for the main wind visual.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="2", ClampMax="64"))
    int32 NumVisualSegments = 12;

    // Number of boundary rings when not placing rings directly on spline points.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0", ClampMax="64"))
    int32 NumBoundaryRings = 14;

    // Visual width multiplier for the main wind Niagara box.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="2.0"))
    float StreamVisualWidthMultiplier = 1.f;

    // Visual length multiplier for each main wind Niagara segment.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="1.25"))
    float StreamVisualLengthMultiplier = 0.95f;

    // Uniform scale applied to generated Niagara components.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.01", ClampMax="10.0"))
    float StreamNiagaraComponentScale = 1.f;

    // Spawn rate forwarded to User.Wind_Spawn Rate on the main Niagara.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0"))
    float StreamWindSpawnRate = 5.f;

    // Spawn rate forwarded to User.Leaves_Spawn Rate on the main Niagara.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0"))
    float StreamLeavesSpawnRate = 8.f;

    // Radius multiplier for the optional boundary Niagara.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0"))
    float BoundaryRadiusMultiplier = 1.02f;

    // Radius multiplier for checkpoint/ring Niagara.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0"))
    float BoundaryRingRadiusMultiplier = 1.05f;

    // If true, rings are placed at spline points instead of evenly spaced.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    bool bPlaceBoundaryRingsAtSplinePoints = false;

    // If using spline-point rings, also place rings at the first and last points.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    bool bIncludeBoundaryRingAtStreamEnds = true;

    // Extra visual scale for ring Niagara without changing gameplay radius.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="5.0"))
    float BoundaryRingNiagaraScaleMultiplier = 1.f;

    // Offset to align your ring asset; default pitch fixes rings authored facing upward.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    FRotator BoundaryRingRotationOffset = FRotator(90.f, 0.f, 0.f);

    // Opacity/intensity forwarded to boundary and ring Niagara systems.
    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BoundaryOpacity = 0.55f;

    // Draw gameplay/debug helpers and enable WindStream logs.
    UPROPERTY(EditAnywhere, Category="WindStream|Debug")
    bool bWindDebugMode = false;

    // Number of samples used to draw debug rings along the spline.
    UPROPERTY(EditAnywhere, Category="WindStream|Debug", meta=(ClampMin="4", ClampMax="64"))
    int32 DebugSplineSamples = 24;

private:
    UPROPERTY()
    TSet<APlayerCharacter*> PlayersInStream;

    TMap<TWeakObjectPtr<APlayerCharacter>, float> PlayerWindUseTimes;
    TMap<TWeakObjectPtr<APlayerCharacter>, float> PlayerSplineDistances;
    TMap<TWeakObjectPtr<APlayerCharacter>, int32> PlayerStreamDirectionSigns;

    UPROPERTY()
    TArray<UNiagaraComponent*> StreamNiagaraComponents;

    UPROPERTY()
    TArray<UNiagaraComponent*> BoundaryNiagaraComponents;

    UPROPERTY()
    TArray<UNiagaraComponent*> BoundaryRingNiagaraComponents;

    UPROPERTY()
    TArray<UCapsuleComponent*> CollisionCapsules;

    void RebuildVisuals();
    void DestroyVisualComponents();
    void ConfigureNiagaraComponent(UNiagaraComponent* NiagaraComponent, int32 SegmentIndex,
        float DistanceStart, float DistanceEnd, bool bBoundary) const;
    void ConfigureBoundaryRingComponent(UNiagaraComponent* NiagaraComponent, int32 RingIndex, float Distance) const;
    void RebuildCollisionCapsules();
    void DrawWindDebug() const;

    float GetClosestSplineDistance(const FVector& WorldPosition) const;
    float GetRadialFalloff(const FVector& WorldPosition, float SplineDistance) const;
    float GetCurveStrength(float SplineDistance) const;
    float GetTrackedSplineDistance(APlayerCharacter* Player, const FVector& WorldPosition, float DeltaTime);
    FVector GetPreferredStreamDirection(APlayerCharacter* Player, float SplineDistance);
    bool IsPlayerInDiveMode(APlayerCharacter* Player) const;
    UDiveMode* GetPlayerDiveMode(APlayerCharacter* Player) const;
    void ApplyWindEffect(APlayerCharacter* Player, float DeltaTime);

    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
