/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueParticipantComponent" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueParticipantComponent.generated.h"

class USkeletalMeshComponent;

/**
 * Add this to any actor that can speak / show dialogue bubbles.
 * Keeps display information lightweight & data-driven.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UDialogueParticipantComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDialogueParticipantComponent();

    UFUNCTION(BlueprintPure, Category="Dialogue|Participant")
    FText GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintCallable, Category="Dialogue|Participant")
    void SetDisplayName(const FText& NewName) { DisplayName = NewName; }

    /** Socket used as attachment point for bubbles (optional). */
    UFUNCTION(BlueprintPure, Category="Dialogue|Participant")
    FName GetBubbleSocketName() const { return BubbleSocketName; }

    /** Relative offset used when socket is not found or not provided. */
    UFUNCTION(BlueprintPure, Category="Dialogue|Participant")
    FVector GetBubbleRelativeOffset() const { return BubbleRelativeOffset; }

    /** Try to resolve a skeletal mesh on this actor (best-effort) so systems can attach to sockets. */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Participant")
    USkeletalMeshComponent* ResolveSkeletalMesh() const;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Participant", meta=(AllowPrivateAccess="true"))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Participant", meta=(AllowPrivateAccess="true"))
    FName BubbleSocketName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Participant", meta=(AllowPrivateAccess="true"))
    FVector BubbleRelativeOffset = FVector(0.f, 0.f, 120.f);
};
