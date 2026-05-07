/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationSubsystem - Source
*/

#include "Core/System/Notifications/NotificationSubsystem.h"

#include "Data/Notifications/NotificationDefinition.h"
#include "Engine/World.h"
#include "Interfaces/Widgets/NotificationWidget.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogNotificationSystem);

void UNotificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NextNotificationId = 1;
	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Subsystem initialized"));
}

void UNotificationSubsystem::Deinitialize()
{
	ClearAllNotifications();
	RegisteredWidget = nullptr;

	Super::Deinitialize();
}

int32 UNotificationSubsystem::ShowNotification(const FNotificationPayload& Payload)
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Ignored empty notification payload"));
		return INDEX_NONE;
	}

	FNotificationPayload PayloadWithId = BuildPayloadWithId(Payload);

	if (ActiveNotifications.Num() < MaxVisibleNotifications)
	{
		DisplayNotification(PayloadWithId);
	}
	else
	{
		QueueNotification(PayloadWithId);
	}

	return PayloadWithId.NotificationId;
}

int32 UNotificationSubsystem::ShowNotificationFromDefinition(const UNotificationDefinition* Definition)
{
	if (!Definition)
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Cannot show null notification definition"));
		return INDEX_NONE;
	}

	return ShowNotification(Definition->MakePayload());
}

bool UNotificationSubsystem::ClearNotification(int32 NotificationId)
{
	const int32 RemovedCount = ActiveNotifications.RemoveAll([NotificationId](const FNotificationPayload& Payload)
	{
		return Payload.NotificationId == NotificationId;
	});

	if (RemovedCount <= 0)
	{
		const int32 RemovedQueuedCount = QueuedNotifications.RemoveAll([NotificationId](const FNotificationPayload& Payload)
		{
			return Payload.NotificationId == NotificationId;
		});

		if (RemovedQueuedCount > 0)
		{
			UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Removed queued notification id=%d"), NotificationId);
			return true;
		}

		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Cannot clear unknown notification id=%d"), NotificationId);
		return false;
	}

	StopExpirationTimer(NotificationId);
	OnNotificationRemoved.Broadcast(NotificationId);

	if (RegisteredWidget)
	{
		RegisteredWidget->OnNotificationRemoved(NotificationId);
	}

	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Cleared id=%d"), NotificationId);
	TryDisplayQueuedNotifications();
	return true;
}

void UNotificationSubsystem::ClearAllNotifications()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<int32, FTimerHandle>& TimerPair : ExpirationTimers)
		{
			World->GetTimerManager().ClearTimer(TimerPair.Value);
		}
	}

	ExpirationTimers.Empty();
	ActiveNotifications.Empty();
	QueuedNotifications.Empty();
	OnNotificationsCleared.Broadcast();

	if (RegisteredWidget)
	{
		RegisteredWidget->OnNotificationsCleared();
	}

	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Cleared all notifications"));
}

void UNotificationSubsystem::RegisterNotificationWidget(UNotificationWidget* Widget)
{
	RegisteredWidget = Widget;

	if (!RegisteredWidget)
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Tried to register a null widget"));
		return;
	}

	for (const FNotificationPayload& Payload : ActiveNotifications)
	{
		RegisteredWidget->OnNotificationReceived(Payload);
	}

	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Widget registered: %s"), *RegisteredWidget->GetName());
}

void UNotificationSubsystem::UnregisterNotificationWidget(UNotificationWidget* Widget)
{
	if (RegisteredWidget == Widget)
	{
		RegisteredWidget = nullptr;
	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Widget unregistered"));
	}
}

void UNotificationSubsystem::SetMaxVisibleNotifications(int32 NewMaxVisibleNotifications)
{
	MaxVisibleNotifications = FMath::Max(1, NewMaxVisibleNotifications);
	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Max visible notifications set to %d"), MaxVisibleNotifications);

	TryDisplayQueuedNotifications();
}

void UNotificationSubsystem::DisplayNotification(const FNotificationPayload& Payload)
{
	ActiveNotifications.Add(Payload);
	OnNotificationReceived.Broadcast(Payload);

	if (RegisteredWidget)
	{
		RegisteredWidget->OnNotificationReceived(Payload);
	}
	else
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] No notification widget registered"));
	}

	StartExpirationTimer(Payload);

	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Shown id=%d type=%d priority=%d title='%s'"),
		Payload.NotificationId,
		static_cast<int32>(Payload.Type),
		static_cast<int32>(Payload.Priority),
		*Payload.Title.ToString());
}

void UNotificationSubsystem::QueueNotification(const FNotificationPayload& Payload)
{
	QueuedNotifications.Add(Payload);

	UE_LOG(LogNotificationSystem, Log, TEXT("[Notification] Queued id=%d queue_size=%d title='%s'"),
		Payload.NotificationId,
		QueuedNotifications.Num(),
		*Payload.Title.ToString());
}

void UNotificationSubsystem::TryDisplayQueuedNotifications()
{
	while (ActiveNotifications.Num() < MaxVisibleNotifications && QueuedNotifications.Num() > 0)
	{
		const FNotificationPayload NextPayload = QueuedNotifications[0];
		QueuedNotifications.RemoveAt(0);
		DisplayNotification(NextPayload);
	}
}

void UNotificationSubsystem::StartExpirationTimer(const FNotificationPayload& Payload)
{
	if (Payload.Duration <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Cannot start expiration timer without a world"));
		return;
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateUObject(this, &UNotificationSubsystem::HandleNotificationExpired, Payload.NotificationId),
		Payload.Duration,
		false);

	ExpirationTimers.Add(Payload.NotificationId, TimerHandle);
}

void UNotificationSubsystem::HandleNotificationExpired(int32 NotificationId)
{
	ClearNotification(NotificationId);
}

void UNotificationSubsystem::StopExpirationTimer(int32 NotificationId)
{
	if (FTimerHandle* TimerHandle = ExpirationTimers.Find(NotificationId))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}

		ExpirationTimers.Remove(NotificationId);
	}
}

FNotificationPayload UNotificationSubsystem::BuildPayloadWithId(const FNotificationPayload& Payload)
{
	FNotificationPayload PayloadWithId = Payload;
	PayloadWithId.NotificationId = NextNotificationId++;
	return PayloadWithId;
}
