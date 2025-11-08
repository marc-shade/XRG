# XRG Live AI Token Tracking - COMPLETE

## ✅ Implementation Complete

XRG now tracks ALL Claude Code conversation tokens in real-time!

## How It Works

```
Claude Code → Session JSONL → Token Extractor → Log File → Metrics Exporter → OTel → XRG
```

### Components Running

1. **Token Extractor** (PID 90475)
   - Monitors: `/Users/marc/.claude/projects/-Volumes-FILES-code-XRG/fd50730d-e9bb-4733-99ec-9f1bbdb7e24d.jsonl`
   - Logs to: `~/.claude/logs/claude-code.log`
   - Installed: `~/.local/bin/claude-token-extractor`

2. **Metrics Exporter** (PID 17442)
   - Reads: `~/.claude/logs/claude-code.log`
   - Exposes: `http://localhost:4318/v1/metrics`
   - Updates: Every 500ms

3. **XRG** (PID 79881)
   - Polls: OTel endpoint every 1 second
   - Displays: Real-time token graphs, totals, cost

## Usage

### Start Token Extractor (per project)
```bash
cd /your/project/directory
python3 ~/.local/bin/claude-token-extractor &
```

### Verify It's Working
```bash
# Check extractor
ps aux | grep claude_token_extractor

# Check log file
tail -f ~/.claude/logs/claude-code.log

# Check metrics
curl -s http://localhost:4318/v1/metrics | grep claude

# Check XRG
# Look for "AI Token" module showing real-time graphs
```

## Data Flow

Claude Code writes to session JSONL:
```json
{"usage": {"input_tokens": 1234, "output_tokens": 567, "cache_read_input_tokens": 12249}}
```

Extractor logs to file:
```
Thu Nov  7 12:52:00 EST 2025: Token usage: input=1234, output=567, cache=12249, service=claude
```

Metrics exporter exposes:
```
claude_code_token_usage{type="input"} 29580
claude_code_token_usage{type="output"} 13928
claude_code_cost_usage 0.2977
```

XRG displays:
- Real-time stacked area graphs
- Total: 43K tokens
- Cost: $0.30

## Files

- Extractor: `~/.local/bin/claude-token-extractor`
- Log: `~/.claude/logs/claude-code.log`
- Session JSONL: `~/.claude/projects/-Volumes-{path}/{session-id}.jsonl`

## Status

**OPERATIONAL** - Tracking THIS conversation's tokens in real-time!

Implementation completed: November 7, 2025
