#include "P4Task.h"
#import <Foundation/Foundation.h>
#import <cocoa/cocoa.h>

bool P4Task::ExecuteTrustThisServerFingerprintDialogBox(const std::string& statusMessage)
{
    const char* c_string = statusMessage.c_str();
    NSString *string = [NSString stringWithUTF8String:c_string]; 
    NSLog(@"String:%@",string);

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:string];
    [alert setInformativeText:@"Informative text."];
    [alert addButtonWithTitle:@"Cancel"];
    [alert addButtonWithTitle:@"Ok"];
    [alert runModal];

    return false;
}
