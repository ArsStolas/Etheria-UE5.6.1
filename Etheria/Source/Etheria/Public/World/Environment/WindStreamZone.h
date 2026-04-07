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
class USplineMeshComponent;
class UCapsuleComponent;
class UStaticMesh;
class UMaterialInterface;
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
    float StreamSpeed = 6000.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="50", ClampMax="2000"))
    float StreamRadius = 300.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirectionInfluence = 0.9f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CrossingAssistStrength = 0.12f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="25.0"))
    float CenteringStrength = 8.f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="0.95"))
    float FreeMovementRadiusRatio = 0.55f;

    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="32"))
    int32 NumCollisionCapsules = 8;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UStaticMesh* StreamMesh;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual")
    UMaterialInterface* StreamMaterial;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="2", ClampMax="64"))
    int32 NumVisualSegments = 12;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
    float TubeOpacity = 0.35f;

    UPROPERTY(EditAnywhere, Category="WindStream|Visual", meta=(ClampMin="0.0", ClampMax="5.0"))
    float UVScrollSpeed = 1.2f;

    UPROPERTY(EditAnywhere, Category="WindStream|Debug")
    bool bWindDebugMode = false;

private:
    UPROPERTY()
    TSet<APlayerCharacter*> PlayersInStream;

    UPROPERTY()
    TArray<USplineMeshComponent*> SplineMeshes;

    UPROPERTY()
    TArray<UCapsuleComponent*> CollisionCapsules;

    float UVOffset = 0.f;

    void RebuildVisualTube();
    void RebuildCollisionCapsules();

    float GetClosestSplineDistance(const FVector& WorldPosition) const;
    float GetRadialFalloff(const FVector& WorldPosition, float SplineDistance) const;
    FVector GetPreferredStreamDirection(APlayerCharacter* Player, float SplineDistance) const;
    bool IsPlayerInDiveMode(APlayerCharacter* Player) const;
    UDiveMode* GetPlayerDiveMode(APlayerCharacter* Player) const;
    void ApplyWindEffect(APlayerCharacter* Player, float DeltaTime, float Falloff);

    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
