#include "P4Task.h"
#import <AppKit/AppKit.h>

bool P4Task::ExecuteTrustThisServerFingerprintDialogBox(const std::string& message)
{
    NSString *nsWindowTitle = @"Test Window Title";
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
