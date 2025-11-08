XRG
===

A real-time system resource monitor for macOS with customizable floating graphs.



Website: https://gaucho.software/xrg/

## Features

XRG displays real-time graphs for:

- **CPU Usage** - Per-core utilization and load averages
- **GPU Usage** - GPU utilization and memory (Apple Silicon & Intel)
- **Memory** - RAM usage, swap, page ins/outs
- **Network** - Upload/download rates per interface
- **Disk Activity** - Read/write operations per volume
- **Battery** - Charge level, time remaining, health
- **Temperature** - CPU, GPU, and system sensors
- **Weather** - Current conditions and forecast
- **Stock Prices** - Real-time market data
- **AI Token Usage** - Live AI API token consumption (NEW)

## AI Token Monitoring

**New in this fork**: Real-time monitoring of AI token usage for Claude Code and other AI services.

### Features

- **Universal Compatibility** - Works on ALL Macs with default Claude Code installation
- **Zero Configuration** - No setup required, works out of the box
- **Real-Time Tracking** - Updates within 1-2 seconds during active AI usage
- **Multi-Strategy** - Automatic fallback between JSONL transcripts, SQLite database, and OpenTelemetry
- **Performance Optimized** - Background threading, intelligent caching, no UI freezing

### How It Works

The AI Token monitor automatically detects and uses the best available data source:

1. **JSONL Transcripts** (Primary - Universal)
   - Source: `~/.claude/projects/*/sessionid.jsonl`
   - Works with default Claude Code installation
   - No configuration required

2. **SQLite Database** (Fallback - Advanced)
   - Source: `~/.claude/monitoring/claude_usage.db`
   - For users with custom monitoring setups

3. **OpenTelemetry** (Fallback - Advanced)
   - Source: `http://localhost:8889/metrics`
   - For users with OTel configured

### Quick Start

1. Build and run XRG
2. Open Preferences → Graphs
3. Enable "Show AI Tokens"
4. Use Claude Code in another terminal
5. Watch live token consumption in the graph

For detailed documentation, see [README_AI_TOKENS.md](README_AI_TOKENS.md)

## Building

### Requirements

- macOS 10.13 or later
- Xcode with Command Line Tools

### Build Commands

```bash
# Build
xcodebuild -project XRG.xcodeproj -scheme XRG build

# Clean and rebuild
xcodebuild -project XRG.xcodeproj -scheme XRG clean build

# Open in Xcode
open XRG.xcodeproj
```

## License

GNU General Public License v2.0 - see LICENSE file for details

## Credits

- Original XRG by Gaucho Software, LLC
- AI Token Monitoring by marc-shade (2025)
