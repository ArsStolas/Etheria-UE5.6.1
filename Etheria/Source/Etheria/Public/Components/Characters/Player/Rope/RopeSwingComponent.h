#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeSwingComponent.generated.h"

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
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Lance le swing dans le vide */
	void StartSwing();

	/** Stop le swing */
	void StopSwing();

	/** Détache la corde complètement */
	void Detach();

	/** Retourne si le joueur est en swing */
	bool IsSwinging() const { return bIsSwinging; }

protected:
	/** Mise à jour de la position du joueur pendant le swing */
	void UpdateSwing(float DeltaTime);
	
	UFUNCTION()
	void OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint);

private:
	/** Le joueur propriétaire */
	APlayerCharacter* OwnerCharacter = nullptr;

	/** Composant de mouvement du joueur */
	UCharacterMovementComponent* MoveComp = nullptr;

	/** Composant RopeAttach pour vérifier si on est attaché */
	URopeAttachComponent* AttachComponent = nullptr;

	/** Composant RopeLock pour connaître le point verrouillé */
	URopeLockComponent* LockComponent = nullptr;

	/** Point de suspension actuel */
	TWeakObjectPtr<ARopeAttachPoint> SwingPoint;

	/** Flag si on est en swing actif */
	bool bIsSwinging = false;

	/** Longueur de la corde */
	float RopeLength = 0.f;

	/** Vitesse projetée pour la simulation du swing */
	FVector VelocityProjected = FVector::ZeroVector;
};
