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

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="500", ClampMax="12000"))
    float StreamSpeed = 4500.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.1", ClampMax="5.0"))
    float StreamSpeedRampTime = 1.35f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float StreamEntrySpeedRatio = 0.42f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="50", ClampMax="2000"))
    float StreamRadius = 300.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirectionInfluence = 0.9f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CrossingAssistStrength = 0.02f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="25.0"))
    float CenteringStrength = 9.5f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="3.0"))
    float CurveGripBoost = 0.85f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.6"))
    float CurveSpeedReduction = 0.22f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="100", ClampMax="2500"))
    float CurveLookAheadDistance = 650.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.95"))
    float FreeMovementRadiusRatio = 0.55f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128"))
    int32 NumCollisionCapsules = 16;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="128"))
    int32 MaxGeneratedCollisionCapsules = 48;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.5", ClampMax="1.5"))
    float CollisionCapsuleOverlap = 0.8f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* StreamNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* BoundaryNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UNiagaraSystem* BoundaryRingNiagaraSystem;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="2", ClampMax="64"))
    int32 NumVisualSegments = 12;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0", ClampMax="64"))
    int32 NumBoundaryRings = 6;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="2.0"))
    float StreamVisualWidthMultiplier = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="1.25"))
    float StreamVisualLengthMultiplier = 0.95f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.01", ClampMax="10.0"))
    float StreamNiagaraComponentScale = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0"))
    float StreamWindSpawnRate = 5.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="200.0"))
    float StreamLeavesSpawnRate = 8.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0"))
    float BoundaryRadiusMultiplier = 1.02f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.5", ClampMax="2.0"))
    float BoundaryRingRadiusMultiplier = 1.05f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    bool bPlaceBoundaryRingsAtSplinePoints = true;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    bool bIncludeBoundaryRingAtStreamEnds = true;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.1", ClampMax="5.0"))
    float BoundaryRingNiagaraScaleMultiplier = 1.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    FRotator BoundaryRingRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BoundaryOpacity = 0.55f;

    UPROPERTY(EditAnywhere, Category="WindStream|Debug")
    bool bWindDebugMode = false;

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
