/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationWidget - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/System/Notifications/NotificationTypes.h"
#include "NotificationWidget.generated.h"

UCLASS()
class ETHERIA_API UNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category="Notifications")
	void OnNotificationReceived(const FNotificationPayload& Payload);

	UFUNCTION(BlueprintImplementableEvent, Category="Notifications")
	void OnNotificationRemoved(int32 NotificationId);

	UFUNCTION(BlueprintImplementableEvent, Category="Notifications")
	void OnNotificationsCleared();
};
