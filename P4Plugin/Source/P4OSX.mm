#include "P4Task.h"
#import <AppKit/AppKit.h>

bool P4Task::ShowOKCancelDialogBox(const std::string& windowTitle, const std::string& message)
{
    NSString *nsWindowTitle = [NSString stringWithUTF8String:windowTitle.c_str()];
    NSString *nsMessage = [NSString stringWithUTF8String:message.c_str()];

    CFOptionFlags cfRes;
    CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel,
            NULL, NULL, NULL,
            (__bridge CFStringRef)nsWindowTitle,
            (__bridge CFStringRef)nsMessage,
            CFSTR("Cancel"),
            CFSTR("Ok"),
            NULL,
            &cfRes);

    return cfRes == kCFUserNotificationAlternateResponse;
}
