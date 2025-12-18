#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeConstraintComponent.generated.h"

class APlayerCharacter;
class ARopeAttachPoint;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRopeTensioned);

/**
 * Gère la contrainte physique de distance de la corde
 * (indépendant de Swing / Pull)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeConstraintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeConstraintComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Active la contrainte */
	void ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength);

	/** Désactive la contrainte */
	void DeactivateConstraint();

	bool IsActive() const { return bIsActive; }

	ARopeAttachPoint* GetAnchor() const { return Anchor.Get(); }
	float GetRopeLength() const { return RopeLength; }
	
	UPROPERTY(BlueprintAssignable)
	FOnRopeTensioned OnRopeTensioned;

private:
	/** Applique la contrainte de distance */
	void ApplyConstraint();

	APlayerCharacter* OwnerCharacter = nullptr;
	UCharacterMovementComponent* MoveComp = nullptr;

	TWeakObjectPtr<ARopeAttachPoint> Anchor;

	float RopeLength = 0.f;
	bool bIsActive = false;
};
