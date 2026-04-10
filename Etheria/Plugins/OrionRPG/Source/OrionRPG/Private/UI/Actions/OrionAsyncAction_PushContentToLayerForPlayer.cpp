// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Actions/OrionAsyncAction_PushContentToLayerForPlayer.h"

#include "Engine/Engine.h"
#include "UI/OrionPrimaryGameLayout.h"
#include "UObject/Stack.h"
#include "Widgets/CommonActivatableWidgetContainer.h"


UOrionAsyncAction_PushContentToLayerForPlayer::UOrionAsyncAction_PushContentToLayerForPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UOrionAsyncAction_PushContentToLayerForPlayer* UOrionAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(APlayerController* InOwningPlayer, TSoftClassPtr<UCommonActivatableWidget> InWidgetClass, FGameplayTag InLayerName, bool bSuspendInputUntilComplete)
{
	if (InWidgetClass.IsNull())
	{
		FFrame::KismetExecutionMessage(TEXT("PushContentToLayerForPlayer was passed a null WidgetClass"), ELogVerbosity::Error);
		return nullptr;
	}

	if (UWorld* World = GEngine->GetWorldFromContextObject(InOwningPlayer, EGetWorldErrorMode::LogAndReturnNull))
	{
		UOrionAsyncAction_PushContentToLayerForPlayer* Action = NewObject<UOrionAsyncAction_PushContentToLayerForPlayer>();
		Action->WidgetClass = InWidgetClass;
		Action->OwningPlayerPtr = InOwningPlayer;
		Action->LayerName = InLayerName;
		Action->bSuspendInputUntilComplete = bSuspendInputUntilComplete;
		Action->RegisterWithGameInstance(World);

		return Action;
	}

	return nullptr;
}

void UOrionAsyncAction_PushContentToLayerForPlayer::Cancel()
{
	Super::Cancel();

	if (StreamingHandle.IsValid())
	{
		StreamingHandle->CancelHandle();
		StreamingHandle.Reset();
	}
}

void UOrionAsyncAction_PushContentToLayerForPlayer::Activate()
{
	if (UOrionPrimaryGameLayout* RootLayout = UOrionPrimaryGameLayout::GetPrimaryGameLayout(OwningPlayerPtr.Get()))
	{
		TWeakObjectPtr<UOrionAsyncAction_PushContentToLayerForPlayer> WeakThis = this;
		StreamingHandle = RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(LayerName, bSuspendInputUntilComplete, WidgetClass, [this, WeakThis](EOrionAsyncWidgetLayerState State, UCommonActivatableWidget* Widget) {
			if (WeakThis.IsValid())
			{
				switch (State)
				{
					case EOrionAsyncWidgetLayerState::Initialize:
						BeforePush.Broadcast(Widget);
						break;
					case EOrionAsyncWidgetLayerState::AfterPush:
						AfterPush.Broadcast(Widget);
						SetReadyToDestroy();
						break;
					case EOrionAsyncWidgetLayerState::Canceled:
						SetReadyToDestroy();
						break;
				}
			}
			SetReadyToDestroy();
		});
	}
	else
	{
		SetReadyToDestroy();
	}
}

