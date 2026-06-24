/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonFallVolume" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonFallVolume.generated.h"

class UBoxComponent;
class ACharacter;

UCLASS()
class ETHERIA_API ADungeonFallVolume : public AActor
{
	GENERATED_BODY()

public:
	ADungeonFallVolume();

	/** Optionnel : VFX / son / camera shake juste après le renvoi du joueur sur l'entrée. */
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon")
	void OnPlayerRespawned(ACharacter* Player);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Components")
	USceneComponent* root = nullptr;

	/** Scale cette box dans l'éditeur pour couvrir la zone de chute (place-la BIEN en dessous du sol jouable). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Components")
	UBoxComponent* triggerBox = nullptr;

	/**
	 * Acteur vers lequel le joueur est téléporté (ex: ton acteur d'entrée tagué "DungeonEntry").
	 * Place ce volume DANS le level du donjon et choisis une cible du MÊME level.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Dungeon|Respawn")
	AActor* respawnTarget = nullptr;

	/** Fallback si respawnTarget est vide : premier acteur trouvé avec ce tag DANS le même level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Respawn")
	FName respawnTag = TEXT("DungeonEntry");

	/** Soulève le joueur de la demi-hauteur de sa capsule pour ne pas spawn dans le sol. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Respawn")
	bool bAutoLiftByCapsuleHalfHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Respawn")
	float extraTeleportZ = 5.f;

private:
	bool ResolveRespawnTransform(FTransform& OutTransform) const;
	bool RespawnCharacter(ACharacter* Character) const;
	float ComputeLiftZ(const ACharacter* Character) const;
};