#import <Foundation/Foundation.h>
#import "XRGAISettingsKeys.h"
#import "XRGAITokensNotifications.h"

NS_ASSUME_NONNULL_BEGIN

@interface XRGAITokensObserver : NSObject

+ (instancetype)shared;

@property (atomic, readonly) NSUInteger sessionPromptTokens;
@property (atomic, readonly) NSUInteger sessionCompletionTokens;
@property (atomic, readonly) NSUInteger sessionTotalTokens;
@property (atomic, readonly) NSUInteger dailyTotalTokens;
@property (atomic, readonly) NSDictionary<NSString *, NSNumber *> *dailyByModel;
@property (atomic, readonly) NSDictionary<NSString *, NSNumber *> *dailyByProvider;

- (void)recordEventWithPromptTokens:(NSUInteger)prompt
                   completionTokens:(NSUInteger)completion
                             model:(nullable NSString *)model
                          provider:(nullable NSString *)provider;

- (void)resetSession;
- (void)resetDaily;

@end

NS_ASSUME_NONNULL_END
