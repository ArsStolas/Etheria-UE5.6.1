#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeLockComponent.generated.h"

class ARopeAttachPoint;
class URopeDetectionComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeLockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeLockComponent();

	/** Tente de verrouiller le point actuellement détecté */
	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	bool TryLock();

	/** Libère le point verrouillé */
	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	void Unlock();

	/** Y a-t-il un point verrouillé ? */
	UFUNCTION(BlueprintPure, Category="Rope|Lock")
	bool HasLockedPoint() const;

	/** Handler appelé lorsque le point détecté change */
	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	void OnDetectedPointChanged(ARopeAttachPoint* NewPoint);

	/** Retourne le point verrouillé */
	UFUNCTION(BlueprintPure, Category="Rope|Lock")
	ARopeAttachPoint* GetLockedPoint() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDebugMode = false;

private:
	UPROPERTY()
	URopeDetectionComponent* DetectionComponent = nullptr;

	TWeakObjectPtr<ARopeAttachPoint> LockedPoint;
};
