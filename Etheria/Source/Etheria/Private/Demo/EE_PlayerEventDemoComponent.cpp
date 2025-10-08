/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EE_PlayerEventDemoComponent" - Source
 * Note : Will be deleted
 */

#include "Demo/EE_PlayerEventDemoComponent.h"
#include "Engine/Engine.h"

UEE_PlayerEventDemoComponent::UEE_PlayerEventDemoComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEE_PlayerEventDemoComponent::BeginPlay()
{
    Super::BeginPlay();

    // Bind pour montrer le single & multi 
    OnSprintToggle.BindUObject(this, &UEE_PlayerEventDemoComponent::HandleSprintToggle);

    OnQuickAction.AddUObject(this, &UEE_PlayerEventDemoComponent::HandleQuickAction_UI);
    OnQuickAction.AddUObject(this, &UEE_PlayerEventDemoComponent::HandleQuickAction_SFX);
}

void UEE_PlayerEventDemoComponent::Demo_TriggerSingleCast(bool bSprinting, float StaminaCost)
{
    OnSprintToggle.ExecuteIfBound(bSprinting, StaminaCost);
}

void UEE_PlayerEventDemoComponent::Demo_TriggerMultiCast(int32 ActionId, float Power)
{
    OnQuickAction.Broadcast(ActionId, Power);
}

void UEE_PlayerEventDemoComponent::Demo_CallDynamicCallback(const FString& Context, int32 Severity, const FOnAskBP& Callback)
{
    if (Callback.IsBound())
    {
        Callback.Execute(Context, Severity);
    }
    else if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            TEXT("[DynamicSingle] Aucun callback BP lié"));
    }
}

void UEE_PlayerEventDemoComponent::Demo_BroadcastPing(FVector Location, FName Tag)
{
    OnPingWorld.Broadcast(Location, Tag);
}

// ---------------- Handlers ----------------

void UEE_PlayerEventDemoComponent::HandleSprintToggle(bool bSprinting, float StaminaCost)
{
    if (GEngine)
    {
        const FString Msg = bSprinting
            ? FString::Printf(TEXT("[SingleCast] Sprint ON (cost=%.1f)"), StaminaCost)
            : TEXT("[SingleCast] Sprint OFF");
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, Msg);
    }
}

void UEE_PlayerEventDemoComponent::HandleQuickAction_UI(int32 ActionId, float Power)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            *FString::Printf(TEXT("[MultiCast/UI] Action %d (Power=%.1f)"), ActionId, Power));
    }
}

void UEE_PlayerEventDemoComponent::HandleQuickAction_SFX(int32 ActionId, float Power)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            *FString::Printf(TEXT("[MultiCast/SFX] Action %d (Power=%.1f)"), ActionId, Power));
    }
}
