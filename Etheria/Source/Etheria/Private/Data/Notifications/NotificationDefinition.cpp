/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: NotificationDefinition - Source
*/

#include "Data/Notifications/NotificationDefinition.h"

FNotificationPayload UNotificationDefinition::MakePayload() const
{
	FNotificationPayload Payload;
	Payload.Title = Title;
	Payload.Message = Message;
	Payload.Type = Type;
	Payload.Priority = Priority;
	Payload.Duration = Duration;
	Payload.Icon = Icon;
	Payload.Sound = Sound;
	Payload.NotificationTag = NotificationTag;
	return Payload;
}
