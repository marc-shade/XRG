#ifndef XRGAISettingsKeys_h
#define XRGAISettingsKeys_h

#import <Foundation/Foundation.h>

#define XRGDefaultsKeyShowAITokenGraph @"showAITokenGraph"
#define XRGDefaultsKeyAITokensTrackingEnabled @"aiTokensTrackingEnabled"
#define XRGDefaultsKeyAITokensDailyAutoReset @"aiTokensDailyAutoReset"
#define XRGDefaultsKeyAITokensDailyBudget @"aiTokensDailyBudget"
#define XRGDefaultsKeyAITokensBudgetNotifyPercent @"aiTokensBudgetNotifyPercent"
#define XRGDefaultsKeyAITokensAggregateByModel @"aiTokensAggregateByModel"
#define XRGDefaultsKeyAITokensAggregateByProvider @"aiTokensAggregateByProvider"
#define XRGDefaultsKeyAITokensShowRate @"aiTokensShowRate"
#define XRGDefaultsKeyAITokensShowBreakdown @"aiTokensShowBreakdown"

// Account type (subscription vs API billing)
// 0 = Subscription/Max (show tokens only, no cost)
// 1 = API billing (show cost calculations)
#define XRGDefaultsKeyAITokensAccountType @"aiTokensAccountType"
#define XRGDefaultsKeyAITokensMonthlyAllowance @"aiTokensMonthlyAllowance"  // For subscription: tokens/month

// Cyberpunk visual effects
#define XRGDefaultsKeyShowScanlines @"showScanlines"
#define XRGDefaultsKeyShowPixelGrid @"showPixelGrid"
#define XRGDefaultsKeyShowPixelDots @"showPixelDots"

#endif /* XRGAISettingsKeys_h */
