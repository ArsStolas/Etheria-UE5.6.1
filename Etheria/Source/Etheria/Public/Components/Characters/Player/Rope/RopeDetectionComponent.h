/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeDetectionComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeDetectionComponent.generated.h"

class APlayerCharacter;
class ARopeAttachPoint;
class UCameraComponent;
class UCharacterStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRopeDetectedPointChanged,
    ARopeAttachPoint*, NewPoint
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeDetectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URopeDetectionComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Rope")
    ARopeAttachPoint* GetCurrentDetectedPoint() const { return CurrentPoint.Get(); }

    UPROPERTY(BlueprintAssignable, Category="Rope|Detection")
    FOnRopeDetectedPointChanged OnDetectedPointChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === Detection ===
    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDetectionDistance = 1500.f;

    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="10", ClampMax="60"))
    float DetectionHalfAngle = 20.f;

    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="0.05", ClampMax="0.5"))
    float DetectionInterval = 0.08f;

    // === Validation ===
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinCameraDot = 0.75f;

    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="-500", ClampMax="500"))
    float MinHeightAbovePlayer = 0.f;
    
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0", ClampMax="1"))
    float ScoringDistanceWeight = 0.3f;
    
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0", ClampMax="1"))
    float ScoringDirectionWeight = 0.7f;

    // === Debug ===
    UPROPERTY(EditAnywhere, Category="Debug")
    bool bDebugMode = false;

    UPROPERTY(EditAnywhere, Category="Debug", meta=(EditCondition="bDebugMode"))
    int32 DebugVerbosity = 1;

private:
    // Cache
    APlayerCharacter* OwnerCharacter = nullptr;
    UCameraComponent* Camera = nullptr;
    UCharacterStateComponent* StateComponent = nullptr;

    FVector CachedCameraLocation = FVector::ZeroVector;
    FVector CachedCameraForward = FVector::ForwardVector;
    float CachedMaxDistSq = 0.f;
    float CachedMinCameraDot = 0.f;

    float TimeSinceLastScan = 0.f;
    TWeakObjectPtr<ARopeAttachPoint> CurrentPoint;

    // Statistiques optionnelles pour debug
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
    struct FDetectionStats
    {
        int32 TotalPointsChecked = 0;
        int32 FailedBroadPhase = 0;
        int32 FailedValidation = 0;
    } Stats;
#endif

    void DetectAttachPoint();
    bool IsValidPoint(ARopeAttachPoint* Point, FString& OutFailReason, const FVector& PlayerLoc) const;
    void UpdateCachedValues();
    void DrawDebugInfo(ARopeAttachPoint* BestPoint) const;
};
