# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

XRG is a system resource monitoring application for macOS written in Objective-C using Cocoa/AppKit. It displays real-time graphs for CPU, memory, network, disk, GPU, battery, temperature, weather, stocks, and AI token usage in a customizable floating window.

Website: https://gaucho.software/xrg/

## Building and Running

### Build Commands

```bash
# Build the project
xcodebuild -project XRG.xcodeproj -scheme XRG build

# Build for debugging
xcodebuild -project XRG.xcodeproj -scheme XRG -configuration Debug build

# Build for release
xcodebuild -project XRG.xcodeproj -scheme XRG -configuration Release build

# Clean build folder
xcodebuild -project XRG.xcodeproj -scheme XRG clean

# Build and run (use Xcode for easier debugging)
open XRG.xcodeproj  # Then press ⌘R in Xcode
```

### Build Without Code Signing

```bash
xcodebuild -project XRG.xcodeproj -scheme XRG \
    CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO build
```

### Target Information

- **Single Target**: XRG
- **Schemes**: XRG
- **Minimum OS**: macOS 10.13
- **Build Configurations**: Debug, Release
- **Required Frameworks**: Cocoa, IOKit, Accelerate

## Architecture

### Three-Layer Module Pattern

XRG uses a consistent architecture across all monitoring modules:

1. **Data Miner Layer** (`Data Miners/`)
   - Inherits from `NSObject`
   - Collects system metrics via IOKit, sysctl, or log file parsing
   - Stores time-series data in `XRGDataSet` ring buffers
   - Updates on timer callbacks: `graphUpdate:`, `fastUpdate:`, `min5Update:`, `min30Update:`
   - Examples: `XRGCPUMiner`, `XRGMemoryMiner`, `XRGNetMiner`, `XRGAITokenMiner`

2. **View Layer** (`Graph Views/`)
   - Inherits from `XRGGenericView` → `NSView`
   - Owns its corresponding miner instance
   - Implements `drawRect:` to render graphs using Quartz
   - Responds to timer events by querying the miner and calling `setNeedsDisplay:`
   - Supports both full and "mini" display modes
   - Examples: `XRGCPUView`, `XRGMemoryView`, `XRGNetView`, `XRGAITokenView`

3. **Module Registration** (via `XRGModule`)
   - Created in view's `awakeFromNib`
   - Registered with `XRGModuleManager` in `XRGGraphWindow`
   - Manages visibility (`isDisplayed`), display order, size constraints
   - Controls which timer events the module receives

### Core Components

- **`XRGAppDelegate`**: Application lifecycle, preferences, window management
- **`XRGGraphWindow`**: Main floating window, hosts all graph views, owns module manager
- **`XRGModuleManager`**: Orchestrates modules, handles layout, dispatches timer events
- **`XRGSettings`**: User preferences wrapper around `NSUserDefaults`
- **`XRGDataSet`**: Ring buffer for time-series data storage
- **`XRGGenericView`**: Base class providing common graphing methods
- **`XRGModule`**: Metadata container (name, size, visibility, update flags)

### Timer-Based Updates

The app uses multiple timers with different frequencies:

- **`graphTimer`** (1 second): Main update loop for most graphs
- **`fastTimer`** (0.1 seconds): High-frequency updates (CPU, network)
- **`min5Timer`** (5 minutes): Weather, stock updates
- **`min30Timer`** (30 minutes): Extended polling intervals

Modules opt-in to timer events via flags:
```objc
m.doesGraphUpdate = YES;      // Receives 1-second updates
m.doesFastUpdate = YES;       // Receives 0.1-second updates
m.doesMin5Update = YES;       // Receives 5-minute updates
m.alwaysDoesGraphUpdate = YES; // Updates even when window minimized
```

### Hardware Access Patterns

**Apple Silicon Sensors** (`Data Miners/APSL/`):
- Uses `IOHIDEventSystemClientCreate` to access HID sensors
- `XRGAppleSiliconSensorMiner` bridges to older `XRGTemperatureMiner`
- `SMCSensors` and `SMCSensorGroup` abstract sensor reading
- Sensor names mapped via `Resources/SMCSensorNames.plist`

**Intel SMC Access**:
- Uses `SMCInterface.m` to read System Management Controller
- Provides temperature, fan speed, and power metrics

**Network Stats**:
- Direct `ioctl` calls with `SIOCGIFDATA` for interface stats
- Ignores `lo0` (loopback) to avoid phantom VPN traffic
- Tracks `bytes_delta` for rate calculations

**GPU Monitoring**:
- Uses `IOAccelerator` and `IOGraphics` APIs
- Tracks GPU utilization and memory usage

## Adding a New Module

Follow these steps to add a monitoring module (refer to AI Token implementation as example):

### 1. Create Data Miner

File: `Data Miners/XRGYourModuleMiner.h`
```objc
@interface XRGYourModuleMiner : NSObject
@property NSMutableArray<XRGDataSet *> *dataValues;
- (void)graphUpdate:(NSTimer *)aTimer;
- (void)setDataSize:(NSInteger)numSamples;
@end
```

File: `Data Miners/XRGYourModuleMiner.m`
```objc
- (void)graphUpdate:(NSTimer *)aTimer {
    // Collect metrics
    // Store in XRGDataSet
    [self.dataValues addValue:newValue];
}
```

### 2. Create Graph View

File: `Graph Views/XRGYourModuleView.h`
```objc
#import "XRGGenericView.h"
@interface XRGYourModuleView : XRGGenericView
@end
```

File: `Graph Views/XRGYourModuleView.m`
```objc
- (void)awakeFromNib {
    // Create miner
    self.miner = [[XRGYourModuleMiner alloc] init];

    // Register module
    XRGModule *m = [[XRGModule alloc] initWithName:@"YourModule" andReference:self];
    m.displayOrder = 10;  // Sequential order: 0-9 already used (CPU through AI Tokens)
    m.isDisplayed = [appSettings showYourModuleGraph];
    m.doesGraphUpdate = YES;
    [moduleManager addModule:m];
}

- (void)drawRect:(NSRect)rect {
    // Draw graph using [self.miner dataValues]
}
```

**Important**: Display order must be sequential (0-9 currently used). Gaps in numbering can cause layout issues and overlapping modules.

### 3. Update Configuration

In `Other Sources/definitions.h`:
```objc
#define XRG_showYourModuleGraph    @"showYourModuleGraph"
#define XRG_YourModuleOrder        @"YourModuleOrder"
```

### 4. Wire Up Window Controller

In `Controllers/XRGGraphWindow.h`:
```objc
#import "XRGYourModuleView.h"
@property XRGYourModuleView *yourModuleView;
- (IBAction)setShowYourModuleGraph:(id)sender;
```

In `Controllers/XRGGraphWindow.m`:
```objc
- (IBAction)setShowYourModuleGraph:(id)sender {
    [self.backgroundView expandWindow];
    [self.moduleManager setModule:@"YourModule" isDisplayed:([sender state] == NSOnState)];
    [self setMinSize:[self.moduleManager getMinSize]];
    [self checkWindowSize];
    [self.moduleManager windowChangedToSize:[self frame].size];
}
```

### 5. Add to Xcode Project

1. Right-click `Data Miners` group → Add Files
2. Right-click `Graph Views` group → Add Files
3. Ensure target `XRG` is checked
4. Build to verify

### 6. Update NIB Files

**MainMenu.nib**:
- Add Custom View with class `XRGYourModuleView`
- Connect outlet from `XRGGraphWindow` to view

**Preferences.nib**:
- Add checkbox "Show Your Module"
- Bind to `NSUserDefaults.showYourModuleGraph`
- Connect action to `setShowYourModuleGraph:`

## Common Development Tasks

### Adding User Preferences

1. Define key in `definitions.h`:
   ```objc
   #define XRG_myNewSetting    @"myNewSetting"
   ```

2. Access in code:
   ```objc
   BOOL value = [[NSUserDefaults standardUserDefaults] boolForKey:XRG_myNewSetting];
   [[NSUserDefaults standardUserDefaults] setObject:value forKey:XRG_myNewSetting];
   ```

3. Wrap in `XRGSettings.h` for easier access:
   ```objc
   - (BOOL)myNewSetting;
   - (void)setMyNewSetting:(BOOL)value;
   ```

### Debugging with XRG_DEBUG

Enable debug logging:
```objc
// In definitions.h, change:
#undef XRG_DEBUG
// To:
#define XRG_DEBUG 1
```

Then add logging:
```objc
#ifdef XRG_DEBUG
NSLog(@"Debug info: %@", data);
#endif
```

### Drawing Graphs

Use helper methods from `XRGGenericView`:

```objc
// Stacked area graph
[self drawGraphWithDataFromDataSet:dataset
                          maxValue:maxValue
                            inRect:graphRect
                           flipped:YES
                            filled:YES
                             color:[appSettings graphFG1Color]];

// Line graph
[self drawRangedGraphWithDataFromDataSet:dataset
                              upperBound:100
                              lowerBound:0
                                  inRect:graphRect
                                 flipped:NO
                                  filled:NO
                                   color:[appSettings graphFG2Color]];

// Mini mode
[self drawMiniGraphWithValues:values
                   upperBound:max
                   lowerBound:min
                    leftLabel:@"Label"
                   rightLabel:@"Value"];
```

### Color Scheme

All modules use centralized color preferences:
- `graphBGColor`: Graph background
- `graphFG1Color`, `graphFG2Color`, `graphFG3Color`: Three data series colors
- `backgroundColor`: Window background
- `textColor`: Labels and text
- `borderColor`: Window borders

Each has corresponding transparency settings.

### Size and Layout

Modules define size constraints:
```objc
m.minHeight = 40;         // Minimum height in pixels
m.maxHeight = 300;        // Maximum height
m.minWidth = 100;         // Minimum width
// Width auto-adjusts based on window size
```

`XRGModuleManager` handles layout based on orientation:
- Vertical: Modules stack top-to-bottom
- Horizontal: Modules arrange left-to-right

### Testing Without Running Full App

For testing miners or calculations:
```objc
// In a separate test file
int main() {
    @autoreleasepool {
        XRGYourModuleMiner *miner = [[XRGYourModuleMiner alloc] init];
        [miner graphUpdate:nil];
        NSLog(@"Result: %@", [miner dataValues]);
    }
    return 0;
}
```

## Recent Changes

### AI Token Monitoring Module (Nov 2025) - Universal Implementation

Added real-time AI API token usage tracking that works on **all Macs** with **any Claude Code installation**:

- **Files**: `XRGAITokenMiner.{h,m}`, `XRGAITokenView.{h,m}`
- **Purpose**: Monitor Claude Code, OpenAI Codex, and other AI service token usage
- **Documentation**: See `UNIVERSAL_AI_TOKEN_MONITORING.md` for complete details

**Universal Multi-Strategy Data Collection**:

The miner automatically detects and uses the best available data source:

1. **Strategy 1 (Universal)**: JSONL Transcripts `~/.claude/projects/*/sessionid.jsonl`
   - Works on ALL Macs with default Claude Code installation
   - No setup or configuration required
   - Parses session transcripts for token usage
   - Background threading for performance
   - Intelligent caching to avoid re-parsing

2. **Strategy 2 (Advanced)**: SQLite Database `~/.claude/monitoring/claude_usage.db`
   - For users with custom monitoring setups
   - Queries session table for total tokens and costs
   - Faster than JSONL parsing when available

3. **Strategy 3 (Advanced)**: OpenTelemetry Endpoint `http://localhost:8889/metrics`
   - For users with OTel configured (optional)
   - Provides real-time granular metrics
   - Automatic health check on startup

**Key Features**:
- ✅ Zero configuration required
- ✅ Works with default and modified AI stacks
- ✅ Automatic failover between strategies
- ✅ No external dependencies or setup
- ✅ Compatible with macOS 10.13+

**Implementation Details**:
- Auto-detects best data source on startup
- Calculates token rate as delta between updates
- Stores rates in instance variables (avoids recalculation bug)
- Three separate data series: `claudeCodeTokens`, `codexTokens`, `otherAITokens`
- Uses native SQLite3 library (linked via `-lsqlite3` in project settings)
- Critical bug fix: Must store rates BEFORE updating lastCount to avoid always-zero rate

**Miner-Observer Integration** (Nov 2025 fix):
The system uses two complementary components:
- **XRGAITokenMiner**: Parses JSONL/SQLite/OTel data sources for total token counts and rates
- **XRGAITokensObserver**: Tracks individual events with model/provider breakdown and budget notifications

The Miner feeds parsed events to the Observer:
```objc
// In parseJSONLFile - for each JSONL line with token usage:
[observer recordEventWithPromptTokens:promptTokens
                     completionTokens:completionTokens
                               model:model        // e.g. "claude-sonnet-4-5"
                            provider:provider];   // e.g. "anthropic"
```

This enables:
- Model breakdown: See which models (claude-sonnet-4-5, gpt-4, etc.) consume most tokens
- Provider breakdown: Track usage across anthropic, openai, etc.
- Budget tracking: Set daily limits and get threshold notifications
- Prompt vs completion separation: Understand input vs output token usage

**Critical**: `aiTokensTrackingEnabled` must be YES (default) or Observer will ignore all events.

### AI Token Settings and Features

The AI Token module has comprehensive settings for tracking, budgeting, and visualizing AI token usage. All settings are defined in `XRGAISettingsKeys.h` and managed through `XRGSettings`:

**Available Settings** (all fully implemented):

1. **`aiTokensTrackingEnabled`** (BOOL, default: NO)
   - Master toggle for token tracking
   - When disabled, no tokens are recorded
   - Used by `XRGAITokensObserver` to gate all recording operations

2. **`aiTokensDailyAutoReset`** (BOOL, default: YES)
   - Automatically reset daily counters at midnight
   - Triggered by `checkAndResetDailyIfNeeded` in Observer
   - When disabled, daily counters persist until manually reset

3. **`aiTokensDailyBudget`** (NSInteger, default: 0)
   - Daily token budget limit (0 = unlimited)
   - Displayed in view as "Daily: X/Y (Z%)"
   - Triggers budget threshold notifications when exceeded

4. **`aiTokensBudgetNotifyPercent`** (NSInteger, default: 80)
   - Percentage of budget that triggers notification (1-100)
   - Implemented in `checkBudgetThresholdAndNotifyIfNeeded`
   - Sends `XRGAITokensBudgetThresholdReachedNotification` when reached
   - Only notifies once per day to avoid spam

5. **`aiTokensAggregateByModel`** (BOOL, default: YES)
   - Track token usage per AI model (e.g., "claude-sonnet-4-5", "gpt-4")
   - Displayed in breakdown section when `showBreakdown` is enabled
   - Stored in `dailyModelPromptTokens` and `dailyModelCompletionTokens` dictionaries

6. **`aiTokensAggregateByProvider`** (BOOL, default: NO)
   - Track token usage per provider (e.g., "anthropic", "openai")
   - Displayed in breakdown section when `showBreakdown` is enabled
   - Stored in `dailyProviderPromptTokens` and `dailyProviderCompletionTokens` dictionaries

7. **`aiTokensShowRate`** (BOOL, default: YES)
   - Display current token rate (tokens/second) in graph
   - Shows "Rate: X/s" or "Rate: XK/s" for rates over 1000

8. **`aiTokensShowBreakdown`** (BOOL, default: YES)
   - Display detailed breakdown by model/provider in graph
   - Shows "─ By Model ─" and "─ By Provider ─" sections with per-item totals

**Observer Pattern**:
- `XRGAITokensObserver` is a singleton that manages all token tracking
- Records events with `recordEventWithPromptTokens:completionTokens:model:provider:`
- Persists daily counters to NSUserDefaults for crash resilience
- Provides read-only properties for session and daily totals
- Supports manual and automatic daily resets

**Notification System**:
- `XRGAITokensDidResetSessionNotification`: Posted when session counters reset
- `XRGAITokensDidResetDailyNotification`: Posted when daily counters reset
- `XRGAITokensBudgetThresholdReachedNotification`: Posted when budget threshold reached
  - UserInfo contains: `@"progress"` (ratio) and `@"percent"` (0-100)

**Context Menu Features**:
The AI Token view provides a right-click context menu with:
- Session statistics (prompt, completion, total tokens)
- Daily statistics (total, budget progress)
- Model breakdown (when aggregateByModel enabled)
- Provider breakdown (when aggregateByProvider enabled)
- All values formatted with K/M suffixes for readability

**Visual Display**:
- Stacked area graphs showing Claude Code (FG1 color), Codex (FG2 color), Other (FG3 color)
- Real-time rate indicator on right side showing proportional breakdown
- Text labels showing rate, session total, daily total, and optional breakdowns
- Budget progress shown as "X/Y (Z%)" when budget configured
- Mini graph mode for compact display

## Code Conventions

- **Language**: Objective-C (modern runtime, ARC enabled where applicable)
- **Naming**: Classes prefixed with `XRG`, ivars with underscores (legacy style)
- **Memory**: Use `strong` properties, avoid manual retain/release in new code
- **Headers**: Import foundation classes in `.h`, specific dependencies in `.m`
- **Comments**: License header in all files (GPL v2), inline comments for complex logic
- **Indentation**: 4 spaces (not tabs in source, tabs in NIB XML)

## Key Files Reference

| File | Purpose |
|------|---------|
| `Controllers/XRGAppDelegate.{h,m}` | App lifecycle, preferences window |
| `Controllers/XRGGraphWindow.{h,m}` | Main window, module hosting |
| `Controllers/XRGPrefController.{h,m}` | Preferences UI controller |
| `Utility/XRGModuleManager.{h,m}` | Module orchestration, layout |
| `Utility/XRGSettings.{h,m}` | User defaults wrapper |
| `Utility/XRGDataSet.{h,m}` | Ring buffer for time series |
| `Utility/XRGModule.{h,m}` | Module metadata container |
| `Graph Views/XRGGenericView.{h,m}` | Base view class, drawing helpers |
| `Other Sources/definitions.h` | Constants, preference keys |
| `Resources/MainMenu.nib` | Main UI layout |
| `Resources/Preferences.nib` | Preferences UI layout |
| `XRG-Info.plist` | Bundle metadata |

## Common Issues

### Network Interface Changes

When adding new network interfaces, ensure `lo0` (loopback) is filtered out to avoid phantom traffic from VPN connections. See commit `0fca322`.

### Temperature Sensor Names

Sensor names are hardware-specific. Use `SMCSensorNames.plist` for friendly name mapping. Auto-configuration runs on first launch.

### GPU Metrics on Apple Silicon

GPU monitoring uses different APIs on Apple Silicon vs. Intel. Check `[XRGCPUMiner systemModelIdentifier]` to determine architecture.

### Build Errors After Adding Files

If Xcode can't find newly added files:
1. Verify files are added to project (not just filesystem)
2. Check target membership (should be checked)
3. Clean build folder (⇧⌘K)
4. Verify imports use correct paths

### High CPU Usage

If XRG uses excessive CPU:
- Check for infinite loops in `drawRect:` (use `setNeedsDisplay:` sparingly)
- Verify timer intervals are reasonable (1 second minimum for most modules)
- Profile with Instruments to identify hotspots

### Module Display Order Issues

If modules overlap or have incorrect positioning:
- Display order must be **sequential** (0, 1, 2, 3...)
- Current assignments: CPU(0), GPU(1), Memory(2), Battery(3), Temperature(4), Network(5), Disk(6), Weather(7), Stock(8), AI Tokens(9)
- Gaps in numbering cause layout problems - use next sequential number when adding modules
- Check `m.displayOrder` in each view's `awakeFromNib` method

### Rate Calculation Bug Pattern

When implementing rate calculations from cumulative counters:
```objc
// ❌ WRONG - Will always return 0
- (UInt32)myRate {
    return (UInt32)(totalCount - lastCount);  // lastCount already updated!
}

// ✅ CORRECT - Store rate in instance variable
- (void)update {
    currentRate = (UInt32)(totalCount - lastCount);  // Calculate BEFORE updating
    lastCount = totalCount;  // Then update
}

- (UInt32)myRate {
    return currentRate;  // Return stored value
}
```

### Missing Readonly Property Getters

When declaring readonly properties in the header, you **must** implement the getter method in the .m file:

```objc
// ❌ WRONG - Header declares property but .m has no implementation
// XRGMyClass.h
@property (readonly) NSDictionary *myData;

// XRGMyClass.m
// (no getter implementation - property returns nil!)

// ✅ CORRECT - Implement the getter
// XRGMyClass.m
- (NSDictionary *)myData {
    // Compute and return the value
    return [self computeData];
}
```

**Real Example from AI Token Observer** (Nov 2025 fix):
The header declared `dailyByModel` and `dailyByProvider` as readonly properties, but had no getter implementations. This caused the breakdown sections to never display. Fixed by implementing getters that combine prompt and completion token dictionaries.

### macOS SDK Compatibility

When targeting older macOS versions (10.13+), be aware of API availability:

**NSBindingName Constants** (10.15+):
```objc
// ❌ WRONG - Not available on macOS 10.13
[control bind:NSBindingNameValue toObject:udc ...];

// ✅ CORRECT - Use string literals for backwards compatibility
[control bind:@"value" toObject:udc ...];
```

Common binding names that require string literals on 10.13:
- `NSBindingNameValue` → `@"value"`
- `NSBindingNameEnabled` → `@"enabled"`
- `NSBindingNameHidden` → `@"hidden"`

### Duplicate File Prevention

When adding new modules, ensure files are not duplicated across directories:
- ✅ **Correct**: Implementation files in proper directories (`Data Miners/`, `Graph Views/`)
- ❌ **Wrong**: Duplicate or experimental files in `Resources/` directory

Before building, verify no conflicting implementations exist:
```bash
# Check for duplicate implementations
find . -name "XRGYourModule*.m" -o -name "XRGYourModule*.h"
```

If duplicate files exist, remove them from the Xcode project (not just filesystem):
1. Select file in Xcode Project Navigator
2. Delete → "Remove Reference" (not "Move to Trash")
3. Or edit `XRG.xcodeproj/project.pbxproj` to remove PBXBuildFile entries

## AI Token Monitoring - Universal Deployment

XRG now includes **universal AI token monitoring** that works on all Macs with any Claude Code installation.

### Quick Start (Zero Configuration)

```bash
# 1. Build and run XRG (works immediately - no setup required)
xcodebuild -project XRG.xcodeproj -scheme XRG build
open ~/Library/Developer/Xcode/DerivedData/XRG-*/Build/Products/Debug/XRG.app

# 2. Enable in XRG: Preferences → Graphs → Show AI Tokens
# That's it! The graph will automatically display your Claude Code usage
```

### How It Works

The AI Token module automatically detects the best data source:

1. **Default (Everyone)**: Parses JSONL transcripts from `~/.claude/projects/*/sessionid.jsonl`
   - Works with default Claude Code installation
   - No setup or configuration needed
   - Background parsing with intelligent caching

2. **Advanced (Optional)**: Reads from `~/.claude/monitoring/claude_usage.db`
   - For users with custom monitoring setups
   - Faster database queries when available

3. **Advanced (Optional)**: Queries OpenTelemetry endpoint if available
   - For users with OTel configured
   - Provides additional granular metrics

### Documentation

| Document | Purpose |
|----------|---------|
| **UNIVERSAL_AI_TOKEN_MONITORING.md** | Complete universal implementation guide |
| **AI_OBSERVABILITY_ARCHITECTURE.md** | Technical architecture details (optional advanced setup) |

### What's Tracked

The AI Token module (display order 9) displays:
- Total tokens used (cumulative across all sessions)
- Token usage rate (tokens/second, real-time)
- Estimated cost in USD (when available)
- Service breakdown (Claude Code, Codex, Other AI)

### Advantages

| Feature | Old System | Universal System |
|---------|-----------|------------------|
| Setup Required | Python scripts, OTel config | None |
| External Dependencies | Yes (bridge, exporter) | No |
| Works Without OTel | No | Yes |
| Works With Custom Stacks | No | Yes |
| Auto-Failover | No | Yes |

For implementation details, see `Data Miners/XRGAITokenMiner.m` and `Graph Views/XRGAITokenView.m`.

## Resources

- **Website**: https://gaucho.software/xrg/
- **License**: GNU GPL v2
- **Minimum OS**: macOS 10.13 (High Sierra)
- **Architecture**: Universal (Intel + Apple Silicon)
