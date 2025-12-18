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

	/** Lance le swing */
	void StartSwing();

	/** Stop le swing */
	void StopSwing();

	bool IsSwinging() const { return bIsSwinging; }

protected:
	void UpdateSwing(float DeltaTime);

	bool ShouldStartSwing() const;
	bool IsFarEnoughFromGround() const;
	bool HasTouchedGround() const;

	UFUNCTION()
	void OnRopeTensioned();

private:
	/* ===== Owner ===== */
	APlayerCharacter* OwnerCharacter = nullptr;
	UCharacterMovementComponent* MoveComp = nullptr;

	/* ===== Rope ===== */
	URopeAttachComponent* AttachComponent = nullptr;
	URopeLockComponent* LockComponent = nullptr;
	TWeakObjectPtr<ARopeAttachPoint> SwingPoint;

	/* ===== Swing State ===== */
	bool bIsSwinging = false;
	FVector VelocityProjected = FVector::ZeroVector;

	/* ===== Tuning ===== */
	UPROPERTY(EditAnywhere, Category="Swing")
	float FallingSpeedToStartSwing = 200.f;

	UPROPERTY(EditAnywhere, Category="Swing")
	float MinHeightAboveGround = 80.f;

	UPROPERTY(EditAnywhere, Category="Swing")
	float GroundStopDistance = 25.f;

	/* ===== Debug ===== */
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDebugSwing = false;

	UPROPERTY(EditAnywhere, Category="Debug", meta=(EditCondition="bDebugSwing"))
	float DebugLineThickness = 2.f;
};
