#import "XRGAppDelegate+AIWindowIntegration.h"
#import "XRGAppDelegate.h"
#import "XRGGraphWindow.h"
#import "XRGAITokensView.h"
#import "XRGAISettingsKeys.h"

@implementation XRGAppDelegate (AIWindowIntegration)

- (void)xrg_setupAITokensIntegration {
    XRGGraphWindow *graphWindow = self.xrgGraphWindow;
    if (!graphWindow) {
        return;
    }
    
    XRGAITokensView *aiView = [[XRGAITokensView alloc] initWithFrame:NSMakeRect(0, 0, 90, 60)];
    [aiView configureWithGraphWindow:graphWindow];
    
    NSView *targetView = graphWindow.contentView;
    if (targetView) {
        [targetView addSubview:aiView];
    }
    
    [[NSNotificationCenter defaultCenter] addObserverForName:@"XRGShowAITokenGraphDidChange"
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification * _Nonnull note) {
        id moduleManager = graphWindow.moduleManager;
        if (moduleManager && [moduleManager respondsToSelector:@selector(setModule:isDisplayed:)] && [moduleManager respondsToSelector:@selector(redisplayModules)]) {
            BOOL shouldDisplay = [note.userInfo[@"display"] boolValue];
            [moduleManager setModule:@"XRGAITokensModule" isDisplayed:shouldDisplay];
            [moduleManager redisplayModules];
        }
    }];
}

@end
