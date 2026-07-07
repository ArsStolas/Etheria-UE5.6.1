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

class UMaterialInterface;

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

	// ------------------------------------------------------------------
	// Fade transition (black screen while teleporting between worlds)
	// ------------------------------------------------------------------
	/**
	 * Call when the portal interaction COMPLETES (fired automatically by
	 * ADungeonPortal::StartPortalInteraction). Captures the character's visible
	 * appearance (before any dissolve VFX runs) and immediately starts the fade
	 * to black, which is held until the teleport sequence finishes.
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon|Transition")
	void NotifyPortalInteractionStarted();

	/** If true, entering/leaving a dungeon fades the screen to black, teleports, then fades back in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition")
	bool bUseFadeTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(ClampMin="0.05", EditCondition="bUseFadeTransition"))
	float fadeOutDuration = 0.45f;

	/** Extra time held fully black before teleporting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(ClampMin="0.0", EditCondition="bUseFadeTransition"))
	float fadeHoldDuration = 0.15f;

	/**
	 * Verification delay spent fully black AFTER the teleport, before revealing:
	 * lets streaming finish, the character appearance restore, and the camera/spring
	 * arm finish blending to its new position.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(ClampMin="0.0"))
	float postTeleportSettleDelay = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(ClampMin="0.05", EditCondition="bUseFadeTransition"))
	float fadeInDuration = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(EditCondition="bUseFadeTransition"))
	FLinearColor fadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Transition", meta=(EditCondition="bUseFadeTransition"))
	bool bFadeAudio = true;

	// ------------------------------------------------------------------
	// Character appearance safety
	// ------------------------------------------------------------------
	/**
	 * The portal BP drives dissolve timelines on the player mesh. When the level loads
	 * too fast, the "appear" timeline can run while the "disappear" timeline is still
	 * playing: both fight over the same material parameters and the character can stay
	 * invisible. While the screen is black, lingering portal timelines are stopped and
	 * the mesh is restored to the exact appearance captured at interaction time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance")
	bool bStopPortalTimelinesOnArrive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance")
	bool bEnsureVisibleAfterTravel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance", meta=(EditCondition="bEnsureVisibleAfterTravel"))
	FName dissolveParamName = TEXT("Dissolve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance", meta=(EditCondition="bEnsureVisibleAfterTravel"))
	FName colorOpacityParamName = TEXT("Color Opacity");

	/** Fallback values used only if no appearance could be captured at interaction time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance", meta=(EditCondition="bEnsureVisibleAfterTravel"))
	float visibleDissolveValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Appearance", meta=(EditCondition="bEnsureVisibleAfterTravel"))
	float visibleColorOpacityValue = 1.f;

	/** Immediately restores the owner's mesh to the captured (or fallback) visible state. Safe to call from BP. */
	UFUNCTION(BlueprintCallable, Category="Dungeon|Appearance")
	void RestoreOwnerAppearance();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartPollingPreload();
	void StopPollingPreload();
	void PollPreload();

	void PerformEnterDungeon(const FName SpawnTag);
	void PerformReturnToOrigin();
	void StartCommitSequence();
	void OnFadeOutFinished();
	void FinishTransition();
	void FadeToBlack();
	void FadeFromBlack() const;
	class APlayerCameraManager* GetCameraManager() const;
	void StopPortalTimelines() const;
	void CaptureOwnerAppearance();

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
	FTimerHandle fadeTimerHandle;

	/** True from commit until the fade-in starts. */
	bool bTransitionInProgress = false;
	bool bPendingTransitionIsEnter = false;
	FName pendingTransitionSpawnTag;

	/** True once the fade to black has been requested for the current travel. */
	bool bFadeOutStarted = false;

	/** World time at which the screen is fully black. */
	double fadeBlackAtTime = 0.0;

	// Appearance captured at interaction time (before any dissolve VFX).
	bool bAppearanceCaptured = false;
	bool bCapturedDissolve = false;
	bool bCapturedColorOpacity = false;
	float capturedDissolveValue = 0.f;
	float capturedColorOpacityValue = 1.f;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> capturedOverlayMaterial = nullptr;
};
