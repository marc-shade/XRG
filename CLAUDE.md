# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

XRG is a **cross-platform** system resource monitoring application with native implementations for **macOS** (Objective-C/Cocoa) and **Linux** (C/GTK3). It displays real-time graphs for CPU, memory, network, disk, GPU, battery, temperature, weather, stocks, and AI token usage.

**Key Points:**
- Both platforms share identical three-layer architecture (Data Collector → Graph View → Module Manager)
- AI token monitoring works on both platforms (parses `~/.claude/projects/*/sessionid.jsonl`)
- macOS: `Controllers/`, `Data Miners/`, `Graph Views/`, `Utility/`
- Linux: `xrg-linux/src/collectors/`, `xrg-linux/src/widgets/`, `xrg-linux/src/core/`

## Building and Running

### macOS Build Commands

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

**Build Without Code Signing:**
```bash
xcodebuild -project XRG.xcodeproj -scheme XRG \
    CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO build
```

**Target Information:**
- **Single Target**: XRG
- **Schemes**: XRG
- **Minimum OS**: macOS 10.13
- **Build Configurations**: Debug, Release
- **Required Frameworks**: Cocoa, IOKit, Accelerate

### Linux Build Commands

```bash
# Navigate to Linux port directory
cd xrg-linux

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run
./xrg-linux

# Install system-wide (optional)
sudo make install
```

**Build Options:**
```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with tests
cmake -DBUILD_TESTS=ON ..
make test
```

**Dependencies (Install Before Building):**

Fedora/RHEL:
```bash
sudo dnf install gtk3-devel cairo-devel glib2-devel json-glib-devel \
                 sqlite-devel libcurl-devel cmake gcc
```

Debian/Ubuntu:
```bash
sudo apt install libgtk-3-dev libcairo2-dev libglib2.0-dev libjson-glib-dev \
                 libsqlite3-dev libcurl4-openssl-dev cmake build-essential
```

**Target Information:**
- **Build System**: CMake 3.16+
- **Minimum Libs**: GTK 3.24, Cairo 1.16, GLib 2.66
- **Build Configurations**: Debug, Release
- **Required Libraries**: GTK3, Cairo, GLib, SQLite3, libcurl, json-glib

## Architecture

XRG maintains a consistent three-layer architecture across **both macOS and Linux** implementations, with platform-specific data collection and rendering layers.

### Three-Layer Module Pattern

Both platforms use this consistent architecture across all monitoring modules:

**macOS Implementation:**

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

**Linux Implementation:**

1. **Data Collector Layer** (`xrg-linux/src/collectors/`)
   - Plain C structs with function pointers
   - Collects system metrics via `/proc`, `/sys`, lm-sensors
   - Stores time-series data in `XRGDataSet` ring buffers (same as macOS)
   - Updates on GLib timer callbacks: `update_callback()`
   - Examples: `cpu_collector.c`, `memory_collector.c`, `network_collector.c`, `aitoken_collector.c`

2. **Widget Layer** (`xrg-linux/src/widgets/`)
   - Custom GTK widgets using `GtkDrawingArea`
   - Owns its corresponding collector instance
   - Implements `draw` signal handler to render graphs using Cairo
   - Responds to timer events by querying the collector and calling `gtk_widget_queue_draw()`
   - Includes activity bar showing real-time stats
   - Examples: `cpu_widget.c`, `memory_widget.c`, `network_widget.c`, `aitoken_widget.c`

3. **Module Manager** (`xrg-linux/src/core/module_manager.c`)
   - Manages module lifecycle and layout
   - Tracks visibility, display order, size constraints
   - Dispatches timer events to modules
   - Same conceptual design as macOS `XRGModuleManager`

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

### Platform-Specific Data Sources

| Module | macOS Data Source | Linux Data Source |
|--------|-------------------|-------------------|
| **CPU** | IOKit, `host_processor_info()`, `vm_statistics64()` | `/proc/stat`, `/proc/loadavg` |
| **Memory** | Mach VM: `host_statistics64()`, `vm_statistics64()` | `/proc/meminfo`, `/proc/vmstat` |
| **Network** | `ioctl` with `SIOCGIFDATA` | `/proc/net/dev`, `/sys/class/net/*/statistics/` |
| **Disk** | IOKit disk statistics | `/proc/diskstats`, `/sys/block/*/stat` |
| **GPU** | IOAccelerator, IOGraphics (Apple Silicon/Intel) | NVML (NVIDIA), libdrm (AMD/Intel), `/sys/class/drm/` |
| **Temperature** | SMC (Intel), IOHIDEventSystem (Apple Silicon) | lm-sensors, `/sys/class/thermal/`, `/sys/class/hwmon/` |
| **Battery** | IOKit: `IOPSCopyPowerSourcesInfo` | UPower DBus API, `/sys/class/power_supply/` |
| **AI Tokens** | `~/.claude/projects/*/sessionid.jsonl` (JSONL parsing) | Same (platform-independent) |
| **Weather/Stock** | HTTP APIs (platform-independent) | Same (platform-independent) |

**Graphics APIs:**
- **macOS**: Quartz 2D (Core Graphics) for vector rendering
- **Linux**: Cairo for vector rendering (similar capabilities)

**UI Frameworks:**
- **macOS**: Cocoa/AppKit with NIB files
- **Linux**: GTK3 with code-based UI

## Adding a New Module

### macOS Module Implementation

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

### Linux Module Implementation

Follow these steps to add a monitoring module to the Linux port:

**1. Create Data Collector**

File: `xrg-linux/src/collectors/yourmodule_collector.h`
```c
#ifndef YOURMODULE_COLLECTOR_H
#define YOURMODULE_COLLECTOR_H

#include "core/dataset.h"

typedef struct {
    XRGDataSet *dataset;
    // Add fields for metric tracking
} YourModuleCollector;

YourModuleCollector* yourmodule_collector_new(void);
void yourmodule_collector_update(YourModuleCollector *collector);
void yourmodule_collector_free(YourModuleCollector *collector);

#endif
```

File: `xrg-linux/src/collectors/yourmodule_collector.c`
```c
#include "yourmodule_collector.h"
#include <stdio.h>

YourModuleCollector* yourmodule_collector_new(void) {
    YourModuleCollector *collector = g_new0(YourModuleCollector, 1);
    collector->dataset = xrg_dataset_new(300); // 300 samples = 5 minutes at 1Hz
    return collector;
}

void yourmodule_collector_update(YourModuleCollector *collector) {
    // Read from /proc or /sys
    // Parse data
    // Add to dataset
    xrg_dataset_add_value(collector->dataset, new_value);
}

void yourmodule_collector_free(YourModuleCollector *collector) {
    if (collector->dataset) xrg_dataset_free(collector->dataset);
    g_free(collector);
}
```

**2. Create Graph Widget**

File: `xrg-linux/src/widgets/yourmodule_widget.h`
```c
#ifndef YOURMODULE_WIDGET_H
#define YOURMODULE_WIDGET_H

#include <gtk/gtk.h>
#include "collectors/yourmodule_collector.h"

GtkWidget* yourmodule_widget_new(YourModuleCollector *collector);

#endif
```

File: `xrg-linux/src/widgets/yourmodule_widget.c`
```c
#include "yourmodule_widget.h"
#include <cairo.h>

typedef struct {
    YourModuleCollector *collector;
} YourModuleWidgetPrivate;

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    YourModuleWidgetPrivate *priv = user_data;

    // Draw background
    cairo_set_source_rgba(cr, 0.05, 0.08, 0.15, 0.95);
    cairo_paint(cr);

    // Draw graph using Cairo (similar to macOS Quartz)
    // See graph_widget.c for helper functions

    return FALSE;
}

GtkWidget* yourmodule_widget_new(YourModuleCollector *collector) {
    GtkWidget *widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(widget, 200, 60);

    YourModuleWidgetPrivate *priv = g_new0(YourModuleWidgetPrivate, 1);
    priv->collector = collector;

    g_signal_connect(widget, "draw", G_CALLBACK(on_draw), priv);

    return widget;
}
```

**3. Register Module**

In `xrg-linux/src/ui/main_window.c`:
```c
// Add collector
YourModuleCollector *yourmodule_collector = yourmodule_collector_new();

// Add widget
GtkWidget *yourmodule_widget = yourmodule_widget_new(yourmodule_collector);
gtk_box_pack_start(GTK_BOX(modules_box), yourmodule_widget, FALSE, FALSE, 0);

// Add update timer
g_timeout_add(1000, (GSourceFunc)yourmodule_update_callback, yourmodule_collector);
```

**4. Update CMakeLists.txt**

Add to `xrg-linux/CMakeLists.txt`:
```cmake
set(COLLECTOR_SOURCES
    # ... existing collectors ...
    src/collectors/yourmodule_collector.c
)

set(WIDGET_SOURCES
    # ... existing widgets ...
    src/widgets/yourmodule_widget.c
)
```

**5. Build and Test**

```bash
cd xrg-linux/build
cmake ..
make -j$(nproc)
./xrg-linux
```

## Common Development Tasks

### Adding User Preferences

**macOS:**

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

**Linux:**

Settings are stored in `~/.config/xrg-linux/settings.conf` using GLib's `GKeyFile`:

```c
// In xrg-linux/src/core/preferences.c

// Save setting
void preferences_set_bool(const char *key, gboolean value) {
    GKeyFile *keyfile = g_key_file_new();
    g_key_file_set_boolean(keyfile, "General", key, value);
    // ... save to file ...
}

// Load setting
gboolean preferences_get_bool(const char *key, gboolean default_value) {
    GKeyFile *keyfile = g_key_file_new();
    // ... load from file ...
    return g_key_file_get_boolean(keyfile, "General", key, NULL);
}
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

## AI Token Monitoring

AI token tracking works on both platforms by parsing `~/.claude/projects/*/sessionid.jsonl`. Zero configuration required.

**Data Sources** (auto-detected in priority order):
1. JSONL transcripts (universal, default)
2. SQLite database `~/.claude/monitoring/claude_usage.db` (advanced)
3. OpenTelemetry endpoint (advanced)

**Key Files**:
- macOS: `Data Miners/XRGAITokenMiner.m`, `Graph Views/XRGAITokenView.m`
- Linux: `xrg-linux/src/collectors/aitoken_collector.c`

**JSONL Parsing Note**: Model field is in `message.model`, not root level:
```json
{"message": {"model": "claude-sonnet-4-5-20250929", "usage": {"input_tokens": 10, "output_tokens": 791}}}
```

**Settings** (macOS - `XRGAISettingsKeys.h`):
- `aiTokensTrackingEnabled`: Master toggle (must be YES for Observer)
- `aiTokensAggregateByModel`: Per-model breakdown
- `aiTokensDailyBudget`: Daily limit (0 = unlimited)

## Platform Requirements

**macOS**: 10.13+, Xcode, Cocoa/IOKit/Accelerate frameworks
**Linux**: GTK 3.24+, Cairo 1.16+, GLib 2.66+, SQLite3, libcurl, json-glib, CMake 3.16+
