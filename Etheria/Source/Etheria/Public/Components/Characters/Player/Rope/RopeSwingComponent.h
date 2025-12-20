#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeSwingComponent.generated.h"

#if UE_BUILD_SHIPPING
    #define SWING_LOG(Category, Verbosity, Format, ...)
    #define SWING_DEBUG_LINE(World, Start, End, Color)
    #define SWING_SCREEN_MSG(Key, Color, Format, ...)
#else
    #define SWING_LOG(Category, Verbosity, Format, ...) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
    #define SWING_DEBUG_LINE(World, Start, End, Color) DrawDebugLine(World, Start, End, Color, false, -1.f, 0, 2.f)
    #define SWING_SCREEN_MSG(Key, Color, Format, ...) if (GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
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

    FORCEINLINE bool IsSwinging() const { return bIsSwinging; }

protected:
    void UpdateSwing(float DeltaTime);
    
    // Physique
    void ApplyGravity(FVector& CurrentVelocity, float DeltaTime);
    void ApplyAirResistance(FVector& CurrentVelocity, float DeltaTime);
    void ApplyPlayerInputForce(FVector& CurrentVelocity, const FVector& RopeDirection, float DeltaTime);
    void SolveRopeConstraint(FVector& CurrentPosition, FVector& CurrentVelocity, const FVector& AnchorLocation, float DeltaTime);

    // Helpers
    bool ShouldStartSwing() const;
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
    float SwingForce = 550.f;
    UPROPERTY(EditAnywhere, Category="Swing|Control")
    float MaxSwingVelocity = 1400.f;

    // Paramètres Anti-Choc (pour la "téléportation")
    UPROPERTY()
    FVector InitialSwingLocation;
    UPROPERTY(EditAnywhere, Category="Swing|Physics")
    float ConstraintStiffness = 20.f; // Vitesse de rappel (plus c'est haut, plus c'est sec)

    UPROPERTY(EditAnywhere, Category="Swing")
    float FallingSpeedToStartSwing = 100.f;
    UPROPERTY(EditAnywhere, Category="Swing")
    float MinHeightAboveGround = 80.f;
    UPROPERTY(EditAnywhere, Category="Swing")
    float GroundStopDistance = 30.f;

    /* Debug */
    UPROPERTY(EditAnywhere, Category="Debug") bool bShowDebug = true;
    void DrawVisualDebug(const FVector& Anchor, const FVector& PlayerPos, const FVector& InputDir);
};
