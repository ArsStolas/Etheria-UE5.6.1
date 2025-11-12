/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GliderComponent - Header
*/

// GliderComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GliderComponent.generated.h"

class APlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGliderEvent);

UENUM(BlueprintType)
enum class EGliderMode : uint8
{
    None     UMETA(DisplayName="Inactive"),
    Gliding  UMETA(DisplayName="Gliding"),
    Diving   UMETA(DisplayName="Diving")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UGliderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGliderComponent();

    void ToggleGliding();
    FORCEINLINE bool IsGliding() const { return CurrentMode == EGliderMode::Gliding; }

    void ToggleDiving();
    FORCEINLINE bool IsDiving() const { return CurrentMode == EGliderMode::Diving; }

    UPROPERTY(BlueprintAssignable)
    FGliderEvent OnGlideStart;

    UPROPERTY(BlueprintAssignable)
    FGliderEvent OnGlideStop;

    UPROPERTY(BlueprintAssignable)
    FGliderEvent OnDiveStart;

    UPROPERTY(BlueprintAssignable)
    FGliderEvent OnDiveStop;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glider|Settings")
    float MinimumHeight = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glider|Settings")
    float DescendingRate = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glider|Settings")
    float GlideSpeed = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glider|Settings")
    float GlideAccelInterp = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glider|Settings")
    float GlideDescentInterp = 3.f;

    // Dive Settings
    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float DiveAcceleration = 1200.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float MaxDiveSpeed = 2400.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float MinDiveSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float DiveDeceleration = 800.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float MaxPitchAngle = 45.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float MaxRollAngle = 30.f;

    // Vitesse de rotation en Yaw (plus bas = virages plus larges)
    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float TurnRateDive = 120.f;

    // Facteur de portance (plus haut = plus facile de remonter)
    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float LiftFactor = 0.5f;

    // Gravité appliquée en dive (plus bas = plane plus longtemps)
    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float DiveGravity = -100.f;

    UPROPERTY(EditAnywhere, Category="Dive|Settings")
    float AutonomousGlideDecay = -200.f;

private:
    void StartGliding();
    void StopGliding(bool bManualStop);
    void StartDiving();
    void StopDiving(bool bGoToGlide = false, bool bManualStop);

    bool CanStartGliding() const;
    bool IsGrounded() const;

    void RecordOriginalSettings();
    void ApplyOriginalSettings();
    
    void HandleDescent(float DeltaTime);
    void HandleDive(float DeltaTime);

    APlayerCharacter* OwnerCharacter = nullptr;

    float CurrentDiveSpeed = 1200.f;

    bool OriginalOrientRotation;
    float OriginalGravityScale;
    float OriginalWalkingSpeed;
    float OriginalDeceleration;
    float OriginalAcceleration;
    float OriginalAirControl;
    bool OriginalDesiredRotation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State", meta=(AllowPrivateAccess="true"))
    EGliderMode CurrentMode = EGliderMode::None;
};