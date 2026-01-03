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
- **AI Token Usage** - Live token consumption and cost tracking for Claude, Codex, Gemini, and Ollama

## AI Token Monitoring

**New in this fork**: Real-time monitoring of AI token usage and costs across multiple AI providers.

### Supported Providers

| Provider | Data Source | Type | Auto-Detected |
|----------|-------------|------|---------------|
| **Claude Code** | `~/.claude/projects/*/sessionid.jsonl` | Cloud API | ✓ |
| **OpenAI Codex CLI** | `~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl` | Cloud API | ✓ |
| **Google Gemini CLI** | `~/.gemini/tmp/<hash>/chats/session-*.json` | Cloud API | ✓ |
| **Ollama** | REST API (localhost:11434) | Local Inference | ✓ |

### Features

- **4-Provider Support** - Track Claude, Codex, Gemini, and Ollama in a single stacked graph
- **Cost Intelligence** - Real-time $/hour burn rate and projected daily costs
- **Per-Provider Costs** - Accurate pricing (Claude $3/$15, Codex $2.50/$10, Gemini $0.15/$0.60 per MTok)
- **Ollama Integration** - Detect available models, show running models, track local inference
- **Zero Configuration** - Auto-detects installed AI tools, no setup required
- **Real-Time Tracking** - Updates within 1-2 seconds during active AI usage
- **Per-Provider Breakdown** - Color-coded visualization shows usage by provider
- **Performance Optimized** - Background threading, intelligent caching, 95MB memory footprint

### How It Works

The AI Token monitor automatically detects all installed AI coding tools:

1. **Claude Code** (Anthropic) - Cloud API
   - Source: `~/.claude/projects/*/sessionid.jsonl`
   - Tracks input/output tokens with accurate cost calculation
   - Pricing: $3/MTok input, $15/MTok output

2. **OpenAI Codex CLI** - Cloud API
   - Source: `~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl`
   - Parses `token_count` events from session files
   - Pricing: $2.50/MTok input, $10/MTok output

3. **Google Gemini CLI** - Cloud API
   - Source: `~/.gemini/tmp/<hash>/chats/session-*.json`
   - Extracts tokens from message history
   - Pricing: $0.15/MTok input, $0.60/MTok output

4. **Ollama** - Local Inference
   - Source: REST API at `http://localhost:11434`
   - Queries `/api/tags` for available models
   - Queries `/api/ps` for currently loaded models
   - Zero cost (local inference)

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
