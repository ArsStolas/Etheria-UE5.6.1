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
class APlayerCharacter;

UCLASS()
class ETHERIA_API AWindStreamZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AWindStreamZone();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind Stream|Components")
	UBoxComponent* TriggerZone;

	// === DEBUG ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	FColor DebugColor = FColor(0, 200, 255, 100);

	// === BOOST SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind Stream|Settings", meta=(ClampMin="0.0"))
	float BoostForce = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind Stream|Settings", meta=(ClampMin="0.0"))
	float ApplyInterval = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind Stream|Settings")
	FVector StreamDirection = FVector(1,0,0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind Stream|Settings")
	bool bAffectOnlyDive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind Stream|Settings")
	bool bOneTimeUse = false;

private:
	// Timer et joueurs actifs
	UPROPERTY()
	TMap<APlayerCharacter*, FTimerHandle> ActivePlayers;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyStreamMovement(APlayerCharacter* Player);
};

