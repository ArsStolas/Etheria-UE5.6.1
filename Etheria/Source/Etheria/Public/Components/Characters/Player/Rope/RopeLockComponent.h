/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeLockComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeLockComponent.generated.h"

#if UE_BUILD_SHIPPING
	#define LOCK_LOG(Category, Verbosity, Format, ...)
#else
	#define LOCK_LOG(Category, Verbosity, Format, ...) \
	if (bLockDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
#endif

class ARopeAttachPoint;
class URopeDetectionComponent;
class APlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLockedPointChanged,
	ARopeAttachPoint*,
	NewLockedPoint
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeLockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeLockComponent();
	
	FORCEINLINE void SetPhysicallyAttached(bool bAttached) { bIsPhysicallyAttached = bAttached; }
	
	UFUNCTION(BlueprintPure, Category="Rope|Lock")
	FORCEINLINE ARopeAttachPoint* GetLockedPoint() const { return LockedPoint.Get(); }
	
	UFUNCTION(BlueprintPure, Category="Rope|Lock")
	FORCEINLINE bool HasLockedPoint() const { return LockedPoint.IsValid(); }

	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	bool TryLock();

	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	void Unlock();
	
	UFUNCTION(BlueprintCallable, Category="Rope|Lock")
	void OnDetectedPointChanged(ARopeAttachPoint* NewPoint);
	
	UPROPERTY(BlueprintAssignable, Category="Rope|Lock")
	FOnLockedPointChanged OnLockedPointChanged;

protected:
	virtual void BeginPlay() override;
	
	bool bIsPhysicallyAttached = false;

private:
	UPROPERTY()
	APlayerCharacter* OwnerCharacter = nullptr;
	
	UPROPERTY()
	URopeDetectionComponent* DetectionComponent = nullptr;

	TWeakObjectPtr<ARopeAttachPoint> LockedPoint;
	
	UPROPERTY(EditAnywhere, Category="Rope|Lock|Debug")
	bool bLockDebugMode = false;
	
	void BroadcastLockedPoint();
};
