/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationTypes - Header
 * Note: Shared data used by the modular notification system
*/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NotificationTypes.generated.h"

class USoundBase;
class UTexture2D;

DECLARE_LOG_CATEGORY_EXTERN(LogNotificationSystem, Log, All);

UENUM(BlueprintType)
enum class ENotificationType : uint8
{
	Info     UMETA(DisplayName="Info"),
	Success  UMETA(DisplayName="Success"),
	Warning  UMETA(DisplayName="Warning"),
	Error    UMETA(DisplayName="Error"),
	Quest    UMETA(DisplayName="Quest"),
	Item     UMETA(DisplayName="Item"),
	Tutorial UMETA(DisplayName="Tutorial"),
};

UENUM(BlueprintType)
enum class ENotificationPriority : uint8
{
	Low      UMETA(DisplayName="Low"),
	Normal   UMETA(DisplayName="Normal"),
	High     UMETA(DisplayName="High"),
	Critical UMETA(DisplayName="Critical"),
};

USTRUCT(BlueprintType)
struct FNotificationPayload
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Notification")
	int32 NotificationId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification", meta=(MultiLine="true"))
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	ENotificationType Type = ENotificationType::Info;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	ENotificationPriority Priority = ENotificationPriority::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification", meta=(ClampMin="0.0"))
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notification")
	FGameplayTag NotificationTag;

	bool IsValid() const
	{
		return !Title.IsEmpty() || !Message.IsEmpty();
	}
};
