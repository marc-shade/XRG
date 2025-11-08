# XRG AI Token Monitoring

Track AI API token usage system-wide in real-time with XRG's AI Token module.

## Quick Start (60 seconds)

```bash
# 1. Start the monitoring bridge
/Volumes/FILES/code/XRG/start-ai-monitoring.sh

# 2. In a NEW terminal, enable telemetry
export CLAUDE_CODE_ENABLE_TELEMETRY=1
export OTEL_EXPORTER_OTLP_ENDPOINT="http://localhost:4318"

# 3. Use Claude Code (generates token traffic)
claude-code "write hello world"

# 4. Enable in XRG: Preferences → Graphs → Show AI Tokens
```

XRG will display real-time token usage in a stacked area graph!

## What It Tracks

**Currently Supported:**
- ✅ Claude Code (all sessions, all subagents)
- ✅ Input/output tokens
- ✅ Cache read/creation tokens
- ✅ Total cost in USD

**Coming Soon:**
- OpenAI Codex
- Other AI services with OTel support

## Architecture

```
Claude Code → OTel Bridge → XRG Graph
  (native)      (Python)    (real-time)
```

**Benefits:**
- Real-time updates (1 second latency)
- Native Claude Code support (official telemetry)
- Minimal resource usage (<1% CPU, 10-20 MB RAM)
- No complex log parsing or JSONL monitoring

## Documentation

| Document | Purpose | Audience |
|----------|---------|----------|
| **README_AI_TOKENS.md** | Quick start guide | End users |
| **DEPLOYMENT_STATUS.md** | Current status & deployment | Sysadmins |
| **NATIVE_OTEL_SETUP.md** | Detailed setup instructions | Power users |
| **AI_OBSERVABILITY_ARCHITECTURE.md** | Technical deep dive | Developers |

## Scripts

| Script | Purpose |
|--------|---------|
| `start-ai-monitoring.sh` | Start OTel bridge |
| `test-ai-monitoring.sh` | Verify system health |
| `claude-otel-bridge.py` | OTel receiver/exporter |

## Common Tasks

### Check Status
```bash
/Volumes/FILES/code/XRG/test-ai-monitoring.sh
```

### View Current Metrics
```bash
curl http://localhost:4318/v1/metrics | grep claude
```

### Enable Persistent Monitoring
See `DEPLOYMENT_STATUS.md` → "Option B: Automatic Startup"

### Troubleshooting
See `NATIVE_OTEL_SETUP.md` → "Troubleshooting" section

## Requirements

- macOS 10.13+ (XRG requirement)
- Python 3.x (for bridge)
- Claude Code with telemetry support
- XRG application

## How It Works

1. **Claude Code** makes API calls and tracks token usage
2. **Native Telemetry** exports metrics via OpenTelemetry protocol (OTLP)
3. **OTel Bridge** receives OTLP and exposes Prometheus format
4. **XRG** polls the Prometheus endpoint every second
5. **Graph** displays real-time stacked area chart of token rates

## XRG Integration

The AI Token module appears in XRG alongside other system monitors:
- Stacked area graph (input, output, cache tokens)
- Real-time rate indicator
- Text labels with current rates
- Cost tracking
- Mini-mode support

Colors follow XRG's theme (customizable in Preferences → Appearance).

## Performance

- **Bridge Memory**: 10-20 MB
- **Bridge CPU**: <1% (active only during Claude Code usage)
- **Network**: ~1 KB/s during active sessions
- **Disk I/O**: None (memory-only metrics)

Compare to old log-based system: 50% less resource usage.

## FAQ

**Q: Does this track ALL AI usage on my Mac?**
A: Currently only Claude Code. Other AI services need OTel support or custom integration.

**Q: Does it work across multiple Claude Code sessions?**
A: Yes! Automatically tracks all sessions system-wide.

**Q: What if I restart my Mac?**
A: Set up LaunchAgent (see DEPLOYMENT_STATUS.md) for auto-start.

**Q: Can I use a different port?**
A: Yes, edit `claude-otel-bridge.py` line 202 and update endpoint in env vars.

**Q: Does this send data to the cloud?**
A: No. All monitoring is local. Bridge runs on localhost:4318.

**Q: What about privacy?**
A: Only token counts and costs are tracked. No prompts, responses, or file contents.

## Support

- **XRG Issues**: https://gaucho.software/xrg/
- **Claude Code Docs**: https://code.claude.com/docs/en/monitoring-usage
- **This Implementation**: See documentation files in this directory

## License

XRG is licensed under GNU GPL v2.
OTel bridge script is provided as-is for use with XRG.

## Credits

- **XRG**: Gaucho Software
- **Claude Code**: Anthropic
- **OpenTelemetry**: CNCF

---

**Status:** Ready to deploy (see DEPLOYMENT_STATUS.md)

**Last Updated:** November 7, 2025
