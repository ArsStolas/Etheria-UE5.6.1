/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GlideMode - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "GlideMode.generated.h"

UCLASS()
class ETHERIA_API UGlideMode : public UFlightModeBase
{
	GENERATED_BODY()

public:
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void TickMode(float DeltaTime) override;

	bool CanStartGliding() const;

protected:
	// ===== VITESSE ET MOUVEMENT =====
	// Vitesse horizontale maximale du planeur (cm/s). Détermine la rapidité du vol
	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="500", ClampMax="2000"))
	float GlideSpeed = 1200.f;

	// Vitesse de descente progressive du planeur (cm/s). Plus haut = descend plus vite
	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="100", ClampMax="1000"))
	float DescendRate = 300.f;

	// Rapidité d'interpolation horizontale (vitesse de lissage). Plus haut = accélération plus rapide
	// 1.0 = lent | 5.0 = très rapide | 2.0 = modéré (actuel)
	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.5", ClampMax="10.0"))
	float GlideInterp = 2.f;

	// Rapidité d'interpolation verticale (vitesse de lissage de la descente). Plus haut = transition plus rapide
	// 1.0 = lent | 5.0 = très rapide | 3.0 = modéré (actuel)
	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.5", ClampMax="10.0"))
	float DescentInterp = 3.f;

	// ===== VALIDATION =====
	// Hauteur minimale pour activer le planeur (cm). Tu dois être au-dessus de cette hauteur
	// Empêche d'activer le planeur trop proche du sol
	UPROPERTY(EditAnywhere, Category="Glide|Validation", meta=(ClampMin="100", ClampMax="1000"))
	float MinimumHeight = 300.f;
};
