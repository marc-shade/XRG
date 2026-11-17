/* XRG cooling monitor. Licensed under GPL-2.0-or-later; see LICENSE. */
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface XRGCoolingPadStatus : NSObject
@property (readonly) BOOL configured;
@property (readonly, copy) NSString *controllerState;
@property (readonly, nullable) NSNumber *commandedRPM;
- (void)refresh;
+ (nullable NSDictionary *)decodeData:(NSData *)data now:(NSTimeInterval)now error:(NSError **)error;
@end

NS_ASSUME_NONNULL_END
