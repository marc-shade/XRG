/*
 * XRG (X Resource Graph):  A system resource grapher for Mac OS X.
 * Copyright (C) 2002-2022 Gaucho Software, LLC.
 * You can view the complete license in the LICENSE file in the root
 * of the source tree.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

//
//  XRGAITokenMiner.m
//  Universal AI Token monitoring implementation
//

#import "XRGAITokenMiner.h"
#import "XRGAITokensObserver.h"
#import "definitions.h"
#import <sqlite3.h>

@interface XRGAITokenMiner ()
- (NSData *)xrg_syncDataForRequest:(NSURLRequest *)request returningResponse:(NSHTTPURLResponse * _Nullable __autoreleasing *)response error:(NSError * _Nullable __autoreleasing *)error;
@end

@implementation XRGAITokenMiner

@synthesize totalClaudeTokens = _totalClaudeTokens;
@synthesize totalCodexTokens = _totalCodexTokens;
@synthesize totalGeminiTokens = _totalGeminiTokens;
@synthesize totalCostUSD = _totalCostUSD;
@synthesize activeStrategy;

- (instancetype)init {
    self = [super init];
    if (self) {
        // Initialize data sets
        claudeCodeTokens = [[XRGDataSet alloc] init];
        codexTokens = [[XRGDataSet alloc] init];
        geminiTokens = [[XRGDataSet alloc] init];

        // Initialize counters
        _totalClaudeTokens = 0;
        _totalCodexTokens = 0;
        _totalGeminiTokens = 0;
        _totalCostUSD = 0.0;
        lastClaudeCount = 0;
        lastCodexCount = 0;
        lastGeminiCount = 0;

        // Initialize current rates
        currentClaudeRate = 0;
        currentCodexRate = 0;
        currentGeminiRate = 0;

        // Initialize paths
        NSString *homeDir = NSHomeDirectory();
        jsonlProjectsPath = [homeDir stringByAppendingPathComponent:@".claude/projects"];
        codexSessionsPath = [homeDir stringByAppendingPathComponent:@".codex/sessions"];
        geminiTmpPath = [homeDir stringByAppendingPathComponent:@".gemini/tmp"];
        dbPath = [homeDir stringByAppendingPathComponent:@".claude/monitoring/claude_usage.db"];
        otelEndpoint = @"http://localhost:8889/metrics";

        // Initialize caches for performance
        jsonlFileModTimes = [[NSMutableDictionary alloc] init];
        cachedJSONLTokens = 0;
        cachedCodexTokens = 0;
        cachedGeminiTokens = 0;
        lastJSONLScanTime = nil;
        lastCodexScanTime = nil;
        lastGeminiScanTime = nil;

        // Create background queue for non-blocking file operations
        jsonlParsingQueue = dispatch_queue_create("com.xrg.jsonl.parsing", DISPATCH_QUEUE_SERIAL);
        cacheSemaphore = dispatch_semaphore_create(1);

        // Detect best data collection strategy (for Claude)
        [self detectBestStrategy];

        // DON'T call getLatestTokenInfo during init - it could block!
        // Instead, start with zeros and let the first graphUpdate call handle it

        // Trigger immediate background cache build for all providers
        dispatch_async(jsonlParsingQueue, ^{
            if (activeStrategy == XRGAIDataStrategyJSONL) {
                [self updateJSONLCacheInBackground];
            }
            [self updateCodexCacheInBackground];
            [self updateGeminiCacheInBackground];
        });

#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Initialized with strategy: %@", [self strategyName]);
#endif
    }

    return self;
}

- (void)setDataSize:(int)newNumSamples {
    if (newNumSamples < 0) return;

    if (claudeCodeTokens && codexTokens && geminiTokens) {
        [claudeCodeTokens resize:(size_t)newNumSamples];
        [codexTokens resize:(size_t)newNumSamples];
        [geminiTokens resize:(size_t)newNumSamples];
    }
    else {
        claudeCodeTokens = [[XRGDataSet alloc] init];
        codexTokens = [[XRGDataSet alloc] init];
        geminiTokens = [[XRGDataSet alloc] init];

        [claudeCodeTokens resize:(size_t)newNumSamples];
        [codexTokens resize:(size_t)newNumSamples];
        [geminiTokens resize:(size_t)newNumSamples];
    }

    numSamples = newNumSamples;
}

- (void)reset {
    [claudeCodeTokens reset];
    [codexTokens reset];
    [geminiTokens reset];

    _totalClaudeTokens = 0;
    _totalCodexTokens = 0;
    _totalGeminiTokens = 0;
    _totalCostUSD = 0.0;
    lastClaudeCount = 0;
    lastCodexCount = 0;
    lastGeminiCount = 0;

    currentClaudeRate = 0;
    currentCodexRate = 0;
    currentGeminiRate = 0;
}

- (void)detectBestStrategy {
    // Try strategies in priority order until one works
    // NOTE: This method should be fast and non-blocking!

    // Strategy 1: JSONL transcripts (universal - works for EVERYONE with default Claude Code)
    NSFileManager *fm = [NSFileManager defaultManager];
    if ([fm fileExistsAtPath:jsonlProjectsPath]) {
        activeStrategy = XRGAIDataStrategyJSONL;
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Using JSONL strategy: %@", jsonlProjectsPath);
#endif
        return;
    }

    // Strategy 2: SQLite database (advanced - custom monitoring setup)
    if ([fm fileExistsAtPath:dbPath]) {
        activeStrategy = XRGAIDataStrategySQLite;
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Using SQLite strategy: %@", dbPath);
#endif
        return;
    }

    // Strategy 3: OpenTelemetry endpoint (advanced - OTel configured)
    // SKIP network check during initialization - too slow!
    // Will be attempted on first getLatestTokenInfo() call if needed
    
    // No file-based strategy available - default to None
    // (OTel will be tried later if needed)
    activeStrategy = XRGAIDataStrategyNone;
#ifdef XRG_DEBUG
    NSLog(@"[XRGAITokenMiner] No file-based strategy found, will try OTel on first update");
#endif
}

- (NSString *)strategyName {
    switch (activeStrategy) {
        case XRGAIDataStrategyJSONL:
            return @"JSONL Transcripts (Universal - Default Claude Code)";
        case XRGAIDataStrategySQLite:
            return @"SQLite Database (Advanced - Custom Monitoring)";
        case XRGAIDataStrategyOTel:
            return @"OpenTelemetry Endpoint (Advanced - OTel Configured)";
        default:
            return @"None (No data available)";
    }
}

- (void)getLatestTokenInfo {
    BOOL success = NO;

    // === Fetch Claude Code tokens ===
    switch (activeStrategy) {
        case XRGAIDataStrategyJSONL:
            success = [self fetchFromJSONLTranscripts];
            break;
        case XRGAIDataStrategySQLite:
            success = [self fetchFromSQLiteDatabase];
            break;
        case XRGAIDataStrategyOTel: {
            success = [self fetchFromOTelEndpoint];
            break;
        }
        default:
            break;
    }

    // If active strategy failed, try to detect a new strategy
    if (!success && activeStrategy != XRGAIDataStrategyNone) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Active strategy failed, re-detecting...");
#endif
        [self detectBestStrategy];
    }

    // === Fetch Codex CLI tokens ===
    [self fetchFromCodexCLI];

    // === Fetch Gemini CLI tokens ===
    [self fetchFromGeminiCLI];

    // Calculate rates (delta from last update) - CRITICAL: Calculate BEFORE updating lastCount
    currentClaudeRate = (UInt32)(_totalClaudeTokens - lastClaudeCount);
    currentCodexRate = (UInt32)(_totalCodexTokens - lastCodexCount);
    currentGeminiRate = (UInt32)(_totalGeminiTokens - lastGeminiCount);

    // Update last counts
    lastClaudeCount = _totalClaudeTokens;
    lastCodexCount = _totalCodexTokens;
    lastGeminiCount = _totalGeminiTokens;

    // Store rates in data sets
    if (claudeCodeTokens) [claudeCodeTokens setNextValue:currentClaudeRate];
    if (codexTokens) [codexTokens setNextValue:currentCodexRate];
    if (geminiTokens) [geminiTokens setNextValue:currentGeminiRate];
}

#pragma mark - Strategy 1: SQLite Database (Universal)

- (BOOL)fetchFromSQLiteDatabase {
    sqlite3 *db = NULL;
    int rc = sqlite3_open([dbPath UTF8String], &db);

    if (rc != SQLITE_OK) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Failed to open database: %s", sqlite3_errmsg(db));
#endif
        sqlite3_close(db);
        return NO;
    }

    // Query for total tokens and cost
    const char *sql = "SELECT SUM(tokens_used) as total_tokens, SUM(estimated_cost) as total_cost FROM sessions WHERE tokens_used > 0";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Failed to prepare statement: %s", sqlite3_errmsg(db));
#endif
        sqlite3_close(db);
        return NO;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        _totalClaudeTokens = (UInt64)sqlite3_column_int64(stmt, 0);
        _totalCostUSD = sqlite3_column_double(stmt, 1);

#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] SQLite: tokens=%llu, cost=$%.4f", _totalClaudeTokens, _totalCostUSD);
#endif
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return YES;
}

#pragma mark - Strategy 2: OpenTelemetry Endpoint (Advanced)

- (BOOL)fetchFromOTelEndpoint {
    NSURL *url = [NSURL URLWithString:otelEndpoint];
    if (!url) return NO;

    NSURLRequest *request = [NSURLRequest requestWithURL:url
                                             cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                         timeoutInterval:2.0];

    NSError *error = nil;
    NSHTTPURLResponse *response = nil;
    NSData *data = [self xrg_syncDataForRequest:request returningResponse:&response error:&error];

    if (error || !data || response.statusCode != 200) {
#ifdef XRG_DEBUG
        if (error) NSLog(@"[XRGAITokenMiner] OTel error: %@", error);
#endif
        return NO;
    }

    NSString *metricsText = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (!metricsText) return NO;

    // Parse Prometheus format metrics
    [self parsePrometheusMetrics:metricsText];
    return YES;
}

- (void)parsePrometheusMetrics:(NSString *)metricsText {
    NSArray *lines = [metricsText componentsSeparatedByString:@"\n"];

    UInt64 inputTokens = 0;
    UInt64 outputTokens = 0;
    UInt64 cacheReadTokens = 0;
    UInt64 cacheCreateTokens = 0;
    double costUSD = 0.0;

    for (NSString *line in lines) {
        if ([line hasPrefix:@"#"] || [line length] == 0) continue;

        if ([line containsString:@"token_usage"] || [line containsString:@"tokens"]) {
            if ([line containsString:@"input"]) {
                inputTokens += [self extractValueFromMetricLine:line];
            }
            else if ([line containsString:@"output"]) {
                outputTokens += [self extractValueFromMetricLine:line];
            }
            else if ([line containsString:@"cache_read"]) {
                cacheReadTokens += [self extractValueFromMetricLine:line];
            }
            else if ([line containsString:@"cache_create"]) {
                cacheCreateTokens += [self extractValueFromMetricLine:line];
            }
        }
        else if ([line containsString:@"cost"]) {
            costUSD += [self extractDoubleValueFromMetricLine:line];
        }
    }

    _totalClaudeTokens = inputTokens + outputTokens + cacheReadTokens + cacheCreateTokens;
    _totalCostUSD = costUSD;
}

- (UInt64)extractValueFromMetricLine:(NSString *)line {
    NSArray *parts = [line componentsSeparatedByString:@" "];
    NSMutableArray *nonEmptyParts = [NSMutableArray array];
    for (NSString *part in parts) {
        if ([part length] > 0) [nonEmptyParts addObject:part];
    }

    if ([nonEmptyParts count] >= 2) {
        NSString *valueStr = nonEmptyParts[[nonEmptyParts count] - 2];
        return (UInt64)[valueStr longLongValue];
    }
    return 0;
}

- (double)extractDoubleValueFromMetricLine:(NSString *)line {
    NSArray *parts = [line componentsSeparatedByString:@" "];
    NSMutableArray *nonEmptyParts = [NSMutableArray array];
    for (NSString *part in parts) {
        if ([part length] > 0) [nonEmptyParts addObject:part];
    }

    if ([nonEmptyParts count] >= 2) {
        NSString *valueStr = nonEmptyParts[[nonEmptyParts count] - 2];
        return [valueStr doubleValue];
    }
    return 0.0;
}

#pragma mark - Strategy 1: JSONL Transcripts (Universal)

- (BOOL)fetchFromJSONLTranscripts {
    // CRITICAL: Return immediately with cached value (non-blocking!)
    // File operations happen asynchronously on background queue

    // Thread-safe read of cached value
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    _totalClaudeTokens = cachedJSONLTokens;
    dispatch_semaphore_signal(cacheSemaphore);

    // Check if we should trigger background update
    NSDate *now = [NSDate date];
    BOOL shouldUpdate = NO;

    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);

    // Trigger update if:
    // 1. First time (no last scan time)
    // 2. Been more than 30 seconds since last scan
    // NOTE: With 26K+ JSONL files, scanning more frequently causes 100% CPU!
    // The 1-second throttle was too aggressive for large file counts.
    if (!lastJSONLScanTime || [now timeIntervalSinceDate:lastJSONLScanTime] > 30.0) {
        shouldUpdate = YES;
        lastJSONLScanTime = now; // Prevent multiple simultaneous updates
    }
    dispatch_semaphore_signal(cacheSemaphore);

    // Dispatch background update (non-blocking)
    if (shouldUpdate) {
        dispatch_async(jsonlParsingQueue, ^{
            [self updateJSONLCacheInBackground];
        });
    }

    return YES; // Always succeeds (returns cached value immediately)
}

// Background thread method - does all the heavy file I/O
- (void)updateJSONLCacheInBackground {
    NSFileManager *fm = [NSFileManager defaultManager];
    NSError *error = nil;

    // Get all project directories in ~/.claude/projects/
    NSArray *projects = [fm contentsOfDirectoryAtPath:jsonlProjectsPath error:&error];
    if (error || !projects) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] Cannot access JSONL projects path: %@", jsonlProjectsPath);
#endif
        return;
    }

    // Thread-safe read of current cache
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    UInt64 totalTokens = cachedJSONLTokens;
    NSMutableDictionary *modTimes = [jsonlFileModTimes mutableCopy]; // Work on copy
    dispatch_semaphore_signal(cacheSemaphore);

    BOOL anyChanges = NO;

    // Scan all project directories (on background thread - won't block UI)
    for (NSString *projectName in projects) {
        NSString *projectPath = [jsonlProjectsPath stringByAppendingPathComponent:projectName];

        BOOL isDir = NO;
        if (![fm fileExistsAtPath:projectPath isDirectory:&isDir] || !isDir) {
            continue;
        }

        NSArray *files = [fm contentsOfDirectoryAtPath:projectPath error:nil];
        if (!files) continue;

        for (NSString *filename in files) {
            if ([filename hasSuffix:@".jsonl"]) {
                NSString *filePath = [projectPath stringByAppendingPathComponent:filename];

                NSDictionary *attrs = [fm attributesOfItemAtPath:filePath error:nil];
                if (!attrs) continue;

                NSDate *modTime = attrs[NSFileModificationDate];
                NSDate *cachedModTime = modTimes[filePath];

                // Skip unchanged files
                if (cachedModTime && [modTime isEqualToDate:cachedModTime]) {
                    continue;
                }

                // File is new or modified - parse it
                anyChanges = YES;

                // Subtract old tokens if file was previously cached
                NSNumber *oldFileTokens = modTimes[[filePath stringByAppendingString:@"_tokens"]];
                if (oldFileTokens) {
                    totalTokens -= [oldFileTokens unsignedLongLongValue];
                }

                // Parse this file (on background thread)
                UInt64 fileTokens = [self parseJSONLFile:filePath];

                totalTokens += fileTokens;

                // Update local copy of cache
                modTimes[filePath] = modTime;
                modTimes[[filePath stringByAppendingString:@"_tokens"]] = @(fileTokens);
            }
        }
    }

    // Thread-safe update of cache (only if changes detected)
    if (anyChanges) {
        dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
        cachedJSONLTokens = totalTokens;
        jsonlFileModTimes = modTimes; // Replace with updated copy
        dispatch_semaphore_signal(cacheSemaphore);

#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenMiner] JSONL cache updated in background: tokens=%llu", totalTokens);
#endif
    }
}

// Helper method to parse a single JSONL file using streaming (memory efficient)
- (UInt64)parseJSONLFile:(NSString *)filePath {
    FILE *file = fopen([filePath UTF8String], "r");
    if (!file) return 0;

    UInt64 fileTokens = 0;
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    // Read file line-by-line to minimize memory usage
    // This avoids loading 443MB+ of JSONL files into memory at once
    while ((linelen = getline(&line, &linecap, file)) > 0) {
        if (linelen <= 1) continue;  // Skip empty lines

        @autoreleasepool {
            NSData *jsonData = [NSData dataWithBytesNoCopy:line length:linelen freeWhenDone:NO];
            NSError *parseError = nil;
            NSDictionary *data = [NSJSONSerialization JSONObjectWithData:jsonData
                                                                options:0
                                                                  error:&parseError];

            // Extract token usage from message.usage
            if (!data || [data isKindOfClass:[NSNull class]]) continue;
            NSDictionary *message = data[@"message"];
            if (!message || [message isKindOfClass:[NSNull class]]) continue;
            NSDictionary *usage = message[@"usage"];
            if (usage && ![usage isKindOfClass:[NSNull class]]) {
                // Calculate prompt and completion tokens
                UInt64 promptTokens = [usage[@"input_tokens"] unsignedLongLongValue] +
                                     [usage[@"cache_creation_input_tokens"] unsignedLongLongValue] +
                                     [usage[@"cache_read_input_tokens"] unsignedLongLongValue];
                UInt64 completionTokens = [usage[@"output_tokens"] unsignedLongLongValue];

                fileTokens += promptTokens + completionTokens;
            }
        }
    }

    free(line);
    fclose(file);

    return fileTokens;
}

#pragma mark - Codex CLI Methods

- (BOOL)fetchFromCodexCLI {
    // CRITICAL: Return immediately with cached value (non-blocking!)
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    _totalCodexTokens = cachedCodexTokens;
    dispatch_semaphore_signal(cacheSemaphore);

    // Check if we should trigger background update (throttled)
    NSDate *now = [NSDate date];
    BOOL shouldUpdate = NO;

    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    // Throttle to every 30 seconds to avoid excessive file I/O
    if (!lastCodexScanTime || [now timeIntervalSinceDate:lastCodexScanTime] > 30.0) {
        shouldUpdate = YES;
        lastCodexScanTime = now;
    }
    dispatch_semaphore_signal(cacheSemaphore);

    // Dispatch background update (non-blocking)
    if (shouldUpdate) {
        dispatch_async(jsonlParsingQueue, ^{
            [self updateCodexCacheInBackground];
        });
    }

    return YES;
}

- (void)updateCodexCacheInBackground {
    NSFileManager *fm = [NSFileManager defaultManager];

    if (![fm fileExistsAtPath:codexSessionsPath]) {
        return;
    }

    UInt64 totalTokens = 0;

    // Traverse YYYY/MM/DD directory structure
    NSArray *years = [fm contentsOfDirectoryAtPath:codexSessionsPath error:nil];
    if (!years) return;

    for (NSString *year in years) {
        NSString *yearPath = [codexSessionsPath stringByAppendingPathComponent:year];
        BOOL isDir = NO;
        if (![fm fileExistsAtPath:yearPath isDirectory:&isDir] || !isDir) continue;

        NSArray *months = [fm contentsOfDirectoryAtPath:yearPath error:nil];
        if (!months) continue;

        for (NSString *month in months) {
            NSString *monthPath = [yearPath stringByAppendingPathComponent:month];
            if (![fm fileExistsAtPath:monthPath isDirectory:&isDir] || !isDir) continue;

            NSArray *days = [fm contentsOfDirectoryAtPath:monthPath error:nil];
            if (!days) continue;

            for (NSString *day in days) {
                NSString *dayPath = [monthPath stringByAppendingPathComponent:day];
                if (![fm fileExistsAtPath:dayPath isDirectory:&isDir] || !isDir) continue;

                NSArray *files = [fm contentsOfDirectoryAtPath:dayPath error:nil];
                if (!files) continue;

                for (NSString *filename in files) {
                    if ([filename hasPrefix:@"rollout-"] && [filename hasSuffix:@".jsonl"]) {
                        NSString *filePath = [dayPath stringByAppendingPathComponent:filename];
                        totalTokens += [self parseCodexJSONLFile:filePath];
                    }
                }
            }
        }
    }

    // Thread-safe update of cache
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    cachedCodexTokens = totalTokens;
    dispatch_semaphore_signal(cacheSemaphore);

#ifdef XRG_DEBUG
    if (totalTokens > 0) {
        NSLog(@"[XRGAITokenMiner] Codex cache updated: tokens=%llu", totalTokens);
    }
#endif
}

// Memory-efficient streaming parser for Codex JSONL files
- (UInt64)parseCodexJSONLFile:(NSString *)filePath {
    FILE *file = fopen([filePath UTF8String], "r");
    if (!file) return 0;

    UInt64 lastTotal = 0;
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, file)) > 0) {
        if (linelen <= 1) continue;

        @autoreleasepool {
            NSData *jsonData = [NSData dataWithBytesNoCopy:line length:linelen freeWhenDone:NO];
            NSDictionary *data = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
            if (!data) continue;

            if (![data[@"type"] isEqualToString:@"event_msg"]) continue;

            NSDictionary *payload = data[@"payload"];
            if (!payload || [payload isKindOfClass:[NSNull class]]) continue;
            if (![payload[@"type"] isEqualToString:@"token_count"]) continue;

            NSDictionary *info = payload[@"info"];
            if (!info || [info isKindOfClass:[NSNull class]]) continue;

            NSDictionary *totalUsage = info[@"total_token_usage"];
            if (!totalUsage || [totalUsage isKindOfClass:[NSNull class]]) continue;

            if (totalUsage[@"total_tokens"]) {
                lastTotal = [totalUsage[@"total_tokens"] unsignedLongLongValue];
            } else {
                UInt64 input = [totalUsage[@"input_tokens"] unsignedLongLongValue];
                UInt64 output = [totalUsage[@"output_tokens"] unsignedLongLongValue];
                lastTotal = input + output;
            }
        }
    }

    free(line);
    fclose(file);

    return lastTotal;
}

#pragma mark - Gemini CLI Methods

- (BOOL)fetchFromGeminiCLI {
    // CRITICAL: Return immediately with cached value (non-blocking!)
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    _totalGeminiTokens = cachedGeminiTokens;
    dispatch_semaphore_signal(cacheSemaphore);

    // Check if we should trigger background update (throttled)
    NSDate *now = [NSDate date];
    BOOL shouldUpdate = NO;

    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    // Throttle to every 30 seconds to avoid excessive file I/O
    if (!lastGeminiScanTime || [now timeIntervalSinceDate:lastGeminiScanTime] > 30.0) {
        shouldUpdate = YES;
        lastGeminiScanTime = now;
    }
    dispatch_semaphore_signal(cacheSemaphore);

    // Dispatch background update (non-blocking)
    if (shouldUpdate) {
        dispatch_async(jsonlParsingQueue, ^{
            [self updateGeminiCacheInBackground];
        });
    }

    return YES;
}

- (void)updateGeminiCacheInBackground {
    NSFileManager *fm = [NSFileManager defaultManager];

    if (![fm fileExistsAtPath:geminiTmpPath]) {
        return;
    }

    UInt64 totalTokens = 0;

    // Traverse <hash>/chats/ directories
    NSArray *hashes = [fm contentsOfDirectoryAtPath:geminiTmpPath error:nil];
    if (!hashes) return;

    for (NSString *hashDir in hashes) {
        NSString *hashPath = [geminiTmpPath stringByAppendingPathComponent:hashDir];
        BOOL isDir = NO;
        if (![fm fileExistsAtPath:hashPath isDirectory:&isDir] || !isDir) continue;

        NSString *chatsPath = [hashPath stringByAppendingPathComponent:@"chats"];
        if (![fm fileExistsAtPath:chatsPath isDirectory:&isDir] || !isDir) continue;

        NSArray *files = [fm contentsOfDirectoryAtPath:chatsPath error:nil];
        if (!files) continue;

        for (NSString *filename in files) {
            if ([filename hasPrefix:@"session-"] && [filename hasSuffix:@".json"]) {
                NSString *filePath = [chatsPath stringByAppendingPathComponent:filename];
                totalTokens += [self parseGeminiSessionFile:filePath];
            }
        }
    }

    // Thread-safe update of cache
    dispatch_semaphore_wait(cacheSemaphore, DISPATCH_TIME_FOREVER);
    cachedGeminiTokens = totalTokens;
    dispatch_semaphore_signal(cacheSemaphore);

#ifdef XRG_DEBUG
    if (totalTokens > 0) {
        NSLog(@"[XRGAITokenMiner] Gemini cache updated: tokens=%llu", totalTokens);
    }
#endif
}

- (UInt64)parseGeminiSessionFile:(NSString *)filePath {
    NSData *jsonData = [NSData dataWithContentsOfFile:filePath];
    if (!jsonData) return 0;

    NSError *parseError = nil;
    NSDictionary *data = [NSJSONSerialization JSONObjectWithData:jsonData
                                                        options:0
                                                          error:&parseError];
    if (!data) return 0;

    UInt64 totalTokens = 0;

    // Look for messages array
    NSArray *messages = data[@"messages"];
    if (!messages || [messages isKindOfClass:[NSNull class]]) return 0;

    for (NSDictionary *msg in messages) {
        if (!msg || [msg isKindOfClass:[NSNull class]]) continue;
        NSDictionary *tokens = msg[@"tokens"];
        if (!tokens || [tokens isKindOfClass:[NSNull class]]) continue;

        // Use "total" if available, otherwise sum input + output
        if (tokens[@"total"]) {
            totalTokens += [tokens[@"total"] unsignedLongLongValue];
        } else {
            UInt64 input = [tokens[@"input"] unsignedLongLongValue];
            UInt64 output = [tokens[@"output"] unsignedLongLongValue];
            totalTokens += input + output;
        }
    }

    return totalTokens;
}

#pragma mark - Accessor Methods

- (UInt32)claudeTokenRate {
    return currentClaudeRate;
}

- (UInt32)codexTokenRate {
    return currentCodexRate;
}

- (UInt32)geminiTokenRate {
    return currentGeminiRate;
}

- (UInt32)totalTokenRate {
    return currentClaudeRate + currentCodexRate + currentGeminiRate;
}

- (XRGDataSet *)claudeTokenData {
    return claudeCodeTokens;
}

- (XRGDataSet *)codexTokenData {
    return codexTokens;
}

- (XRGDataSet *)geminiTokenData {
    return geminiTokens;
}

#pragma mark - Private Networking Helper

- (NSData *)xrg_syncDataForRequest:(NSURLRequest *)request returningResponse:(NSHTTPURLResponse * _Nullable __autoreleasing *)response error:(NSError * _Nullable __autoreleasing *)error {
    __block NSData *resultData = nil;
    __block NSURLResponse *resultResponse = nil;
    __block NSError *resultError = nil;

    NSURLSessionConfiguration *config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
    config.requestCachePolicy = request.cachePolicy;
    config.timeoutIntervalForRequest = request.timeoutInterval > 0 ? request.timeoutInterval : 2.0;
    config.timeoutIntervalForResource = config.timeoutIntervalForRequest + 1.0;

    NSURLSession *session = [NSURLSession sessionWithConfiguration:config];
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);

    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable resp, NSError * _Nullable err) {
        resultData = data;
        resultResponse = resp;
        resultError = err;
        dispatch_semaphore_signal(sema);
    }];
    [task resume];

    // Wait up to the timeout + small grace period
    NSTimeInterval waitSeconds = config.timeoutIntervalForRequest + 0.5;
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(waitSeconds * NSEC_PER_SEC));
    if (dispatch_semaphore_wait(sema, timeout) != 0) {
        // Timed out
        [task cancel];
        resultError = resultError ?: [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorTimedOut userInfo:nil];
    }

    [session finishTasksAndInvalidate];

    if (response) {
        if ([resultResponse isKindOfClass:[NSHTTPURLResponse class]]) {
            *response = (NSHTTPURLResponse *)resultResponse;
        } else {
            *response = nil;
        }
    }
    if (error) {
        *error = resultError;
    }
    return resultData;
}

@end

