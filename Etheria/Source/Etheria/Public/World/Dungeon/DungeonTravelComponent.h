/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDungeonTravelComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "DungeonTravelComponent.generated.h"

UENUM(BlueprintType)
enum class EDungeonTravelState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Preloading UMETA(DisplayName="Preloading"),
	ReadyToEnter UMETA(DisplayName="ReadyToEnter"),
	InDungeon UMETA(DisplayName="InDungeon"),
};

USTRUCT(BlueprintType)
struct FDungeonReturnData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return")
	FTransform ReturnTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return")
	FRotator ReturnControlRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return")
	TSoftObjectPtr<UWorld> OriginLevel;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonPreloadReadySignature, TSoftObjectPtr<UWorld>, Level);

UCLASS(ClassGroup=(Dungeon), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UDungeonTravelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDungeonTravelComponent();

	/**
	 * Call from the MAIN WORLD portal (enter portal).
	 * We keep this reference so OnArriveBackToOrigin runs on the origin portal (persistent),
	 * not on the dungeon exit portal (unloaded).
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void SetEnterPortalActor(AActor* PortalActor);

	/** Call from the DUNGEON portal (exit portal). */
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void SetExitPortalActor(AActor* PortalActor);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void BeginPreloadFromPortal(TSoftObjectPtr<UWorld> DungeonLevel, const FVector& InstanceLocation, const FTransform& ReturnTransform, const FRotator& ReturnControlRotation);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void CommitEnterDungeon(const FName SpawnTag);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void CommitReturnToOrigin();

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool IsPreloadReady() const { return travelState == EDungeonTravelState::ReadyToEnter; }

	UPROPERTY(BlueprintAssignable, Category="Dungeon")
	FDungeonPreloadReadySignature OnPreloadReady;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Teleport")
	bool bAutoLiftByCapsuleHalfHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Teleport")
	float extraTeleportZ = 5.f;

protected:
	virtual void BeginPlay() override;

private:
	void StartPollingPreload();
	void StopPollingPreload();
	void PollPreload();

	bool TryResolveSpawnTransform(const FName SpawnTag, FTransform& OutTransform) const;
	void TeleportOwnerTo(const FTransform& Target, const FRotator& ControlRot) const;
	float ComputeLiftZ() const;

	void TryTriggerArriveInDungeonEvent() const;
	void TryTriggerArriveBackToOriginEvent() const;

private:
	UPROPERTY()
	ULevelStreamingDynamic* streamingLevel = nullptr;

	UPROPERTY()
	TSoftObjectPtr<UWorld> pendingLevel;

	UPROPERTY()
	FDungeonReturnData returnData;

	UPROPERTY(VisibleAnywhere, Category="Dungeon")
	EDungeonTravelState travelState = EDungeonTravelState::Idle;

	bool bPendingEnterCommit = false;

	UPROPERTY()
	FName pendingSpawnTag;

	UPROPERTY()
	FVector cachedInstanceLocation = FVector::ZeroVector;

	/** The portal placed in the main world that started the travel. */
	UPROPERTY()
	TWeakObjectPtr<AActor> enterPortalActor;

	/** The portal inside the dungeon that triggers return. Might be unloaded after return. */
	UPROPERTY()
	TWeakObjectPtr<AActor> exitPortalActor;

	FTimerHandle preloadPollHandle;
};
