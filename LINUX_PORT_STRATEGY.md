# XRG Linux Port Strategy

## Executive Summary

This document outlines the strategy for porting XRG (macOS system resource monitor) to Linux while maintaining the same look, feel, and functionality including AI token monitoring.

**Target Name:** XRG-Linux (working title)
**Primary Language:** C with GObject (GTK's object system)
**UI Framework:** GTK 3/4
**Graphics:** Cairo
**Build System:** CMake

## Architecture Overview

### Original macOS Architecture

XRG uses a three-layer architecture:

1. **Data Miner Layer** - Collects system metrics via IOKit, sysctl, SMC
2. **View Layer** - Renders graphs using Quartz (Cocoa/AppKit)
3. **Module Registration** - Orchestrates modules via XRGModuleManager

### Linux Port Architecture

We'll maintain the same three-layer pattern:

1. **Data Collector Layer** - Collects metrics via /proc, /sys, sysctl, lm-sensors
2. **Graph Widget Layer** - Renders graphs using Cairo (GTK)
3. **Module Manager** - Orchestrates modules (same pattern)

## Technology Mapping

### UI Framework

| macOS (Original) | Linux (Port) |
|------------------|--------------|
| Cocoa/AppKit | GTK 3 or GTK 4 |
| NSWindow | GtkWindow |
| NSView | GtkWidget/GtkDrawingArea |
| NSTimer | GLib main loop / g_timeout_add() |
| Quartz 2D | Cairo |
| NIB files | GtkBuilder XML or code-based UI |

### Data Collection APIs

| macOS API | Linux Equivalent | Notes |
|-----------|------------------|-------|
| IOKit | /proc, /sys | Kernel interfaces |
| sysctl | sysctl (similar) | Different syscalls |
| SMC (System Management Controller) | lm-sensors | Hardware sensors |
| IOAccelerator (GPU) | NVML (NVIDIA), libdrm | GPU-specific |
| Battery (IOKit) | UPower DBus API | Power management |
| Network (ioctl SIOCGIFDATA) | /proc/net/dev, netlink | Interface stats |

### Build System

| macOS | Linux |
|-------|-------|
| Xcode project | CMakeLists.txt |
| xcodebuild | cmake + make/ninja |
| .app bundle | /usr/bin binary + /usr/share assets |

## Module-by-Module Port Plan

### 1. CPU Monitor (XRGCPUMiner → xrg_cpu_collector)

**macOS Data Source:**
- IOKit: `IOServiceGetMatchingService`, `IOServiceMatching`
- sysctl: `host_processor_info()`, `vm_statistics64()`

**Linux Data Source:**
- `/proc/stat` - CPU usage per core
- `/proc/loadavg` - Load averages
- sysconf() - CPU count

**Complexity:** Medium (straightforward proc file parsing)

### 2. Memory Monitor (XRGMemoryMiner → xrg_memory_collector)

**macOS Data Source:**
- `host_statistics64()`, `vm_statistics64()`
- Mach VM subsystem

**Linux Data Source:**
- `/proc/meminfo` - MemTotal, MemFree, MemAvailable, SwapTotal, SwapFree
- `/proc/vmstat` - Page ins/outs

**Complexity:** Low (simple proc file parsing)

### 3. Network Monitor (XRGNetMiner → xrg_network_collector)

**macOS Data Source:**
- `ioctl` with `SIOCGIFDATA`
- Interface enumeration

**Linux Data Source:**
- `/proc/net/dev` - Interface statistics
- `/sys/class/net/*/statistics/` - Per-interface stats
- Netlink sockets (advanced)

**Complexity:** Medium (need interface detection and delta calculations)

### 4. Disk Monitor (XRGDiskMiner → xrg_disk_collector)

**macOS Data Source:**
- IOKit disk statistics

**Linux Data Source:**
- `/proc/diskstats` - Per-device read/write stats
- `/sys/block/*/stat` - Device statistics

**Complexity:** Medium (device naming and filesystem mapping)

### 5. GPU Monitor (XRGGPUMiner → xrg_gpu_collector)

**macOS Data Source:**
- IOAccelerator, IOGraphics APIs
- Apple Silicon: HID sensors

**Linux Data Source:**
- **NVIDIA:** NVML library (`nvidia-ml.h`)
- **AMD:** libdrm, sysfs (`/sys/class/drm/`)
- **Intel:** i915 driver, sysfs

**Complexity:** High (vendor-specific, multiple codepaths)

### 6. Temperature Monitor (XRGTemperatureMiner → xrg_sensors_collector)

**macOS Data Source:**
- SMC (System Management Controller)
- Apple Silicon: IOHIDEventSystem

**Linux Data Source:**
- lm-sensors library (`sensors.h`)
- `/sys/class/thermal/` - Thermal zones
- `/sys/class/hwmon/` - Hardware monitoring

**Complexity:** Medium (lm-sensors integration, sensor enumeration)

### 7. Battery Monitor (XRGBatteryMiner → xrg_battery_collector)

**macOS Data Source:**
- IOKit: `IOPSCopyPowerSourcesInfo`

**Linux Data Source:**
- UPower DBus API (`org.freedesktop.UPower`)
- `/sys/class/power_supply/` - Direct sysfs access

**Complexity:** Medium (DBus integration or sysfs parsing)

### 8. AI Token Monitor (XRGAITokenMiner → xrg_aitoken_collector)

**macOS Data Source:**
- JSONL: `~/.claude/projects/*/sessionid.jsonl`
- SQLite: `~/.claude/monitoring/claude_usage.db`
- OpenTelemetry: `http://localhost:8889/metrics`

**Linux Data Source:**
- **Same paths and strategies work on Linux!**
- JSONL parsing (same format)
- SQLite (cross-platform)
- HTTP requests to OTel endpoint

**Complexity:** Low (mostly platform-independent, just need JSON/SQLite/HTTP libraries)

### 9. Weather & Stock (Network-based)

**Data Source:** Web APIs (platform-independent)

**Complexity:** Low (just need HTTP client library)

## UI/UX Design

### Window Design

XRG features a **semi-transparent floating window** with:
- No window decorations (frameless)
- Always-on-top behavior
- Click-through capability (optional)
- Rounded corners
- Draggable anywhere

**GTK Implementation:**
```c
GtkWindow *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
gtk_window_set_decorated(window, FALSE);
gtk_window_set_keep_above(window, TRUE);
gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DOCK);

// Transparency via CSS or window attributes
```

### Graph Rendering

**macOS:** Quartz 2D (Core Graphics)
**Linux:** Cairo

Both are vector graphics APIs with similar capabilities:
- Path drawing
- Gradients and fills
- Transparency
- Text rendering

**Example Cairo graph:**
```c
void draw_graph(cairo_t *cr, double *values, int count) {
    cairo_set_source_rgba(cr, 0.2, 0.8, 0.9, 0.8); // Cyan
    cairo_move_to(cr, 0, height);

    for (int i = 0; i < count; i++) {
        double x = (double)i / count * width;
        double y = height - (values[i] * height);
        cairo_line_to(cr, x, y);
    }

    cairo_line_to(cr, width, height);
    cairo_close_path(cr);
    cairo_fill(cr);
}
```

### Color Scheme

XRG uses a dark theme with customizable colors:
- Background: Dark gray (0.1, 0.1, 0.1, 0.9)
- Graph BG: Slightly lighter (0.15, 0.15, 0.15, 0.9)
- FG1: Cyan (0.2, 0.8, 0.9)
- FG2: Purple (0.7, 0.3, 0.9)
- FG3: Amber (1.0, 0.7, 0.2)

**Linux:** Same colors, stored in GSettings or config file

### Module Layout

Modules stack vertically or horizontally based on user preference:
- Each module is a GtkDrawingArea
- GtkBox container with expand/fill properties
- Module manager tracks order and visibility

## Build System

### CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.16)
project(xrg-linux VERSION 1.0.0 LANGUAGES C)

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-glib-1.0)
pkg_check_modules(SENSORS libsensors) # Optional
pkg_check_modules(UPOWER upower-glib) # Optional

# Source files
add_executable(xrg-linux
    src/main.c
    src/collectors/cpu_collector.c
    src/collectors/memory_collector.c
    src/collectors/network_collector.c
    src/collectors/disk_collector.c
    src/collectors/gpu_collector.c
    src/collectors/sensors_collector.c
    src/collectors/battery_collector.c
    src/collectors/aitoken_collector.c
    src/widgets/graph_widget.c
    src/widgets/cpu_widget.c
    src/widgets/memory_widget.c
    src/widgets/network_widget.c
    src/widgets/disk_widget.c
    src/widgets/gpu_widget.c
    src/widgets/sensors_widget.c
    src/widgets/battery_widget.c
    src/widgets/aitoken_widget.c
    src/core/module_manager.c
    src/core/dataset.c
    src/core/preferences.c
    src/ui/main_window.c
    src/ui/preferences_window.c
)

target_link_libraries(xrg-linux
    ${GTK_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
    ${SENSORS_LIBRARIES}
    ${UPOWER_LIBRARIES}
    m # math library
)

install(TARGETS xrg-linux DESTINATION bin)
install(FILES xrg-linux.desktop DESTINATION share/applications)
install(FILES icons/xrg-linux.svg DESTINATION share/icons/hicolor/scalable/apps)
```

## Directory Structure

```
xrg-linux/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── src/
│   ├── main.c
│   ├── collectors/           # Data collection layer
│   │   ├── cpu_collector.c/h
│   │   ├── memory_collector.c/h
│   │   ├── network_collector.c/h
│   │   ├── disk_collector.c/h
│   │   ├── gpu_collector.c/h
│   │   ├── sensors_collector.c/h
│   │   ├── battery_collector.c/h
│   │   └── aitoken_collector.c/h
│   ├── widgets/              # Graph rendering layer
│   │   ├── graph_widget.c/h  (base class)
│   │   ├── cpu_widget.c/h
│   │   ├── memory_widget.c/h
│   │   ├── network_widget.c/h
│   │   ├── disk_widget.c/h
│   │   ├── gpu_widget.c/h
│   │   ├── sensors_widget.c/h
│   │   ├── battery_widget.c/h
│   │   └── aitoken_widget.c/h
│   ├── core/                 # Core infrastructure
│   │   ├── module_manager.c/h
│   │   ├── dataset.c/h       (ring buffer)
│   │   ├── preferences.c/h
│   │   └── utils.c/h
│   └── ui/                   # Main UI
│       ├── main_window.c/h
│       └── preferences_window.c/h
├── data/
│   ├── xrg-linux.desktop
│   ├── icons/
│   └── gresources.xml        (optional: embedded resources)
└── tests/
    ├── test_collectors.c
    └── test_widgets.c
```

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1)
- [ ] Set up CMake build system
- [ ] Implement GObject-based module system
- [ ] Create dataset (ring buffer) implementation
- [ ] Basic GTK window with transparent background

### Phase 2: Essential Data Collectors (Week 2)
- [ ] CPU collector (/proc/stat)
- [ ] Memory collector (/proc/meminfo)
- [ ] Network collector (/proc/net/dev)
- [ ] Disk collector (/proc/diskstats)

### Phase 3: Graph Rendering (Week 3)
- [ ] Base graph widget with Cairo
- [ ] CPU graph widget
- [ ] Memory graph widget
- [ ] Network graph widget
- [ ] Disk graph widget

### Phase 4: Advanced Collectors (Week 4)
- [ ] GPU collector (NVIDIA/AMD/Intel)
- [ ] Temperature/Sensors collector (lm-sensors)
- [ ] Battery collector (UPower)
- [ ] AI Token collector (JSONL/SQLite/OTel)

### Phase 5: Advanced UI (Week 5)
- [ ] Preferences window
- [ ] Settings persistence (GSettings or config file)
- [ ] Module visibility controls
- [ ] Color customization

### Phase 6: Polish & Testing (Week 6)
- [ ] Weather module (network API)
- [ ] Stock module (network API)
- [ ] Performance optimization
- [ ] Comprehensive testing
- [ ] Documentation
- [ ] Packaging (DEB, RPM, AppImage)

## Dependencies

### Required
- GTK 3.24+ or GTK 4.x
- Cairo 1.16+
- GLib 2.66+
- SQLite3
- libcurl
- json-glib

### Optional
- lm-sensors (temperature monitoring)
- libupower-glib (battery monitoring)
- nvidia-ml (NVIDIA GPU monitoring)
- libdrm (AMD/Intel GPU monitoring)

## AI Token Monitoring Implementation

The AI Token monitor is largely **platform-independent**:

### Strategy 1: JSONL Parsing (Universal)
```c
// Read ~/.claude/projects/*/sessionid.jsonl
// Parse JSON lines for token usage
// Libraries: json-glib or cJSON

typedef struct {
    char *model;
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
    double timestamp;
} TokenUsageEvent;

GPtrArray* parse_jsonl_file(const char *path);
```

### Strategy 2: SQLite Database
```c
// Read ~/.claude/monitoring/claude_usage.db
// Query session table
// Library: sqlite3

int query_token_usage(const char *db_path, int64_t *total_tokens);
```

### Strategy 3: OpenTelemetry HTTP
```c
// GET http://localhost:8889/metrics
// Parse Prometheus-style metrics
// Library: libcurl

int fetch_otel_metrics(const char *endpoint, double *token_rate);
```

All three strategies work identically on Linux and macOS!

## Distribution

### Packaging Options

1. **DEB (Debian/Ubuntu)**
   - Use debhelper
   - Package name: `xrg-linux`
   - Depends: libgtk-3-0, libcairo2, libsqlite3-0, libcurl4

2. **RPM (Fedora/RHEL)**
   - Use rpmbuild
   - Package name: `xrg-linux`
   - Requires: gtk3, cairo, sqlite-libs, libcurl

3. **AppImage (Universal)**
   - Bundle all dependencies
   - Single executable file
   - Works on all Linux distributions

4. **Flatpak (Universal)**
   - Sandboxed application
   - Flathub distribution

### Desktop Integration

**xrg-linux.desktop:**
```ini
[Desktop Entry]
Name=XRG System Monitor
Comment=Real-time system resource monitor with floating graphs
Exec=xrg-linux
Icon=xrg-linux
Terminal=false
Type=Application
Categories=System;Monitor;
StartupNotify=false
```

## Performance Considerations

### Update Timers
- Fast update (100ms): CPU, Network (active monitoring)
- Normal update (1s): Memory, Disk, GPU
- Slow update (5s): Temperature, Battery
- Very slow update (5min): Weather, Stocks

### Memory Efficiency
- Ring buffer for time-series data (fixed size, ~1KB per dataset)
- Graph caching (only redraw on data change)
- Lazy loading of optional modules (GPU, sensors)

### CPU Efficiency
- Use GLib main loop (event-driven, not polling)
- Batch reads from /proc and /sys
- Minimize string allocations (pre-allocated buffers)

## Testing Strategy

### Unit Tests
- Test each collector independently
- Mock /proc and /sys data
- Verify calculations (rates, deltas, percentages)

### Integration Tests
- Test full application startup
- Verify module registration
- Test preferences persistence

### Visual Tests
- Screenshot comparison with macOS XRG
- Verify graph rendering accuracy
- Test on different display scales (HiDPI)

## Compatibility Matrix

| Feature | Desktops | Laptops | Servers |
|---------|----------|---------|---------|
| CPU Monitor | ✓ | ✓ | ✓ |
| Memory Monitor | ✓ | ✓ | ✓ |
| Network Monitor | ✓ | ✓ | ✓ |
| Disk Monitor | ✓ | ✓ | ✓ |
| GPU Monitor | ✓ (if GPU) | ✓ (if dGPU) | ✓ (if GPU) |
| Temperature Monitor | ✓ | ✓ | ✓ |
| Battery Monitor | ✗ | ✓ | ✗ |
| AI Token Monitor | ✓ | ✓ | ✓ |
| Weather | ✓ | ✓ | ✓ |
| Stocks | ✓ | ✓ | ✓ |

## Success Criteria

The Linux port is complete when:

1. **Visual Parity:** Looks identical to macOS XRG
2. **Functional Parity:** All modules work (CPU, Memory, Network, Disk, GPU, Temp, Battery, AI Tokens)
3. **Performance:** Uses <1% CPU when idle, <50MB RAM
4. **Stability:** Runs for days without crashes or memory leaks
5. **Distribution:** Available as DEB, RPM, and AppImage
6. **Documentation:** Comprehensive README and man page

## Timeline

**Total Estimated Time:** 6 weeks (1 developer, full-time)

- Week 1: Infrastructure and build system
- Week 2: Core data collectors
- Week 3: Graph rendering
- Week 4: Advanced collectors (GPU, sensors, battery, AI tokens)
- Week 5: UI polish and preferences
- Week 6: Testing, documentation, packaging

## Next Steps

1. Create project skeleton with CMake
2. Implement basic GTK window with transparency
3. Port CPU collector as proof-of-concept
4. Implement base graph widget with Cairo
5. Iterate on remaining modules

---

**Document Version:** 1.0
**Date:** 2025-11-13
**Author:** Claude Code (Anthropic)
