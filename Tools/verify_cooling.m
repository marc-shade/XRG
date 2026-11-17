/* XRG cooling monitor validation. GPL-2.0-or-later; see LICENSE. */
#import <Cocoa/Cocoa.h>
#import "SMCInterface.h"
#import "XRGAppleSiliconSensorMiner.h"
#import "XRGCoolingPadStatus.h"
#include <math.h>

static FourCharCode keyCode(NSString *key) {
    const unsigned char *b = (const unsigned char *)key.UTF8String;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

int main(void) {
    @autoreleasepool {
        NSString *path = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Caches/padctl/status.json"];
        NSError *error = nil;
        NSData *data = [NSData dataWithContentsOfFile:path options:0 error:&error];
        NSTimeInterval now = NSDate.date.timeIntervalSince1970;
        NSDictionary *snapshot = data ? [XRGCoolingPadStatus decodeData:data now:now error:&error] : nil;
        if (!snapshot) { NSLog(@"Live pad feed failed: %@", error); return 1; }
        XRGCoolingPadStatus *pad = [[XRGCoolingPadStatus alloc] init];
        [pad refresh];
        if (!pad.commandedRPM) { NSLog(@"Live controller is not acknowledged: %@", pad.controllerState); return 1; }

        // Falsify malformed/stale inputs without touching the live status file.
        NSArray *mutations = @[@{@"updated_at":@(now - 1000)}, @{@"fan_commanded_rpm":@YES},
            @{@"fan_commanded_rpm":@4000}, @{@"schema_version":@2}, @{@"running":@1}];
        for (NSDictionary *mutation in mutations) {
            NSMutableDictionary *invalid = snapshot.mutableCopy;
            [invalid addEntriesFromDictionary:mutation];
            NSData *encoded = [NSJSONSerialization dataWithJSONObject:invalid options:0 error:&error];
            if ([XRGCoolingPadStatus decodeData:encoded now:now error:&error]) {
                NSLog(@"Invalid input accepted: %@", mutation); return 1;
            }
        }
        if ([XRGCoolingPadStatus decodeData:[@"{" dataUsingEncoding:NSUTF8StringEncoding] now:now error:&error]) return 1;

        SMCInterface *smc = [[SMCInterface alloc] init];
        NSNumber *count = [smc readValue:'FNum' error:&error];
        if (![count isKindOfClass:NSNumber.class] || count.integerValue < 1 || count.integerValue > 10) {
            NSLog(@"Fan count unavailable: %@", error); return 1;
        }
        NSMutableDictionary *fans = [NSMutableDictionary dictionary];
        for (NSInteger i = 0; i < count.integerValue; i++) {
            NSString *key = [NSString stringWithFormat:@"F%ldAc", (long)i];
            NSNumber *rpm = [smc readValue:keyCode(key) error:&error];
            if (![rpm isKindOfClass:NSNumber.class] || !isfinite(rpm.doubleValue) || rpm.doubleValue < 0 || rpm.doubleValue > 20000) {
                NSLog(@"Fan read failed for %@: %@", key, error); return 1;
            }
            fans[key] = rpm;
        }
        NSDictionary *sensors = XRGAppleSiliconSensorMiner.sensorData;
        double hottest = -INFINITY;
        for (NSString *key in sensors) {
            if ([key hasPrefix:@"PMU tdie"] || [key containsString:@"MTR Temp Sensor"]) hottest = MAX(hottest, [sensors[key] doubleValue]);
        }
        if (!isfinite(hottest)) { NSLog(@"No live die sensor"); return 1; }
        NSDictionary *result = @{@"mac_fans_measured_rpm":fans, @"mac_die_celsius":@(hottest),
            @"mac_die_fahrenheit":@(hottest * 1.8 + 32), @"pad_commanded_rpm":pad.commandedRPM,
            @"pad_state":pad.controllerState, @"invalid_input_checks":@6};
        NSData *output = [NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:&error];
        if (!output) { NSLog(@"%@", error); return 1; }
        fwrite(output.bytes, 1, output.length, stdout);
        puts("");
    }
    return 0;
}
