/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIPackRegistrySubsystem - Header"
 * Notes: O(1) lookup of pack members by PackID, so AI don't TActorIterator the whole level on hot paths
 *        (pack alerts, leader election, herd following). AI register on BeginPlay, unregister on EndPlay/death.
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AIPackRegistrySubsystem.generated.h"

class ABaseAICharacter;

UCLASS()
class ETHERIA_API UAIPackRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(ABaseAICharacter* AI, FName PackID);
	void Unregister(ABaseAICharacter* AI, FName PackID);

	/** Fills OutMembers with all still-valid members of PackID (excludes nothing — caller filters by distance/alive). */
	void GetPackMembers(FName PackID, TArray<ABaseAICharacter*>& OutMembers) const;

private:
	TMap<FName, TArray<TWeakObjectPtr<ABaseAICharacter>>> Packs;
};
