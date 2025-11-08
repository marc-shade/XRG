#import "XRGAITokensObserver.h"
#import "XRGAISettingsKeys.h"
#import "XRGSettings.h"
#import "definitions.h"

@interface XRGAITokensObserver ()

@property (nonatomic, strong) dispatch_queue_t serialQueue;

@property (atomic, assign) NSUInteger sessionPromptTokens;
@property (atomic, assign) NSUInteger sessionCompletionTokens;

@property (nonatomic, assign) NSUInteger dailyPromptTokens;
@property (nonatomic, assign) NSUInteger dailyCompletionTokens;

@property (nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *dailyModelPromptTokens;
@property (nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *dailyModelCompletionTokens;

@property (nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *dailyProviderPromptTokens;
@property (nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *dailyProviderCompletionTokens;

@property (nonatomic, strong) NSDate *lastNotifyDate;

@end

@implementation XRGAITokensObserver

+ (instancetype)shared {
    static XRGAITokensObserver *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[XRGAITokensObserver alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _serialQueue = dispatch_queue_create("com.xrga.tokensObserver.queue", DISPATCH_QUEUE_SERIAL);
        
        _dailyModelPromptTokens = [NSMutableDictionary dictionary];
        _dailyModelCompletionTokens = [NSMutableDictionary dictionary];
        _dailyProviderPromptTokens = [NSMutableDictionary dictionary];
        _dailyProviderCompletionTokens = [NSMutableDictionary dictionary];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(resetSession)
                                                     name:XRGAITokensDidResetSessionNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(resetDaily)
                                                     name:XRGAITokensDidResetDailyNotification
                                                   object:nil];
        
        [self loadDailyCounters];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Public

- (void)recordEventWithPromptTokens:(NSUInteger)promptTokens
                     completionTokens:(NSUInteger)completionTokens
                              model:(NSString *)model
                           provider:(NSString *)provider {
    
    if (![self aiTokensTrackingEnabled]) {
        return;
    }
    
    dispatch_async(self.serialQueue, ^{
        [self checkAndResetDailyIfNeeded];
        
        // Update session counters
        self.sessionPromptTokens += promptTokens;
        self.sessionCompletionTokens += completionTokens;
        
        // Update daily counters
        self.dailyPromptTokens += promptTokens;
        self.dailyCompletionTokens += completionTokens;
        
        XRGSettings *settings = [XRGSettings sharedSettings];
        
        if (settings.aiTokensAggregateByModel) {
            if (model.length) {
                NSNumber *prevPrompt = self.dailyModelPromptTokens[model] ?: @0;
                NSNumber *prevCompletion = self.dailyModelCompletionTokens[model] ?: @0;
                self.dailyModelPromptTokens[model] = @(prevPrompt.unsignedIntegerValue + promptTokens);
                self.dailyModelCompletionTokens[model] = @(prevCompletion.unsignedIntegerValue + completionTokens);
            }
        }
        
        if (settings.aiTokensAggregateByProvider) {
            if (provider.length) {
                NSNumber *prevPrompt = self.dailyProviderPromptTokens[provider] ?: @0;
                NSNumber *prevCompletion = self.dailyProviderCompletionTokens[provider] ?: @0;
                self.dailyProviderPromptTokens[provider] = @(prevPrompt.unsignedIntegerValue + promptTokens);
                self.dailyProviderCompletionTokens[provider] = @(prevCompletion.unsignedIntegerValue + completionTokens);
            }
        }
        
        [self persistDailyCounters];
        
        [self checkBudgetThresholdAndNotifyIfNeeded];
    });
}

- (void)resetSession {
    dispatch_async(self.serialQueue, ^{
        self.sessionPromptTokens = 0;
        self.sessionCompletionTokens = 0;
    });
}

- (void)resetDaily {
    dispatch_async(self.serialQueue, ^{
        self.dailyPromptTokens = 0;
        self.dailyCompletionTokens = 0;
        
        [self.dailyModelPromptTokens removeAllObjects];
        [self.dailyModelCompletionTokens removeAllObjects];
        [self.dailyProviderPromptTokens removeAllObjects];
        [self.dailyProviderCompletionTokens removeAllObjects];
        
        self.lastNotifyDate = nil;

        [self saveTodayDateToDefaults];
        [self persistDailyCounters];
    });
}

#pragma mark - Property Getters

- (NSUInteger)sessionTotalTokens {
    return self.sessionPromptTokens + self.sessionCompletionTokens;
}

- (NSUInteger)dailyTotalTokens {
    return self.dailyPromptTokens + self.dailyCompletionTokens;
}

- (NSDictionary<NSString *, NSNumber *> *)dailyByModel {
    NSMutableDictionary *totals = [NSMutableDictionary dictionary];

    // Combine prompt and completion tokens for each model
    for (NSString *model in self.dailyModelPromptTokens) {
        NSUInteger promptTokens = [self.dailyModelPromptTokens[model] unsignedIntegerValue];
        NSUInteger completionTokens = [self.dailyModelCompletionTokens[model] unsignedIntegerValue];
        totals[model] = @(promptTokens + completionTokens);
    }

    // Add any models that only have completion tokens
    for (NSString *model in self.dailyModelCompletionTokens) {
        if (!totals[model]) {
            totals[model] = self.dailyModelCompletionTokens[model];
        }
    }

    return [totals copy];
}

- (NSDictionary<NSString *, NSNumber *> *)dailyByProvider {
    NSMutableDictionary *totals = [NSMutableDictionary dictionary];

    // Combine prompt and completion tokens for each provider
    for (NSString *provider in self.dailyProviderPromptTokens) {
        NSUInteger promptTokens = [self.dailyProviderPromptTokens[provider] unsignedIntegerValue];
        NSUInteger completionTokens = [self.dailyProviderCompletionTokens[provider] unsignedIntegerValue];
        totals[provider] = @(promptTokens + completionTokens);
    }

    // Add any providers that only have completion tokens
    for (NSString *provider in self.dailyProviderCompletionTokens) {
        if (!totals[provider]) {
            totals[provider] = self.dailyProviderCompletionTokens[provider];
        }
    }

    return [totals copy];
}

#pragma mark - Private

- (BOOL)aiTokensTrackingEnabled {
    return [XRGSettings sharedSettings].aiTokensTrackingEnabled;
}

- (void)checkAndResetDailyIfNeeded {
    XRGSettings *settings = [XRGSettings sharedSettings];
    if (!settings.aiTokensDailyAutoReset) {
        return;
    }
    
    NSDate *storedDate = [self storedDailyDate];
    NSDate *today = [self todayDate];
    if (![self isDate:storedDate sameDayAsDate:today]) {
        [self resetDailySync];
    }
}

- (void)resetDailySync {
    self.dailyPromptTokens = 0;
    self.dailyCompletionTokens = 0;
    
    [self.dailyModelPromptTokens removeAllObjects];
    [self.dailyModelCompletionTokens removeAllObjects];
    [self.dailyProviderPromptTokens removeAllObjects];
    [self.dailyProviderCompletionTokens removeAllObjects];
    
    self.lastNotifyDate = nil;
    
    [self saveTodayDateToDefaults];
    [self persistDailyCounters];
}

- (void)persistDailyCounters {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setInteger:self.dailyPromptTokens forKey:XRGAITokensDailyPromptTokensKey];
    [defaults setInteger:self.dailyCompletionTokens forKey:XRGAITokensDailyCompletionTokensKey];
    [defaults setObject:[self.dailyModelPromptTokens copy] forKey:XRGAITokensDailyModelPromptTokensKey];
    [defaults setObject:[self.dailyModelCompletionTokens copy] forKey:XRGAITokensDailyModelCompletionTokensKey];
    [defaults setObject:[self.dailyProviderPromptTokens copy] forKey:XRGAITokensDailyProviderPromptTokensKey];
    [defaults setObject:[self.dailyProviderCompletionTokens copy] forKey:XRGAITokensDailyProviderCompletionTokensKey];
    
    if (self.lastNotifyDate) {
        [defaults setObject:self.lastNotifyDate forKey:XRGAITokensLastNotifyDateKey];
    } else {
        [defaults removeObjectForKey:XRGAITokensLastNotifyDateKey];
    }
    [defaults synchronize];
}

- (void)loadDailyCounters {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    _dailyPromptTokens = (NSUInteger)[defaults integerForKey:XRGAITokensDailyPromptTokensKey];
    _dailyCompletionTokens = (NSUInteger)[defaults integerForKey:XRGAITokensDailyCompletionTokensKey];
    
    NSDictionary *modelPrompt = [defaults dictionaryForKey:XRGAITokensDailyModelPromptTokensKey];
    NSDictionary *modelCompletion = [defaults dictionaryForKey:XRGAITokensDailyModelCompletionTokensKey];
    NSDictionary *providerPrompt = [defaults dictionaryForKey:XRGAITokensDailyProviderPromptTokensKey];
    NSDictionary *providerCompletion = [defaults dictionaryForKey:XRGAITokensDailyProviderCompletionTokensKey];
    
    if (modelPrompt) {
        _dailyModelPromptTokens = [modelPrompt mutableCopy];
    }
    if (modelCompletion) {
        _dailyModelCompletionTokens = [modelCompletion mutableCopy];
    }
    if (providerPrompt) {
        _dailyProviderPromptTokens = [providerPrompt mutableCopy];
    }
    if (providerCompletion) {
        _dailyProviderCompletionTokens = [providerCompletion mutableCopy];
    }
    
    NSDate *notifyDate = [defaults objectForKey:XRGAITokensLastNotifyDateKey];
    if ([notifyDate isKindOfClass:[NSDate class]]) {
        _lastNotifyDate = notifyDate;
    }
}

- (NSDate *)storedDailyDate {
    NSDate *date = [[NSUserDefaults standardUserDefaults] objectForKey:XRGAITokensDailyDateKey];
    if ([date isKindOfClass:[NSDate class]]) {
        return date;
    }
    return nil;
}

- (void)saveTodayDateToDefaults {
    NSDate *today = [self todayDate];
    [[NSUserDefaults standardUserDefaults] setObject:today forKey:XRGAITokensDailyDateKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSDate *)todayDate {
    NSDate *now = [NSDate date];
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDateComponents *components = [calendar components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay fromDate:now];
    return [calendar dateFromComponents:components];
}

- (BOOL)isDate:(NSDate *)date1 sameDayAsDate:(NSDate *)date2 {
    if (!date1 || !date2) {
        return NO;
    }
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDateComponents *comp1 = [calendar components:NSCalendarUnitYear|NSCalendarUnitMonth|NSCalendarUnitDay fromDate:date1];
    NSDateComponents *comp2 = [calendar components:NSCalendarUnitYear|NSCalendarUnitMonth|NSCalendarUnitDay fromDate:date2];
    return (comp1.year == comp2.year && comp1.month == comp2.month && comp1.day == comp2.day);
}

- (void)checkBudgetThresholdAndNotifyIfNeeded {
    XRGSettings *settings = [XRGSettings sharedSettings];
    if (settings.aiTokensDailyBudget <= 0) {
        return;
    }

    NSUInteger totalTokens = self.dailyPromptTokens + self.dailyCompletionTokens;
    NSUInteger budget = settings.aiTokensDailyBudget;
    CGFloat progress = (CGFloat)totalTokens / (CGFloat)budget;
    CGFloat progressPercent = progress * 100.0;

    NSInteger notifyPercent = settings.aiTokensBudgetNotifyPercent;
    if (notifyPercent <= 0) {
        notifyPercent = 80; // Default to 80% if not set
    }

    NSDate *today = [self todayDate];

    if ([self.lastNotifyDate isKindOfClass:[NSDate class]] && [self isDate:self.lastNotifyDate sameDayAsDate:today]) {
        return; // Already notified today
    }

    // Check if we've reached the notification threshold
    if (progressPercent >= notifyPercent) {
        self.lastNotifyDate = today;
        [self persistDailyCounters];
        dispatch_async(dispatch_get_main_queue(), ^{
            [[NSNotificationCenter defaultCenter] postNotificationName:XRGAITokensBudgetThresholdReachedNotification
                                                                object:nil
                                                              userInfo:@{@"progress": @(progress), @"percent": @(progressPercent)}];
        });
    }
}

@end
