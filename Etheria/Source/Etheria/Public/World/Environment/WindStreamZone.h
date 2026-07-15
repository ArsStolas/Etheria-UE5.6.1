/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
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
class UGlideMode;

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

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="500", ClampMax="12000",
        ToolTip="Maximum target speed reached after staying in the stream long enough."))
    float StreamSpeed = 4500.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.1", ClampMax="1.0",
        ToolTip="Multiplier applied to StreamSpeed while the player rides the stream with the glider."))
    float GliderStreamSpeedMultiplier = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.1", ClampMax="5.0",
        ToolTip="Time in seconds needed to ramp from entry speed to StreamSpeed."))
    float StreamSpeedRampTime = 1.35f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0",
        ToolTip="Percentage of StreamSpeed used when entering or crossing the stream."))
    float StreamEntrySpeedRatio = 0.42f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="50", ClampMax="2000",
        ToolTip="Gameplay radius of the tunnel; player must stay inside this radius."))
    float StreamRadius = 300.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0",
        ToolTip="How much the stream can align the player velocity toward the spline."))
    float DirectionInfluence = 0.9f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0",
        ToolTip="Small boost/assist when the player crosses the stream instead of using it."))
    float CrossingAssistStrength = 0.02f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="25.0",
        ToolTip="Pull toward the center line when the player is using the stream."))
    float CenteringStrength = 9.5f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="3.0",
        ToolTip="Extra centering/steering grip in curved sections."))
    float CurveGripBoost = 0.85f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.6",
        ToolTip="Speed reduction applied in strong curves to help the player stay inside."))
    float CurveSpeedReduction = 0.22f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="100", ClampMax="2500",
        ToolTip="Distance used to detect how sharp the upcoming/previous curve is."))
    float CurveLookAheadDistance = 650.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.95",
        ToolTip="Inner area where the player can move freely before edge centering increases."))
    float FreeMovementRadiusRatio = 0.55f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128",
        ToolTip="Minimum amount of trigger capsules placed along the spline."))
    int32 NumCollisionCapsules = 16;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128",
        ToolTip="Hard cap for auto-generated trigger capsules on long streams."))
    int32 MaxGeneratedCollisionCapsules = 48;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.5", ClampMax="1.5",
        ToolTip="Capsule half-height multiplier; higher values overlap capsules more."))
    float CollisionCapsuleOverlap = 0.8f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="Main wind Niagara repeated in segments along the spline."))
    UNiagaraSystem* StreamNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="Optional soft boundary Niagara repeated in segments along the spline."))
    UNiagaraSystem* BoundaryNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="Optional checkpoint/ring Niagara placed along the stream boundary."))
    UNiagaraSystem* BoundaryRingNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="2", ClampMax="64",
        ToolTip="Number of Niagara segments for the main wind visual."))
    int32 NumVisualSegments = 12;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0", ClampMax="64",
        ToolTip="Number of boundary rings when not placing rings directly on spline points."))
    int32 NumBoundaryRings = 14;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="2.0",
        ToolTip="Visual width multiplier for the main wind Niagara box."))
    float StreamVisualWidthMultiplier = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="1.25",
        ToolTip="Visual length multiplier for each main wind Niagara segment."))
    float StreamVisualLengthMultiplier = 0.95f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.01", ClampMax="10.0",
        ToolTip="Uniform scale applied to generated Niagara components."))
    float StreamNiagaraComponentScale = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0",
        ToolTip="Spawn rate forwarded to User.Wind_Spawn Rate on the main Niagara."))
    float StreamWindSpawnRate = 5.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0",
        ToolTip="Spawn rate forwarded to User.Leaves_Spawn Rate on the main Niagara."))
    float StreamLeavesSpawnRate = 8.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0",
        ToolTip="Radius multiplier for the optional boundary Niagara."))
    float BoundaryRadiusMultiplier = 1.02f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0",
        ToolTip="Radius multiplier for checkpoint/ring Niagara."))
    float BoundaryRingRadiusMultiplier = 1.05f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="If true, rings are placed at spline points instead of evenly spaced."))
    bool bPlaceBoundaryRingsAtSplinePoints = false;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="If using spline-point rings, also place rings at the first and last points."))
    bool bIncludeBoundaryRingAtStreamEnds = true;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="5.0",
        ToolTip="Extra visual scale for ring Niagara without changing gameplay radius."))
    float BoundaryRingNiagaraScaleMultiplier = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual",
        meta=(ToolTip="Offset to align your ring asset; default pitch fixes rings authored facing upward."))
    FRotator BoundaryRingRotationOffset = FRotator(90.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="1.0",
        ToolTip="Opacity/intensity forwarded to boundary and ring Niagara systems."))
    float BoundaryOpacity = 0.55f;

    UPROPERTY(EditAnywhere, Category="WindStream|Debug",
        meta=(ToolTip="Draw gameplay/debug helpers and enable WindStream logs."))
    bool bWindDebugMode = false;

    UPROPERTY(EditAnywhere, Category="WindStream|Debug", meta=(ClampMin="4", ClampMax="64",
        ToolTip="Number of samples used to draw debug rings along the spline."))
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
    bool IsPlayerInStreamFlightMode(APlayerCharacter* Player) const;
    UDiveMode* GetPlayerDiveMode(APlayerCharacter* Player) const;
    UGlideMode* GetPlayerGlideMode(APlayerCharacter* Player) const;
    void ApplyWindEffect(APlayerCharacter* Player, float DeltaTime);

    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
