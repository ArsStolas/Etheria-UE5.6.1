/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueParticipantComponent" - Source
 */
#include "Core/Dialogue/Components/DialogueParticipantComponent.h"

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"

UDialogueParticipantComponent::UDialogueParticipantComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

USkeletalMeshComponent* UDialogueParticipantComponent::ResolveSkeletalMesh() const
{
    if (AActor* OwnerActor = GetOwner())
    {
        return OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
    }
    return nullptr;
}
