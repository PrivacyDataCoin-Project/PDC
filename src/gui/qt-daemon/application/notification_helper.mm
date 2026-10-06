#include <UserNotifications/UserNotifications.h>
#include "notification_helper.h"

void notification_helper::show(const std::string& title, const std::string& message)
{
  NSString *title_text = [NSString stringWithUTF8String:title.c_str()];
  NSString *message_text = [NSString stringWithUTF8String:message.c_str()];
  UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                         completionHandler:^(BOOL granted, NSError *error) {
    if (!granted || error != nil)
      return;

    UNMutableNotificationContent *content = [[[UNMutableNotificationContent alloc] init] autorelease];
    content.title = title_text;
    content.body = message_text;
    UNNotificationRequest *request = [UNNotificationRequest
        requestWithIdentifier:[[NSUUID UUID] UUIDString]
                      content:content
                      trigger:nil];
    [center addNotificationRequest:request withCompletionHandler:nil];
  }];
}
