// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "Interaction/InteractionData.h"
#include "NativeGameplayTags.h"
#include "OrionInteractionComponent.generated.h"



class UOrionInteractionTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOrionInteractionSignature, UOrionInteractionTargetComponent*, InteractionTarget, FGameplayTag, InteractionTargetTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrionAvailableInteractionChanged, const TArray<class UOrionInteractionTargetComponent*>&, AvailableInteractions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrionOnInteractingChanged, bool, bIsInteracting);
/**
 * Interaction component
 */
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, HideDropdown, ClassGroup = "OrionRPG", DisplayName = "Orion Interaction")
class UOrionInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UOrionInteractionComponent();

	virtual void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//We cache the OwningController, we won't cache pawn as this might change
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<APlayerController> OwningController;

	/**The radius where the character becomes "aware" of interactable objects. This is used detect and trigger UI elements for highlighting the object.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	float DetectionRadius;

	/**
	* Give notification based on configured netmode
	* E_Server		: Default, will notify only in server side
	* E_All			: Using multicast to notify interaction result
	* 
	* For client only notification, use the event with prefix local_ instead
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	EOrionInteractionNetMode InteractionNetMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	EOrionInteractionType InteractionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (DisplayName = "Debug Trace?"))
	bool bDebugTrace;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "InteractionType == EOrionInteractionType::E_InteractionPawn", HideEditConditionToggle = true, EditConditionHides = true))  
    FVector InteractionOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "InteractionType == EOrionInteractionType::E_InteractionPawn", HideEditConditionToggle = true, EditConditionHides = true))
	float InteractionRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "InteractionType == EOrionInteractionType::E_InteractionCamera", HideEditConditionToggle = true, EditConditionHides = true))
	float InteractionTraceDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	float InteractionTraceIntervalSecond;

	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UOrionInteractionTargetComponent> CurrentInteractionTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TArray<TObjectPtr<UOrionInteractionTargetComponent>> AvailableInteractionTargets;

	/*Index of the interaction list chosen by player*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Interaction")
	int32 InteractionIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Interaction")
	bool bIsInteractionEnabled;


	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionOnInteractingChanged OnInteractingStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnInteractionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionAvailableInteractionChanged OnAvailableInteractionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnAvailableInteractionAdded;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnAvailableInteractionRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnInteractionCanceled;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnInteractionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOrionInteractionSignature OnInteractionBlocked;

protected:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bInteracting, Category = "Interaction")
	bool bInteracting;

protected:
	UFUNCTION()
	void OnRep_bInteracting();
	/**
	 * Sets bInteracting Value and Calls Related On Rep
	 */

	UFUNCTION()
	void SetInteracting(bool bNewInteracting)
	{
		bInteracting = bNewInteracting;

		if (GetNetMode() != ENetMode::NM_DedicatedServer)
		{
			OnRep_bInteracting();
		}
	};

public:
	UFUNCTION(BlueprintPure, Category = "Interaction")
	virtual APawn* GetOwningPawn() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	virtual APlayerController* GetOwningController() const;

	UFUNCTION()
	virtual void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StartInteraction();

	/**
	 * RPC to Server To Start the Interaction
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartInteraction();
	bool Server_StartInteraction_Validate() { return true; };

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	/**
	 * Tries to Cancel an Interaction on Authority Side of the Interaction
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CancelInteraction();

	/**
	 * RPC to Server To Cancel the Interaction
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CancelInteraction();
	bool Server_CancelInteraction_Validate() { return true; };

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndInteraction(UOrionInteractionTargetComponent* InteractionTarget, bool bCompleted);

	UFUNCTION()
	void RegisterNewInteractionTarget(UOrionInteractionTargetComponent* NewInteractionTarget);

	UFUNCTION()
	void DeRegisterInteractionTarget();

	UFUNCTION(BlueprintPure, Category = Interactor)
	bool IsLocalInteractor() const
	{
		const ENetMode NetMode = GetNetMode();
		const ENetRole Role = GetInteractorRole();

		if (NetMode == NM_Standalone)
		{
			return true;
		}

		if (NetMode == NM_Client && Role == ROLE_AutonomousProxy)
		{
			return true;
		}

		if (GetInteractorRemoteRole() != ROLE_AutonomousProxy && Role == ROLE_Authority)
		{
			return true;
		}

		return false;
	};

	/**
	 * Returns the Component Owner Role
	 */
	UFUNCTION(BlueprintPure, Category = Interactor)
	FORCEINLINE ENetRole GetInteractorRole() const
	{
		return IsValid(GetOwner()) ? GetOwner()->GetLocalRole() : ENetRole::ROLE_None;
	};

	/**
	 * Returns the Component Owner Remote Role
	 */
	UFUNCTION(BlueprintPure, Category = Interactor)
	FORCEINLINE ENetRole GetInteractorRemoteRole() const
	{
		return IsValid(GetOwner()) ? GetOwner()->GetRemoteRole() : ENetRole::ROLE_None;
	};

	/**
	 * Determines Whether this Interactor Instance Should Tick
	 *
	 * @return True If Local is Owner Or Instance is on Server
	 */
	UFUNCTION()
	FORCEINLINE bool ShouldTickInstance() const
	{
		return GetNetMode() == NM_Client && GetInteractorRole() != ROLE_AutonomousProxy ? false : true;
	}

	/**
	 * Handles Interaction Notification Based on Config
	 *
	 * @param InteractionResult - Result of the Interaction
	 */
	UFUNCTION()
	void NotifyInteraction(EOrionInteractionResult NewInteractionResult);

	/**
	 * Owner Only Interaction State Notification
	 *
	 * @param InteractionResult - Result of the Interaction
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyInteraction(EOrionInteractionResult NewInteractionResult);

	/**
	 * Interaction Target State Notification
	 *
	 * @param InteractionResult - Result of the Interaction
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyInteractionTarget(EOrionInteractionResult NewInteractionResult);

	/**
	 * Multi Cast Call to all Clients Notifying Interaction State
	 *
	 * @param InteractionResult - Result of the Interaction
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyInteraction(EOrionInteractionResult NewInteractionResult);

	UFUNCTION()
	void CallInteractionResult(EOrionInteractionResult NewInteractionResult, UOrionInteractionTargetComponent* InteractionTarget, FGameplayTag Tag);
private:
	UFUNCTION()
	void UpdateInteractionTrace(); 

	UFUNCTION()
	UOrionInteractionTargetComponent* GetInteractionTrace();

	UFUNCTION()
	void DetectAvailableInteractionTargets();
};
