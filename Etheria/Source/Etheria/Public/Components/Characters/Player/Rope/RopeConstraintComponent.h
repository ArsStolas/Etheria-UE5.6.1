/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeConstraintComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeConstraintComponent.generated.h"

#if UE_BUILD_SHIPPING
    #define CONSTRAINT_LOG(Category, Verbosity, Format, ...)
    #define CONSTRAINT_SCREEN_MSG(Key, Color, Format, ...)
#else
    #define CONSTRAINT_LOG(Category, Verbosity, Format, ...) \
    if (bConstraintDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
    #define CONSTRAINT_SCREEN_MSG(Key, Color, Format, ...) \
    if (bConstraintDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class APlayerCharacter;
class ARopeAttachPoint;
class UCharacterMovementComponent;
class URopeAttachComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRopeTensioned);

/**
 * Manages a rope constraint for the player character when attached to a rope anchor point
 * Applies physics constraints to simulate rope tension
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeConstraintComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URopeConstraintComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    FORCEINLINE bool IsActive() const { return bIsActive; }
    FORCEINLINE ARopeAttachPoint* GetAnchor() const { return Anchor.Get(); }
    FORCEINLINE float GetRopeLength() const { return RopeLength; }
    FORCEINLINE float GetCurrentPullRopeLength() const { return CurrentPullRopeLength; }
    void SetCurrentPullRopeLength(float NewLength) { CurrentPullRopeLength = FMath::Max(0.f, NewLength); }

    /** Reset le timer de grâce physique — appeler à chaque nouveau pull input pour éviter
     *  que la latence de réponse physique soit confondue avec un blocage réel */
    void ResetPullGraceTimer() { PullForceGraceTimer = 0.f; bIsObjectBlocked = false; BlockedAccumulator = 0.f; UnblockedAccumulator = 0.f; }

    void ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength);

    void DeactivateConstraint();

    void SetRopeLength(float NewLength);

    UPROPERTY(BlueprintAssignable)
    FOnRopeTensioned OnRopeTensioned;

private:
    void ApplyConstraint(float DeltaTime);
    
    void HandlePullConstraint(float DeltaTime);
    void HandleSwingConstraint(float DeltaTime);
    void ResetPullState();

    UPROPERTY()
    APlayerCharacter* OwnerCharacter = nullptr;
    
    UPROPERTY()
    UCharacterMovementComponent* MoveComp = nullptr;
    
    UPROPERTY()
    URopeAttachComponent* AttachComponent = nullptr;

    TWeakObjectPtr<ARopeAttachPoint> Anchor;

    float RopeLength = 0.f;
    bool bIsActive = false;
    
    // Tracking Pull
    float CurrentPullRopeLength = 0.f;
    float LastPlayerObjectDistance = 0.f;
    FVector LastObjectLocation = FVector::ZeroVector;

    // Détection blocage robuste
    float BlockedAccumulator = 0.f;
    float BlockedConfirmDelay = 0.15f;
    float UnblockedAccumulator = 0.f;
    float UnblockedConfirmDelay = 0.1f;
    bool bIsObjectBlocked = false;

    // Timer de grâce pour la latence physique :
    // Quand une force est appliquée à l'objet, le moteur physique met quelques frames à le faire bouger.
    // Sans ce timer, la détection de blocage voit l'objet immobile pendant ces frames
    // et pense qu'il est bloqué → contraint le joueur à tort au lieu de l'objet.
    // Ce timer ignore la détection pendant PullForceGraceDuration secondes après le début du pull.
    float PullForceGraceTimer = 0.f;

    /** Durée (secondes) pendant laquelle on ignore la détection de blocage après application d'une force.
     *  Doit être légèrement supérieure au temps de réponse physique (généralement 2-3 frames à 60fps ≈ 0.05s).
     *  Augmenter si le joueur se rapproche encore trop tôt sur des objets lourds. */
    UPROPERTY(EditAnywhere, Category="Rope|Constraint|Pull", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PullForceGraceDuration = 0.25f;

    UPROPERTY(EditAnywhere, Category="Rope|Constraint|Pull|Debug")
    float BlockDetectionSensitivity = 3.f;
    
    /** Smoothing for constraint application (prevents jitter) */
    UPROPERTY(EditAnywhere, Category="Rope|Constraint|Debug")
    float ConstraintSmoothness = 0.8f;
    
    UPROPERTY(EditAnywhere, Category="Rope|Constraint|Debug")
    bool bConstraintDebugMode = false;
};
