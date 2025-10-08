/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EE_PlayerEventDemoComponent" - Header
 * Note : Will be deleted
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EE_PlayerEventDemoComponent.generated.h"

// 1) Single-cast (C++): sprint on/off + coût
DECLARE_DELEGATE_TwoParams(FOnSprintToggle, bool /*bSprinting*/, float /*StaminaCost*/);

// 2) Multi-cast (C++): action rapide (id + puissance)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuickAction, int32 /*ActionId*/, float /*Power*/);

// 3) Dynamic single-cast (BP): callback (message + severity)
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnAskBP, const FString&, Context, int32, Severity);

// 4) Dynamic multi-cast (BP): ping monde (position + tag)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPingWorld, FVector, Location, FName, Tag);

UCLASS(ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UEE_PlayerEventDemoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEE_PlayerEventDemoComponent();
    
    UFUNCTION(BlueprintCallable, Category="Events|QuickDemo")
    void Demo_TriggerSingleCast(bool bSprinting, float StaminaCost = 10.f);

    UFUNCTION(BlueprintCallable, Category="Events|QuickDemo")
    void Demo_TriggerMultiCast(int32 ActionId = 1, float Power = 1.f);

    UFUNCTION(BlueprintCallable, Category="Events|QuickDemo", meta=(AutoCreateRefTerm="Callback,Context"))
    void Demo_CallDynamicCallback(const FString& Context, int32 Severity, const FOnAskBP& Callback);

    UFUNCTION(BlueprintCallable, Category="Events|QuickDemo")
    void Demo_BroadcastPing(FVector Location, FName Tag);

    // 4) Dynamic multi-cast
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnPingWorld OnPingWorld;

private:
    // 1) Single-cast 
    FOnSprintToggle OnSprintToggle;
    void HandleSprintToggle(bool bSprinting, float StaminaCost);

    // 2) Multi-cast w 2 handlers print
    FOnQuickAction OnQuickAction;
    void HandleQuickAction_UI(int32 ActionId, float Power);
    void HandleQuickAction_SFX(int32 ActionId, float Power);

protected:
    virtual void BeginPlay() override;
    
};
