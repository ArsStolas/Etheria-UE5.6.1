/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: DiveMode - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "DiveMode.generated.h"

UCLASS()
class ETHERIA_API UDiveMode : public UFlightModeBase
{
	GENERATED_BODY()

public:
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void TickMode(float DeltaTime) override;

protected:
	// ===== VITESSE =====
    // Vitesse maximale en piqué (cm/s). Plus bas = moins d'accélération possible
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
    float MaxDiveSpeed = 2600.f;

    // Vitesse minimale en piqué (cm/s). Empêche de tomber trop lentement
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="100", ClampMax="1000"))
    float MinDiveSpeed = 300.f;

    // Accélération quand le nez pointe vers le bas (cm/s²). Plus haut = accélération plus rapide en piqué
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="500", ClampMax="2000"))
    float DiveAcceleration = 1400.f;

    // Décélération quand le nez pointe vers le haut (cm/s²). Plus haut = ralentit plus vite en montée
    UPROPERTY(EditAnywhere, Category="Dive|Speed", meta=(ClampMin="200", ClampMax="1500"))
    float DiveDeceleration = 800.f;

    // Vitesse actuelle en piqué (interne)
    float CurrentSpeed = 400.f;

    // ===== ROTATION =====
    // Angle de pitch maximum (degrés). Limite à quel point tu peux pencher vers le bas/haut
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="15", ClampMax="90"))
    float MaxPitch = 60.f;

    // Angle de roll maximum (degrés). Limite l'inclinaison latérale
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="10", ClampMax="90"))
    float MaxRoll = 30.f;

    // Vitesse de rotation en Yaw (degrés/s). Plus bas = virages plus larges et lents
    UPROPERTY(EditAnywhere, Category="Dive|Rotation", meta=(ClampMin="30", ClampMax="180"))
    float TurnRateDive = 90.f;

    // ===== PHYSIQUE =====
    // Facteur de portance (0-2). Détermine combien tu peux remonter en cabriolet
    // Plus haut = plus facile de remonter (0.6 = modéré, 1.0+ = très ascensionnel)
    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="0.1", ClampMax="2.0"))
    float LiftFactor = 0.6f;

    // Gravité appliquée en piqué (cm/s²). Contrôle à quel point tu "tombes" naturellement
    // Moins négatif (-100) = plane plus longtemps | Plus négatif (-500) = chute plus rapide
    // NOTE: Utilise directement la gravité en arcade du TickMode (400.f), pas cette valeur
    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="-50"))
    float DiveGravity = -100.f;

    // Décay autonome en glide (cm/s² par seconde). Vitesse de perte de vitesse sans input
    UPROPERTY(EditAnywhere, Category="Dive|Physics", meta=(ClampMin="-500", ClampMax="0"))
    float AutonomousGlideDecay = -200.f;
};
