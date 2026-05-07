/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationBlueprintLibrary - Source
*/

#include "Core/System/Notifications/NotificationBlueprintLibrary.h"

#include "Core/System/Notifications/NotificationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UNotificationSubsystem* UNotificationBlueprintLibrary::GetNotificationSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UNotificationSubsystem>() : nullptr;
}

int32 UNotificationBlueprintLibrary::ShowNotification(const UObject* WorldContextObject, const FNotificationPayload& Payload)
{
	if (UNotificationSubsystem* NotificationSubsystem = GetNotificationSubsystem(WorldContextObject))
	{
		return NotificationSubsystem->ShowNotification(Payload);
	}

	return INDEX_NONE;
}

int32 UNotificationBlueprintLibrary::ShowNotificationFromDefinition(const UObject* WorldContextObject, const UNotificationDefinition* Definition)
{
	if (UNotificationSubsystem* NotificationSubsystem = GetNotificationSubsystem(WorldContextObject))
	{
		return NotificationSubsystem->ShowNotificationFromDefinition(Definition);
	}

	return INDEX_NONE;
}

int32 UNotificationBlueprintLibrary::ShowInfoNotification(const UObject* WorldContextObject, const FText& Title, const FText& Message, float Duration)
{
	FNotificationPayload Payload;
	Payload.Title = Title;
	Payload.Message = Message;
	Payload.Type = ENotificationType::Info;
	Payload.Priority = ENotificationPriority::Normal;
	Payload.Duration = Duration;

	return ShowNotification(WorldContextObject, Payload);
}

int32 UNotificationBlueprintLibrary::ShowWarningNotification(const UObject* WorldContextObject, const FText& Title, const FText& Message, float Duration)
{
	FNotificationPayload Payload;
	Payload.Title = Title;
	Payload.Message = Message;
	Payload.Type = ENotificationType::Warning;
	Payload.Priority = ENotificationPriority::High;
	Payload.Duration = Duration;

	return ShowNotification(WorldContextObject, Payload);
}

void UNotificationBlueprintLibrary::ClearAllNotifications(const UObject* WorldContextObject)
{
	if (UNotificationSubsystem* NotificationSubsystem = GetNotificationSubsystem(WorldContextObject))
	{
		NotificationSubsystem->ClearAllNotifications();
	}
}
