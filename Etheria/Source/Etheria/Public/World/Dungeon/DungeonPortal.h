/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonPortal" - Header
 */

#pragma once

#include "Interfaces/Interaction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonPortal.generated.h"

class UBoxComponent;
class UArrowComponent;
class UStaticMeshComponent;
class UDungeonTravelComponent;


UCLASS()
class ETHERIA_API ADungeonPortal : public AActor, public IInteraction
{
	GENERATED_BODY()

public:
	ADungeonPortal();

	virtual void Interact_Implementation(AActor* Target) override;
	virtual void CanReceiveTrace_Implementation() override;

	/**
	 * If you OVERRIDE the Interface Event "Interact" in this BP, call this function first.
	 * This prevents losing the base C++ portal logic.
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void StartPortalInteraction(AActor* Target);

	/**
	 * Called from Blueprint at the END of your departure cinematic/VFX.
	 * If called early (preload not ready), the travel component queues the commit and auto-fires when ready.
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void NotifySequenceFinished(AActor* Target);

	// ------------------------------------------------------------------
	// Blueprint hooks
	// ------------------------------------------------------------------
	/** Optional: use if you want a departure sequence. */
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon")
	void OnPortalSequenceStart(AActor* Target);

	/** Called right AFTER the player is teleported into the dungeon. */
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon")
	void OnArriveInDungeon(AActor* Target);

	/**
	 * Called right AFTER the player is teleported back to the original world.
	 * IMPORTANT: In V2.3.6 this event is fired on the ORIGIN portal actor (the one placed in the main world),
	 * so your Timeline won't stop when the dungeon level instance unloads.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon")
	void OnArriveBackToOrigin(AActor* Target);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Components")
	USceneComponent* root = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Components")
	UBoxComponent* interactionBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return")
	UStaticMeshComponent* returnPlane = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return")
	UArrowComponent* returnArrow = nullptr;

	/** Where the dungeon instance is streamed in the persistent world (set FAR to avoid overlap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Destination", meta=(EditCondition="!bIsReturnPortal"))
	FVector dungeonInstanceLocation = FVector(10000000.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Destination", meta=(EditCondition="!bIsReturnPortal"))
	TSoftObjectPtr<UWorld> destinationLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Destination", meta=(EditCondition="!bIsReturnPortal"))
	FName destinationSpawnTag = TEXT("DungeonEntry");

	/** If true, this portal commits "return to origin" instead of "enter dungeon". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bIsReturnPortal = false;

private:
	UDungeonTravelComponent* GetTravelComponentFrom(AActor* Target) const;
};
