XRG
===

<img width="1080" height="188" alt="AI-Tokens" src="https://github.com/user-attachments/assets/bfb873ea-6564-437f-af2c-315b14b3dfb0" />


A real-time system resource monitor for **macOS and Linux** with customizable floating graphs.

🍎 **macOS**: Native Objective-C/Cocoa implementation
🐧 **Linux**: Native C/GTK3 implementation



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
- **AI Token Usage** - Live token consumption for Claude, OpenAI Codex, and Gemini CLI

## AI Token Monitoring

**New in this fork**: Real-time monitoring of AI token usage across multiple AI coding assistants.

### Supported Providers

| Provider | Data Source | Auto-Detected |
|----------|-------------|---------------|
| **Claude Code** | `~/.claude/projects/*/sessionid.jsonl` | ✓ |
| **OpenAI Codex CLI** | `~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl` | ✓ |
| **Google Gemini CLI** | `~/.gemini/tmp/<hash>/chats/session-*.json` | ✓ |

### Features

- **Multi-Provider** - Track Claude, Codex, and Gemini tokens in a single stacked graph
- **Zero Configuration** - Auto-detects installed AI tools, no setup required
- **Real-Time Tracking** - Updates within 1-2 seconds during active AI usage
- **Per-Provider Breakdown** - Color-coded visualization shows usage by provider
- **Performance Optimized** - Background threading, intelligent caching, no UI freezing

### How It Works

The AI Token monitor automatically detects all installed AI coding tools:

1. **Claude Code** (Anthropic)
   - Source: `~/.claude/projects/*/sessionid.jsonl`
   - Fallback: SQLite database or OpenTelemetry endpoint

2. **OpenAI Codex CLI**
   - Source: `~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl`
   - Parses `token_count` events from session files

3. **Google Gemini CLI**
   - Source: `~/.gemini/tmp/<hash>/chats/session-*.json`
   - Extracts tokens from message history

### Quick Start

1. Build and run XRG
2. Open Preferences → Graphs
3. Enable "Show AI Tokens"
4. Use Claude Code in another terminal
5. Watch live token consumption in the graph

For detailed documentation, see [README_AI_TOKENS.md](README_AI_TOKENS.md)

## Building

### macOS

**Requirements:**
- macOS 10.13 or later
- Xcode with Command Line Tools

**Build Commands:**

```bash
# Build
xcodebuild -project XRG.xcodeproj -scheme XRG build

# Clean and rebuild
xcodebuild -project XRG.xcodeproj -scheme XRG clean build

# Open in Xcode
open XRG.xcodeproj
```

### Linux

**Requirements:**
- GTK 3.24+
- Cairo 1.16+
- GLib 2.66+
- json-glib
- CMake 3.16+
- C compiler (gcc/clang)

**Build Commands:**

```bash
cd xrg-linux
mkdir build && cd build
cmake ..
make -j$(nproc)
./xrg-linux
```

**Install System-Wide:**

```bash
sudo make install
```

For detailed Linux documentation, see [xrg-linux/README.md](xrg-linux/README.md)

## License

GNU General Public License v2.0 - see LICENSE file for details

## Credits

- Original XRG by Gaucho Software, LLC
- AI Token Monitoring by marc-shade (2025)
