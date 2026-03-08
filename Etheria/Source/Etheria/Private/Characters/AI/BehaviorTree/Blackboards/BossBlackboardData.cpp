/*
* Etheria's End Project, 2025
* Boss Blackboard Data - Source
*
* En éditeur : créer un asset Blackboard à partir de cette classe, puis ajouter
* manuellement la clé "TargetActor" (type Object) dans le Blackboard.
*/

#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"

UBossBlackboardData::UBossBlackboardData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// La clé TargetActor doit être ajoutée dans l'éditeur sur l'asset Blackboard.
	// Nom de la clé : BossBlackboardKeys::TargetActor ("TargetActor"), type : Object.
}
