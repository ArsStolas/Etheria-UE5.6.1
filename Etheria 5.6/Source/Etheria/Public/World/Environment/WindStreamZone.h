/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindStreamZone - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindStreamZone.generated.h"

class UBoxComponent;

UCLASS()
class ETHERIA_API AWindStreamZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AWindStreamZone();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boost Zone")
	UBoxComponent* TriggerZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	FColor DebugColor = FColor(0, 200, 255, 100);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost Zone", meta=(ClampMin="0.0"))
	float BoostForce = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost Zone", meta=(ClampMin="0.0"))
	float BoostDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost Zone")
	bool bOneTimeUse = false;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bUsed = false;
};
