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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    FORCEINLINE bool IsAttached() const { return AttachedPoint.IsValid(); }
    FORCEINLINE ARopeAttachPoint* GetAttachedPoint() const { return AttachedPoint.Get(); }
    FORCEINLINE UCableComponent* GetCableComponent() const { return CableComponent; }
    FORCEINLINE float GetCableLengthOffset() const { return CableLengthOffset; }

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
    float CableWidth = 5.f;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    float CableLengthOffset = 10.f;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    UMaterialInterface* RopeMaterial;

    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    USkeletalMesh* RopeMesh;
    
    UPROPERTY(EditDefaultsOnly, Category="Rope")
    FName RopeStartSocketName = TEXT("hand_r");

private:
    UFUNCTION()
    void OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint);
    
    UPROPERTY(EditAnywhere, Category="Rope|Attach|Debug")
    bool bAttachDebugMode = false;
    
    // Visual smoothing
    UPROPERTY(EditAnywhere, Category="Rope|Visual")
    float CableLengthInterpSpeed = 10.f;
};
