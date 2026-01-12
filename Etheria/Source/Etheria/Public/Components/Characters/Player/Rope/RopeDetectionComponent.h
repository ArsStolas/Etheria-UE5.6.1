/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeDetectionComponent - Header
*/

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

#if UE_BUILD_SHIPPING
    #define DETECTION_LOG(Category, Verbosity, Format, ...)
    #define DETECTION_SCREEN_MSG(Key, Color, Format, ...)
    #define DEBUG_ONLY(x)
#else
    #define DETECTION_LOG(Category, Verbosity, Format, ...) \
        if (bDetectionDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
    #define DETECTION_SCREEN_MSG(Key, Color, Format, ...) \
        if (bDetectionDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
    #define DEBUG_ONLY(x) if (bDetectionDebugMode) { x; }
#endif

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

    /** Get the currently detected rope attachment point */
    UFUNCTION(BlueprintCallable, Category="Rope")
    ARopeAttachPoint* GetCurrentDetectedPoint() const { return CurrentPoint.Get(); }

    /** Broadcast when detected point changes */
    UPROPERTY(BlueprintAssignable, Category="Rope|Detection")
    FOnRopeDetectedPointChanged OnDetectedPointChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === Detection Parameters ===
    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDetectionDistance = 1500.f;

    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="10", ClampMax="60"))
    float DetectionHalfAngle = 20.f;

    UPROPERTY(EditAnywhere, Category="Detection", meta=(ClampMin="0.05", ClampMax="0.5"))
    float DetectionInterval = 0.08f;

    // === Validation Parameters ===
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinCameraDot = 0.75f;

    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="-500", ClampMax="500"))
    float MinHeightAbovePlayer = 0.f;
    
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0", ClampMax="1"))
    float ScoringDistanceWeight = 0.3f;
    
    UPROPERTY(EditAnywhere, Category="Detection|Validation", meta=(ClampMin="0", ClampMax="1"))
    float ScoringDirectionWeight = 0.7f;

    // === Debug Parameters ===
    UPROPERTY(EditAnywhere, Category="Rope|Detection|Debug")
    bool bDetectionDebugMode = false;

    UPROPERTY(EditAnywhere, Category="Rope|Detection|Debug", meta=(EditCondition="bDetectionDebugMode"))
    int32 DebugVerbosity = 0;

private:
    // Cached references
    UPROPERTY()
    APlayerCharacter* OwnerCharacter = nullptr;
    
    UPROPERTY()
    UCameraComponent* Camera = nullptr;
    
    UPROPERTY()
    UCharacterStateComponent* StateComponent = nullptr;

    // Cached values
    FVector CachedCameraLocation = FVector::ZeroVector;
    FVector CachedCameraForward = FVector::ForwardVector;
    float CachedMaxDistSq = 0.f;
    float CachedMinCameraDot = 0.f;

    float TimeSinceLastScan = 0.f;
    TWeakObjectPtr<ARopeAttachPoint> CurrentPoint;

    // === Debug Stats ===
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
};
