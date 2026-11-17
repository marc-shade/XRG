/* XRG cooling monitor. Licensed under GPL-2.0-or-later; see LICENSE. */
#import "XRGCoolingPadStatus.h"
#include <math.h>

@interface XRGCoolingPadStatus ()
@property BOOL configured;
@property (copy) NSString *controllerState;
@property (nullable) NSNumber *commandedRPM;
@end

static BOOL XRGRealNumber(id value) {
    return [value isKindOfClass:NSNumber.class] &&
        CFGetTypeID((__bridge CFTypeRef)value) != CFBooleanGetTypeID() && isfinite([value doubleValue]);
}

static BOOL XRGBool(id value) {
    return [value isKindOfClass:NSNumber.class] &&
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID();
}

@implementation XRGCoolingPadStatus

- (instancetype)init {
    if ((self = [super init])) _controllerState = @"Not configured";
    return self;
}

+ (NSDictionary *)decodeData:(NSData *)data now:(NSTimeInterval)now error:(NSError **)error {
    NSString *problem = nil;
    id json = data.length <= 65536 ? [NSJSONSerialization JSONObjectWithData:data options:0 error:error] : nil;
    if (![json isKindOfClass:NSDictionary.class]) {
        problem = @"Invalid status";
    } else if (!XRGRealNumber(json[@"schema_version"]) || [json[@"schema_version"] doubleValue] != 1) {
        problem = @"Unsupported status version";
    } else if (!XRGRealNumber(json[@"updated_at"]) || !XRGRealNumber(json[@"valid_for_seconds"])) {
        problem = @"Missing freshness information";
    } else {
        double age = now - [json[@"updated_at"] doubleValue];
        double lifetime = [json[@"valid_for_seconds"] doubleValue];
        id rpm = json[@"fan_commanded_rpm"];
        if (lifetime < 1 || lifetime > 900 || age < -5 || age > lifetime) {
            problem = @"Stale";
        } else if (!XRGBool(json[@"running"]) || !XRGBool(json[@"connected"])) {
            problem = @"Invalid controller state";
        } else if (rpm != NSNull.null && (!XRGRealNumber(rpm) ||
                   ([rpm doubleValue] != 0 && ([rpm doubleValue] < 500 || [rpm doubleValue] > 3200)) ||
                   fmod([rpm doubleValue], 50) != 0)) {
            problem = @"Invalid fan command";
        }
    }
    if (problem) {
        if (error) *error = [NSError errorWithDomain:@"XRGCoolingPadStatus" code:1
            userInfo:@{NSLocalizedDescriptionKey:problem}];
        return nil;
    }
    return json;
}

- (void)refresh {
    self.commandedRPM = nil;
    NSString *path = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Caches/padctl/status.json"];
    NSError *error = nil;
    NSDictionary *attributes = [NSFileManager.defaultManager attributesOfItemAtPath:path error:&error];
    if (!attributes) {
        self.controllerState = [error.domain isEqualToString:NSCocoaErrorDomain] && error.code == NSFileReadNoSuchFileError
            ? @"Not running" : @"Status unreadable";
        return;
    }
    self.configured = YES;
    if ([attributes[NSFileSize] unsignedLongLongValue] > 65536) {
        self.controllerState = @"Invalid status";
        return;
    }
    NSData *data = [NSData dataWithContentsOfFile:path options:0 error:&error];
    NSDictionary *json = data ? [XRGCoolingPadStatus decodeData:data now:NSDate.date.timeIntervalSince1970 error:&error] : nil;
    if (!json) {
        self.controllerState = error.localizedDescription ?: @"Status unavailable";
        return;
    }
    if (![json[@"running"] boolValue]) { self.controllerState = @"Stopped"; return; }
    if (![json[@"connected"] boolValue]) { self.controllerState = @"Disconnected"; return; }
    id rpm = json[@"fan_commanded_rpm"];
    self.commandedRPM = XRGRealNumber(rpm) ? rpm : nil;
    self.controllerState = [json[@"error"] isKindOfClass:NSString.class] ? @"Failsafe" : @"Automatic";
    if (!self.commandedRPM) self.controllerState = @"Awaiting acknowledgement";
}
@end
