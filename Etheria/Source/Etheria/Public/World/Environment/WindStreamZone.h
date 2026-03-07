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

    // ─── Components ───────────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WindStream|Components")
    USplineComponent* Spline;

    // ─── Settings ─────────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="100", ClampMax="3000"))
    float BoostStrength = 800.f;

    // Rayon du tube — contrôle la taille des capsules ET la zone de boost
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="50", ClampMax="2000"))
    float StreamRadius = 300.f;

    // Combien le stream guide le Yaw du joueur vers la tangente (0=aucun, 1=fort)
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirectionInfluence = 0.4f;

    // Force de rappel vertical vers la hauteur de la spline (cm/s² par cm d'écart)
    // 2.0 = correction douce | 8.0 = correction très réactive
    // Le joueur peut toujours sortir verticalement en pitchant ou en ne pitchant plus
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="0.0", ClampMax="20.0"))
    float VerticalCorrectionStrength = 3.5f;

    // Nombre de capsules de collision le long de la spline
    UPROPERTY(EditAnywhere, Category="WindStream|Settings", meta=(ClampMin="2", ClampMax="32"))
    int32 NumCollisionCapsules = 8;

    // ─── Visual ───────────────────────────────────────────────────────────────
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

    // ─── Debug ────────────────────────────────────────────────────────────────
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

    float      GetClosestSplineAlpha(const FVector& WorldPosition) const;
    float      GetRadialFalloff(const FVector& WorldPosition) const;
    bool       IsPlayerInDiveMode(APlayerCharacter* Player) const;
    UDiveMode* GetPlayerDiveMode(APlayerCharacter* Player) const;
    void       ApplyWindEffect(APlayerCharacter* Player, float DeltaTime, float Falloff);

    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};