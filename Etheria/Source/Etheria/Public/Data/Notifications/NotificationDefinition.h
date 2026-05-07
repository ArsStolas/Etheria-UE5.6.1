/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationDefinition - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Core/System/Notifications/NotificationTypes.h"
#include "Engine/DataAsset.h"
#include "NotificationDefinition.generated.h"

UCLASS(BlueprintType)
class ETHERIA_API UNotificationDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification", meta=(MultiLine="true"))
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	ENotificationType Type = ENotificationType::Info;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	ENotificationPriority Priority = ENotificationPriority::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification", meta=(ClampMin="0.0"))
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
	FGameplayTag NotificationTag;

	UFUNCTION(BlueprintPure, Category="Notification")
	FNotificationPayload MakePayload() const;
};
