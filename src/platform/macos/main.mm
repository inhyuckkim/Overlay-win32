#import <Cocoa/Cocoa.h>
#include <ixwebsocket/IXNetSystem.h>

#include "app.h"

@interface LixorAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation LixorAppDelegate

- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    App::instance().shutdown();
}

@end

int main(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        ix::initNetSystem();

        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];

        LixorAppDelegate* delegate = [[LixorAppDelegate alloc] init];
        [app setDelegate:delegate];

        if (!App::instance().init()) {
            ix::uninitNetSystem();
            return 1;
        }

        [app activateIgnoringOtherApps:YES];
        [app run];

        ix::uninitNetSystem();
    }
    return 0;
}
