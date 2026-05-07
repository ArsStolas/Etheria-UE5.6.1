/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationSubsystem - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Core/System/Notifications/NotificationTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "NotificationSubsystem.generated.h"

class UNotificationDefinition;
class UNotificationWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNotificationReceivedSignature, const FNotificationPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNotificationRemovedSignature, int32, NotificationId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotificationsClearedSignature);

UCLASS(BlueprintType)
class ETHERIA_API UNotificationSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Notifications")
	int32 ShowNotification(const FNotificationPayload& Payload);

	UFUNCTION(BlueprintCallable, Category="Notifications")
	int32 ShowNotificationFromDefinition(const UNotificationDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="Notifications")
	bool ClearNotification(int32 NotificationId);

	UFUNCTION(BlueprintCallable, Category="Notifications")
	void ClearAllNotifications();

	UFUNCTION(BlueprintCallable, Category="Notifications")
	void RegisterNotificationWidget(UNotificationWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="Notifications")
	void UnregisterNotificationWidget(UNotificationWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(ClampMin="1"))
	void SetMaxVisibleNotifications(int32 NewMaxVisibleNotifications);

	UFUNCTION(BlueprintPure, Category="Notifications")
	TArray<FNotificationPayload> GetActiveNotifications() const { return ActiveNotifications; }

	UFUNCTION(BlueprintPure, Category="Notifications")
	TArray<FNotificationPayload> GetQueuedNotifications() const { return QueuedNotifications; }

	UFUNCTION(BlueprintPure, Category="Notifications")
	int32 GetMaxVisibleNotifications() const { return MaxVisibleNotifications; }

	UPROPERTY(BlueprintAssignable, Category="Notifications")
	FNotificationReceivedSignature OnNotificationReceived;

	UPROPERTY(BlueprintAssignable, Category="Notifications")
	FNotificationRemovedSignature OnNotificationRemoved;

	UPROPERTY(BlueprintAssignable, Category="Notifications")
	FNotificationsClearedSignature OnNotificationsCleared;

private:
	void DisplayNotification(const FNotificationPayload& Payload);
	void QueueNotification(const FNotificationPayload& Payload);
	void TryDisplayQueuedNotifications();
	void StartExpirationTimer(const FNotificationPayload& Payload);
	void HandleNotificationExpired(int32 NotificationId);
	void StopExpirationTimer(int32 NotificationId);
	FNotificationPayload BuildPayloadWithId(const FNotificationPayload& Payload);

	UPROPERTY(Transient)
	int32 MaxVisibleNotifications = 3;

	UPROPERTY(Transient)
	TObjectPtr<UNotificationWidget> RegisteredWidget;

	UPROPERTY(Transient)
	TArray<FNotificationPayload> ActiveNotifications;

	UPROPERTY(Transient)
	TArray<FNotificationPayload> QueuedNotifications;

	TMap<int32, FTimerHandle> ExpirationTimers;
	int32 NextNotificationId = 1;
};
