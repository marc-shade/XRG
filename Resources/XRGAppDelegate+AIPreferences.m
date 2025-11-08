#import "XRGAppDelegate+AIPreferences.h"
#import "XRGAIPrefController.h"

@implementation XRGAppDelegate (AIPreferences)

- (void)openAIPreferences:(id)sender {
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 480, 320)
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskUtilityWindow)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    XRGAIPrefController *prefController = [[XRGAIPrefController alloc] init];
    window.contentViewController = prefController;
    window.title = @"AI Tokens";
    [window center];
    [window makeKeyAndOrderFront:nil];
}

@end
