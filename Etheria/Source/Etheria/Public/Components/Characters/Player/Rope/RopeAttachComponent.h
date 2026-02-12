/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeAttachComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeAttachComponent.generated.h"

#if UE_BUILD_SHIPPING
    #define ROPE_LOG(Category, Verbosity, Format, ...)
    #define ROPE_SCREEN_MSG(Key, Color, Format, ...)
#else
    #define ROPE_LOG(Category, Verbosity, Format, ...) \
    if (bAttachDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
    #define ROPE_SCREEN_MSG(Key, Color, Format, ...) \
    if (bAttachDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class APlayerCharacter;
class ARopeAttachPoint;
class URopeLockComponent;
class UCableComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeAttachComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URopeAttachComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    FORCEINLINE bool IsAttached() const { return AttachedPoint.IsValid(); }
    FORCEINLINE ARopeAttachPoint* GetAttachedPoint() const { return AttachedPoint.Get(); }
    FORCEINLINE UCableComponent* GetCableComponent() const { return CableComponent; }
    FORCEINLINE float GetCableLengthOffset() const { return RopeLengthOffset; }
    FORCEINLINE float GetMinRopeLength() const { return MinRopeLength; }
    FORCEINLINE float GetMaxRopeLength() const { return MaxRopeLength; }

    /** Attach the rope to a target point */
    void AttachRope(ARopeAttachPoint* TargetPoint);

    /** Detach the rope */
    void DetachRope();

    /** Smoothly update the visual length of the cable */
    UFUNCTION(BlueprintCallable, Category="Rope|Visual")
    void UpdateVisualCableLength(float TargetLength, float DeltaTime);

    /** Change the rope mesh or material */
    UFUNCTION(BlueprintCallable, Category="Rope|Visual")
    void SetRopeMesh(USkeletalMesh* NewMesh);

    UFUNCTION(BlueprintCallable, Category="Rope|Visual")
    void SetRopeMaterial(UMaterialInterface* NewMaterial);
    
    float GetCurrentRopeLength() const;

protected:
    APlayerCharacter* OwnerCharacter = nullptr;
    URopeLockComponent* LockComponent = nullptr;
    TWeakObjectPtr<ARopeAttachPoint> AttachedPoint;

    UPROPERTY(VisibleAnywhere, Category="Rope|Visual")
    UCableComponent* CableComponent = nullptr;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    float RopeWidth = 5.f;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    float RopeLengthOffset = 10.f;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    UMaterialInterface* RopeMaterial;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    USkeletalMesh* RopeMesh;
    
    /** Vertical offset for the rope attachment point on the anchor (positive = higher) */
    UPROPERTY(EditAnywhere, Category="Rope|Attachment")
    float AnchorAttachmentOffset = 25.f;
    
    // Min/max width for cable visual
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope|Visual")
    float MinRopeWidth = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope|Visual")
    float MaxRopeWidth = 6.f;

    // A multiplier to exaggerate tension
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope|Visual")
    float TensionWidthMultiplier = 1.f;
    
    UPROPERTY(EditAnywhere, Category="Rope|Climb")
    float MinRopeLength = 500.f;

    UPROPERTY(EditAnywhere, Category="Rope|Climb")
    float MaxRopeLength = 1600.f;

    // ===== CABLE PHYSICS SETTINGS =====
    
    /** Number of segments for cable simulation (more = smoother but more expensive) 
     * WARNING: Values above 10 may cause crashes in CableComponent
     * Recommended: 8-10 for best stability/quality balance */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="5", ClampMax="10"))
    int32 NumSegments = 10;
    
    /** Substep time for physics simulation (lower = more stable) */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="0.01", ClampMax="0.1"))
    float SubstepTime = 0.02f;
    
    /** Solver iterations (higher = less stretchy but more expensive) */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="1", ClampMax="10"))
    int32 SolverIterations = 2;
    
    /** Damping force applied to cable to reduce bouncing (higher = less bouncy) */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="0", ClampMax="100"))
    FVector CableForce = FVector(0.f, 0.f, -50.f);
    
    /** Cable gravity scale (1.0 = normal gravity, adjust for rope weight feel) */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="0", ClampMax="3"))
    float CableGravityScale = 1.0f;
    
    /** Enable collision for cable with world geometry */
    UPROPERTY(EditAnywhere, Category="Rope|Physics")
    bool bEnableCollision = true; // ACTIVÉ par défaut maintenant
    
    /** Collision radius for each cable segment */
    UPROPERTY(EditAnywhere, Category="Rope|Physics", meta=(ClampMin="1", ClampMax="20"))
    float CableCollisionRadius = 5.f;

private:
    UFUNCTION()
    void OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint);
    
    /** Target length we're interpolating towards */
    float TargetCableLength = 0.f;
    
    UPROPERTY(EditAnywhere, Category="Rope|Attach|Debug")
    bool bAttachDebugMode = false;
    
    // Visual smoothing
    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    float CableLengthInterpSpeed = 10.f;
};