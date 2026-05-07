/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationBlueprintLibrary - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Core/System/Notifications/NotificationTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NotificationBlueprintLibrary.generated.h"

class UNotificationDefinition;
class UNotificationSubsystem;

UCLASS()
class ETHERIA_API UNotificationBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static UNotificationSubsystem* GetNotificationSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static int32 ShowNotification(const UObject* WorldContextObject, const FNotificationPayload& Payload);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static int32 ShowNotificationFromDefinition(const UObject* WorldContextObject, const UNotificationDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static int32 ShowInfoNotification(const UObject* WorldContextObject, const FText& Title, const FText& Message, float Duration = 3.0f);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static int32 ShowWarningNotification(const UObject* WorldContextObject, const FText& Title, const FText& Message, float Duration = 4.0f);

	UFUNCTION(BlueprintCallable, Category="Notifications", meta=(WorldContext="WorldContextObject"))
	static void ClearAllNotifications(const UObject* WorldContextObject);
};
