/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
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
	void ConfigureGlideTuning(
		float InGlideSpeed,
		float InDescendRate,
		float InGlideInterp,
		float InDescentInterp,
		float InMinimumHeight,
		float InWindStreamAcceleration,
		float InWindEscapeInputThreshold,
		float InWindEscapeHoldTime);

	bool CanStartGliding() const;

	bool IsInWindZone() const;

	void ApplyWindBoost(float TargetSpeed, const FVector& WindDirection,
	                    float Influence, const FVector& CenteringAccel);

	float GetCurrentSpeed() const;

protected:
	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="500", ClampMax="2000",
		ToolTip="Vitesse horizontale maximale du planeur (cm/s). Détermine la rapidité du vol."))
	float GlideSpeed = 1200.f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="100", ClampMax="1000",
		ToolTip="Vitesse de descente progressive du planeur (cm/s). Plus haut = descend plus vite."))
	float DescendRate = 300.f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.5", ClampMax="10.0",
		ToolTip="Rapidité d'interpolation horizontale (vitesse de lissage). 1.0 = lent | 5.0 = très rapide | 2.0 = modéré."))
	float GlideInterp = 2.f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.5", ClampMax="10.0",
		ToolTip="Rapidité d'interpolation verticale (vitesse de lissage de la descente). 1.0 = lent | 5.0 = très rapide | 3.0 = modéré."))
	float DescentInterp = 3.f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="500", ClampMax="20000",
		ToolTip="Accélération utilisée quand un WindStreamZone pousse le planeur vers la vitesse du courant."))
	float WindStreamAcceleration = 2400.f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.1", ClampMax="1.0",
		ToolTip="Déflexion latérale minimale de l'input (perpendiculaire au stream) pour commencer à s'échapper d'un wind stream. En dessous, l'input latéral est ignoré."))
	float WindEscapeInputThreshold = 0.5f;

	UPROPERTY(EditAnywhere, Category="Glide|Movement", meta=(ClampMin="0.05", ClampMax="3.0",
		ToolTip="Durée d'appui latéral soutenu (secondes) avant d'obtenir la pleine autorité pour sortir du wind stream."))
	float WindEscapeHoldTime = 0.7f;

	UPROPERTY(EditAnywhere, Category="Glide|Validation", meta=(ClampMin="100", ClampMax="1000",
		ToolTip="Hauteur minimale pour activer le planeur (cm). Empêche d'activer le planeur trop proche du sol."))
	float MinimumHeight = 300.f;

private:
	void TickWindStream(float DeltaTime, const FVector& InputDir);
	void ResetWindStreamState();

	bool bWindStreamActive = false;
	float PendingWindTargetSpeed = 0.f;
	float PendingWindAlignmentStrength = 0.f;
	float WindRideSpeed = 0.f;
	float WindEscapeTime = 0.f;
	FVector PendingWindDirection = FVector::ZeroVector;
	FVector PendingWindCenteringAccel = FVector::ZeroVector;
};
