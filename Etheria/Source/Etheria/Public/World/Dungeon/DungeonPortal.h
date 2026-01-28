
/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonPortal" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "DungeonPortal.generated.h"

class UDungeonTravelComponent;

UCLASS()
class ETHERIA_API ADungeonPortal : public AActor
{
    GENERATED_BODY()

public:
    ADungeonPortal();

    UFUNCTION(BlueprintImplementableEvent, Category="Dungeon")
    void OnPortalInteracted(AActor* Interactor);

    UFUNCTION(BlueprintCallable, Category="Dungeon")
    void NotifySequenceFinished(AActor* Interactor);

protected:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* ReturnRoot;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ReturnPlane;

    UPROPERTY(VisibleAnywhere)
    UArrowComponent* ReturnArrow;

    UPROPERTY(EditAnywhere, Category="Dungeon")
    TSoftObjectPtr<UWorld> DestinationLevel;

    UPROPERTY(EditAnywhere, Category="Dungeon")
    FName DestinationSpawnTag;

    UPROPERTY(EditAnywhere, Category="Dungeon")
    bool bIsReturnPortal = false;
};
