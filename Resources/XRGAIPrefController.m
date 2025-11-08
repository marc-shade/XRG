#import "XRGAIPrefController.h"
#import "XRGAISettingsKeys.h"
#import "XRGAITokensNotifications.h"
#import "XRGSettings.h"

@implementation XRGAIPrefController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Register default values for AI Token settings
    NSDictionary *defaults = @{
        XRGDefaultsKeyAITokensTrackingEnabled: @YES,  // Enable tracking by default
        XRGDefaultsKeyShowAITokenGraph: @NO,
        XRGDefaultsKeyAITokensDailyAutoReset: @YES,
        XRGDefaultsKeyAITokensDailyBudget: @0,
        XRGDefaultsKeyAITokensBudgetNotifyPercent: @80,
        XRGDefaultsKeyAITokensAggregateByModel: @YES,
        XRGDefaultsKeyAITokensAggregateByProvider: @YES,  // Enable provider tracking by default
        XRGDefaultsKeyAITokensShowRate: @YES,
        XRGDefaultsKeyAITokensShowBreakdown: @YES
    };

    [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
}

- (IBAction)resetSessionCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:XRGAITokensDidResetSessionNotification object:self];
}

- (IBAction)resetDailyCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:XRGAITokensDidResetDailyNotification object:self];
}

@end
