/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeSwingComponent - Source
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeSwingComponent.generated.h"

#if UE_BUILD_SHIPPING
    #define SWING_LOG(Category, Verbosity, Format, ...)
    #define SWING_DEBUG_LINE(World, Start, End, Color)
    #define SWING_SCREEN_MSG(Key, Color, Format, ...)
#else
    #define SWING_LOG(Category, Verbosity, Format, ...) \
    if (bSwingDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
    #define SWING_DEBUG_LINE(World, Start, End, Color) \
    if (bSwingDebugMode) DrawDebugLine(World, Start, End, Color, false, -1.f, 0, 2.f)
    #define SWING_SCREEN_MSG(Key, Color, Format, ...) \
    if (bSwingDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class URopeConstraintComponent;
class APlayerCharacter;
class URopeAttachComponent;
class URopeLockComponent;
class ARopeAttachPoint;
class UCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeSwingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URopeSwingComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void StartSwing();
    void StopSwing();
    
    // SLACK DYNAMIQUE
    void SetBaseRopeLength(float NewLength);
    
    FORCEINLINE void SetClimbActive(bool bActive) { bClimbInputActive = bActive; }

    FORCEINLINE bool IsSwinging() const { return bIsSwinging; }
    
    /** Récupère la longueur actuelle utilisée par le swing */
    FORCEINLINE float GetSwingRopeLength() const { return RopeLength; }

protected:
    void UpdateSwing(float DeltaTime);
    void UpdateDynamicSlack(float DeltaTime, const FVector& RopeDir);
    
    // FORCES APPLICATION
    void ApplyGravity(FVector& CurrentVelocity, float DeltaTime);
    void ApplyAirResistance(FVector& CurrentVelocity, float DeltaTime);
    void ApplyPlayerInputForce(FVector& CurrentVelocity, const FVector& RopeDirection, float DeltaTime);
    void SolveRopeConstraint(FVector& CurrentPosition, FVector& CurrentVelocity, const FVector& AnchorLocation, float DeltaTime);

    // PUMP CALCULATION
    bool TryConsumePump(const FVector& CurrentVelocity, const FVector& TangentDir);
    
    // CONSTRAINT SOLVER
    bool ShouldStartSwing() const;
    
    // SWING CONDITIONS
    bool IsFarEnoughFromGround() const;
    bool HasTouchedGround() const;
    FVector GetCameraInputDirection() const;

    UFUNCTION()
    void OnRopeTensioned();

private:
    UPROPERTY()
    APlayerCharacter* OwnerCharacter = nullptr;
    UPROPERTY()
    UCharacterMovementComponent* MoveComp = nullptr;
    UPROPERTY()
    URopeAttachComponent* AttachComponent = nullptr;
    UPROPERTY()
    URopeLockComponent* LockComponent = nullptr;
    UPROPERTY()
    URopeConstraintComponent* ConstraintComponent = nullptr;
    
    TWeakObjectPtr<ARopeAttachPoint> SwingPoint;

    bool bIsSwinging = false;
    bool bFirstFrame = true;
    float RopeLength = 0.f;
    float MaxVelocityReached = 0.f;
    FVector SwingVelocity = FVector::ZeroVector; 

    /* ===== TUNING ===== */
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float GravityScale = 2.f;
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float AirDrag = 0.3f; // Plus c'est haut, plus ça freine vite
    UPROPERTY(EditAnywhere, Category="Swing|Control")
    float SwingForce = 600.f;
    UPROPERTY(EditAnywhere, Category="Swing|Control")
    float MaxSwingVelocity = 1400.f;

    /* ===== PUMP TIMING ===== */

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpBonusMultiplier = 1.35f;

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpVerticalSpeedThreshold = 80.f; // |Z| proche du bas

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpMinSpeed = 450.f; // vitesse mini pour autoriser pump

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpCooldown = 0.35f;
    
    float LastVerticalSpeed = 0.f;
    bool bPumpConsumedThisSwing = false;

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpImpulseStrength = 280.f;

    UPROPERTY(EditAnywhere, Category="Swing|Pump")
    float PumpResetVerticalSpeed = 120.f; // quand on remonte assez

    float LastPumpTime = -1000.f;
    bool bPumpActive = false;
    
    /* ===== SLACK DYNAMIQUE ===== */
    
    // Physical length of the rope without slack
    float BaseRopeLength;

    // Effective length considering slack
    float EffectiveRopeLength;

    UPROPERTY(EditAnywhere, Category="Swing|Slack")
    float MaxSlackLength = 120.f;

    UPROPERTY(EditAnywhere, Category="Swing|Slack")
    float SlackInterpSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category="Swing|Slack")
    float SlackVerticalSpeedThreshold = 50.f;
    
    UPROPERTY(EditAnywhere, Category="Swing|Slack")
    float SlackReleaseSpeed = 10.f; // retension speed
    
    bool bClimbInputActive = false;
    
    // Paramètres Anti-Choc
    UPROPERTY()
    FVector InitialSwingLocation;
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float ConstraintStiffness = 20.f; // Vitesse de rappel (plus c'est haut, plus c'est sec)

    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float FallingSpeedToStartSwing = 100.f;
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float MinHeightAboveGround = 80.f;
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float GroundStopDistance = 30.f;
    
    /* Debug */
    UPROPERTY(EditAnywhere, Category="Rope|Swing|Debug")
    bool bSwingDebugMode = true;
    
    void DrawVisualDebug(const FVector& Anchor, const FVector& PlayerPos, const FVector& InputDir);
};
