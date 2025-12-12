/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestLocationTrigger" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestLocationTrigger.generated.h"

class UBoxComponent;
class UQuestComponent;

/**
 * Simple trigger actor used for "ReachLocation" quest objectives.
 * When the owner with a QuestComponent overlaps, it notifies the quest system.
 */
UCLASS()
class ETHERIA_API AQuestLocationTrigger : public AActor
{
    GENERATED_BODY()

public:
    AQuestLocationTrigger();

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    TObjectPtr<UBoxComponent> boxComponent;

    /** Identifier used by objectives.targetId for ReachLocation objectives. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
    FName locationId = NAME_None;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
    FORCEINLINE FName GetLocationId() const { return locationId; }
};
