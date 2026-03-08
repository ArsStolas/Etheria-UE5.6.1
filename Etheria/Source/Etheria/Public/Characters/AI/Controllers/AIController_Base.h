/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIController_Base - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AIController_Base.generated.h"

class UNavigationPath;

UCLASS()
class ETHERIA_API AAIController_Base : public AAIController
{
	GENERATED_BODY()

public:
	AAIController_Base();

	/** Demande un déplacement vers une position (pathfinding + AddMovementInput, accélération respectée). Utilisé par patrol et wander. */
	UFUNCTION(BlueprintCallable, Category="AI")
	void RequestMoveToLocation(const FVector& Destination);

	/** Annule le déplacement en cours vers une location (pas la chase). */
	UFUNCTION(BlueprintCallable, Category="AI")
	void AbortMoveToLocation();

	/** Cible actuelle (perception), nullptr si aucune. */
	UFUNCTION(BlueprintPure, Category="AI")
	AActor* GetTargetActor() const { return TargetActor; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void UpdateChasePath();
	void UpdateMoveToLocationPath();
	/** Déplace le pawn avec AddMovementInput vers le prochain point du chemin (accélération respectée). Retourne true si le déplacement "Move To Location" est terminé. */
	bool TickMovement(float DeltaTime);
	/** Appelé quand un RequestMoveToLocation a atteint sa destination (notifie le patrol). */
	void NotifyMoveToLocationCompleted();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UAIPerceptionComponent* PerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	AActor* TargetActor = nullptr;

	/** --- Move To Location (patrol points, return spline, wander) --- */
	UPROPERTY()
	FVector MoveToLocationDestination = FVector::ZeroVector;
	bool bHasMoveToLocationDestination = false;

	/** Points du chemin courant (chase ou move-to-location). */
	UPROPERTY()
	TArray<FVector> PathPoints;
	int32 PathPointIndex = 0;
	UPROPERTY(EditAnywhere, Category="AI|Movement", meta=(ClampMin="0.1", ClampMax="2.0"))
	float PathUpdateInterval = 0.35f;
	float PathUpdateTimer = 0.f;

	/** Rayon d'acceptation pour considérer qu'un point est atteint. */
	UPROPERTY(EditAnywhere, Category="AI|Movement", meta=(ClampMin="10", ClampMax="200"))
	float MoveAcceptRadius = 80.f;
};
