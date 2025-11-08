#import "XRGAIPrefController.h"
#import "XRGAISettingsKeys.h"

@implementation XRGAIPrefController

- (void)viewDidLoad {
    [super viewDidLoad];

    NSDictionary *defaults = @{
        XRGDefaultsKeyAITokensTrackingEnabled: @NO,
        XRGDefaultsKeyShowAITokenGraph: @NO,
        XRGDefaultsKeyAITokensDailyAutoReset: @YES,
        XRGDefaultsKeyAITokensDailyBudget: @0,
        XRGDefaultsKeyAITokensBudgetNotifyPercent: @80,
        XRGDefaultsKeyAITokensAggregateByModel: @YES,
        XRGDefaultsKeyAITokensAggregateByProvider: @NO,
        XRGDefaultsKeyAITokensShowRate: @YES,
        XRGDefaultsKeyAITokensShowBreakdown: @YES
    };

    [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
}

- (IBAction)resetSessionCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:@"XRGAITokensDidResetSession" object:self];
}

- (IBAction)resetDailyCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:@"XRGAITokensDidResetDaily" object:self];
}

@end
