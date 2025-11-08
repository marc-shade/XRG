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
//  XRGAITokenMiner.h
//  Universal AI Token monitoring with multi-strategy data collection
//  Works on all Macs with any Claude Code installation
//

#import <Foundation/Foundation.h>
#import "XRGDataSet.h"

// Data collection strategies (auto-detected and used in priority order)
typedef NS_ENUM(NSInteger, XRGAIDataStrategy) {
    XRGAIDataStrategyNone = 0,
    XRGAIDataStrategyJSONL = 1,       // Universal: ~/.claude/projects/*/sessionid.jsonl (DEFAULT Claude Code)
    XRGAIDataStrategySQLite = 2,      // Advanced: ~/.claude/monitoring/claude_usage.db (custom monitoring)
    XRGAIDataStrategyOTel = 3         // Advanced: OpenTelemetry endpoint (OTel configured)
};

@interface XRGAITokenMiner : NSObject {
@private
    int numSamples;

    // Data sets for different AI services
    XRGDataSet *claudeCodeTokens;      // Claude Code API tokens/sec
    XRGDataSet *codexTokens;            // OpenAI Codex tokens/sec (if available)
    XRGDataSet *otherAITokens;          // Other AI services tokens/sec

    // Cumulative counters
    UInt64 totalClaudeTokens;
    UInt64 totalCodexTokens;
    UInt64 totalOtherTokens;
    double totalCostUSD;

    // Rate tracking (for proper delta calculation)
    UInt64 lastClaudeCount;
    UInt64 lastCodexCount;
    UInt64 lastOtherCount;

    // Current rates (stored to avoid recalculation bug)
    UInt32 currentClaudeRate;
    UInt32 currentCodexRate;
    UInt32 currentOtherRate;

    // Multi-strategy data collection
    XRGAIDataStrategy activeStrategy;
    NSString *jsonlProjectsPath;        // JSONL transcripts path (universal)
    NSString *dbPath;                   // SQLite database path (advanced)
    NSString *otelEndpoint;             // OTel metrics endpoint (advanced)

    // JSONL caching for performance (avoid re-parsing every second)
    NSMutableDictionary *jsonlFileModTimes;  // Track file modification times
    UInt64 cachedJSONLTokens;                 // Cached total from last parse
    NSDate *lastJSONLScanTime;                // Last time we scanned for new files
    dispatch_queue_t jsonlParsingQueue;       // Background queue for file I/O
    dispatch_semaphore_t cacheSemaphore;      // Thread-safe cache access
}

// Properties
@property (nonatomic, assign) UInt64 totalClaudeTokens;
@property (nonatomic, assign) UInt64 totalCodexTokens;
@property (nonatomic, assign) UInt64 totalOtherTokens;
@property (nonatomic, assign) double totalCostUSD;
@property (nonatomic, assign, readonly) XRGAIDataStrategy activeStrategy;

// Core methods
- (void)getLatestTokenInfo;
- (void)setDataSize:(int)newNumSamples;
- (void)reset;

// Accessor methods for current rates (tokens per second)
- (UInt32)claudeTokenRate;
- (UInt32)codexTokenRate;
- (UInt32)otherTokenRate;
- (UInt32)totalTokenRate;

// Data set accessors for graphing
- (XRGDataSet *)claudeTokenData;
- (XRGDataSet *)codexTokenData;
- (XRGDataSet *)otherTokenData;

// Strategy detection and management
- (void)detectBestStrategy;
- (NSString *)strategyName;

// Data collection strategies
- (BOOL)fetchFromJSONLTranscripts;
- (void)updateJSONLCacheInBackground;  // Background thread update
- (UInt64)parseJSONLFile:(NSString *)filePath;  // Helper for incremental parsing
- (BOOL)fetchFromSQLiteDatabase;
- (BOOL)fetchFromOTelEndpoint;

@end
