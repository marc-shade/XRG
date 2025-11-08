# OpenTelemetry AI Token Monitoring - Deployment Complete

## Status: ✅ FULLY OPERATIONAL

**Date**: 2025-11-07 20:47
**Version**: Native OTel with Official Collector

## System Architecture

```
Claude Code (native OTel)
    ↓ gRPC (port 4317)
OTel Collector (official binary)
    ↓ Prometheus format (port 8889)
XRG AI Token Module
    ↓ Real-time display
User (1-second refresh rate)
```

## Active Components

### 1. OpenTelemetry Collector
- **Binary**: `~/.local/bin/otel/otelcol-contrib` (v0.114.0, ARM64)
- **Config**: `/Volumes/FILES/code/XRG/otel-collector-xrg.yaml`
- **Process**: PID 6142
- **Ports**:
  - 4317: OTLP gRPC receiver (Claude Code → Collector)
  - 4318: OTLP HTTP receiver (backup)
  - 8889: Prometheus metrics exporter (Collector → XRG)

**Start Command**:
```bash
~/.local/bin/otel/otelcol-contrib \
  --config=/Volumes/FILES/code/XRG/otel-collector-xrg.yaml \
  > /tmp/otel-collector.log 2>&1 &
```

### 2. Claude Code Telemetry
- **Enabled**: Yes (via `~/.zshrc` configuration)
- **Protocol**: gRPC
- **Endpoint**: http://localhost:4317
- **Export Interval**: 10 seconds (metrics), 5 seconds (logs)

**Environment Variables** (`~/.zshrc` lines 279-297):
```bash
export CLAUDE_CODE_ENABLE_TELEMETRY=1
export OTEL_METRICS_EXPORTER=otlp
export OTEL_LOGS_EXPORTER=otlp
export OTEL_EXPORTER_OTLP_PROTOCOL=grpc
export OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4317
export OTEL_METRIC_EXPORT_INTERVAL=10000
export OTEL_LOGS_EXPORT_INTERVAL=5000
```

### 3. XRG Application
- **Binary**: Debug build (DerivedData)
- **Process**: PID 95067
- **Endpoint**: http://localhost:8889/metrics
- **Poll Interval**: 1 second (graphUpdate timer)
- **Module**: AI Tokens (display order 9)
- **Graph Enabled**: Yes (showAITokenGraph = 1)

**Key Files**:
- `Data Miners/XRGAITokenMiner.m` - Polls OTel, parses Prometheus format
- `Graph Views/XRGAITokenView.m` - Renders stacked area graph
- Default endpoint: `http://localhost:8889/metrics` (line 61)

## Metrics Being Tracked

### Current Session Metrics (as of 20:47)
```
Total Cost:     $0.79 USD
  - Sonnet 4.5: $0.766
  - Haiku 4.5:  $0.027

Token Usage:
  - Cache Read:      1,489,341 tokens (Sonnet)
  - Cache Creation:     10,768 tokens (Sonnet)
  - Input Tokens:       23,986 tokens (23,914 Haiku + 72 Sonnet)
  - Output Tokens:       4,070 tokens (647 Haiku + 3,423 Sonnet)
```

### Metric Format (Prometheus)
```prometheus
claude_code_token_usage_tokens_total{type="input"} VALUE TIMESTAMP
claude_code_token_usage_tokens_total{type="output"} VALUE TIMESTAMP
claude_code_token_usage_tokens_total{type="cacheRead"} VALUE TIMESTAMP
claude_code_token_usage_tokens_total{type="cacheCreation"} VALUE TIMESTAMP
claude_code_cost_usage_USD_total VALUE TIMESTAMP
```

## Code Implementation Details

### XRGAITokenMiner.m (Lines 55-62)
Endpoint configuration with preference override:
```objc
NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
NSString *customEndpoint = [defs stringForKey:XRG_aiTokenOTelEndpoint];
if (customEndpoint && [customEndpoint length] > 0) {
    otelMetricsURL = [customEndpoint copy];
} else {
    otelMetricsURL = @"http://localhost:8889/metrics";  // ✅ Correct port
}
```

### XRGAITokenMiner.m (Lines 183-188)
Metric parsing with camelCase support:
```objc
else if ([line containsString:@"cacheRead"] || [line containsString:@"cache_read"]) {
    cacheReadTokens += [self extractValueFromMetricLine:line];
}
else if ([line containsString:@"cacheCreation"] || [line containsString:@"cache_create"]) {
    cacheCreateTokens += [self extractValueFromMetricLine:line];
}
```

### Rate Calculation Pattern (Lines 111-118)
Critical: Calculate BEFORE updating lastCount:
```objc
currentClaudeRate = (UInt32)(_totalClaudeTokens - lastClaudeCount);
currentCodexRate = (UInt32)(_totalCodexTokens - lastCodexCount);
currentOtherRate = (UInt32)(_totalOtherTokens - lastOtherCount);

lastClaudeCount = _totalClaudeTokens;
lastCodexCount = _totalCodexTokens;
lastOtherCount = _totalOtherTokens;
```

## Verification Steps

### 1. Check OTel Collector
```bash
ps aux | grep otelcol-contrib
curl -s http://localhost:8889/metrics | grep claude_code
```

### 2. Verify Claude Code Telemetry
```bash
echo $CLAUDE_CODE_ENABLE_TELEMETRY  # Should be: 1
echo $OTEL_EXPORTER_OTLP_ENDPOINT   # Should be: http://localhost:4317
```

### 3. Test XRG
```bash
ps aux | grep XRG.app
defaults read com.gauchosoft.XRG showAITokenGraph  # Should be: 1
```

### 4. End-to-End Test
```bash
# Use Claude Code for a few seconds, then check metrics
curl -s http://localhost:8889/metrics | grep -A 2 claude_code_token_usage
```

## Startup Scripts

### Automatic Start (start-ai-monitoring.sh)
```bash
#!/bin/bash
# Kill existing processes
pkill -f otelcol-contrib

# Start OTel Collector
nohup ~/.local/bin/otel/otelcol-contrib \
  --config=/Volumes/FILES/code/XRG/otel-collector-xrg.yaml \
  > /tmp/otel-collector.log 2>&1 &

echo "✅ OTel Collector started (PID: $!)"
echo "Prometheus metrics: http://localhost:8889/metrics"
```

### Testing (test-ai-monitoring.sh)
```bash
#!/bin/bash
echo "🔍 Testing AI Token Monitoring System..."

# Test OTel Collector
if curl -s http://localhost:8889/metrics > /dev/null; then
    echo "✅ OTel Collector responding on port 8889"
else
    echo "❌ OTel Collector not responding"
fi

# Test metrics content
if curl -s http://localhost:8889/metrics | grep -q claude_code_token_usage; then
    echo "✅ Claude Code metrics found"
else
    echo "⚠️  No Claude Code metrics (session may not have started)"
fi
```

## Key Fixes Applied

1. **Port Correction**: Changed default from 4318 to 8889 (XRGAITokenMiner.m:61)
2. **CamelCase Parsing**: Added support for `cacheRead`/`cacheCreation` (XRGAITokenMiner.m:183-188)
3. **gRPC Protocol**: Switched from `http/json` to `grpc` (.zshrc:284)
4. **Fast Export**: Reduced interval from 60s to 10s for testing (.zshrc:288-289)

## Known Issues & Solutions

### Issue: Metrics showing 0
**Solution**: Ensure Claude Code session started AFTER telemetry env vars were loaded. Restart terminal or source ~/.zshrc.

### Issue: OTel Collector not receiving data
**Solution**: Check port 4317 is not in use: `lsof -i :4317`

### Issue: XRG graph not updating
**Solution**:
1. Verify AI Token graph is enabled in XRG preferences
2. Check XRG is polling correct endpoint (8889, not 4318)
3. Rebuild XRG after code changes

### Issue: Build requires Xcode
**Solution**: Use `DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer` prefix:
```bash
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -project XRG.xcodeproj -scheme XRG -configuration Debug build
```

## Performance Notes

- **Polling Overhead**: ~1ms per HTTP request every second (negligible)
- **Memory Usage**: OTel Collector capped at 256MB (config line 19)
- **Network**: Local loopback only, no external traffic
- **CPU**: <1% on M1/M2 for all components

## Architecture Benefits

### vs. JSONL-based System
- ✅ **Native**: No file parsing, no Python bridge
- ✅ **Real-time**: 10-second export vs. 60-second polling
- ✅ **Standard**: Industry-standard OTel protocol
- ✅ **Extensible**: Can add Prometheus, Grafana, etc.
- ✅ **Reliable**: Official Anthropic support

### vs. Simple Python Bridge
- ✅ **Protocol Support**: Full gRPC support (not just HTTP/JSON)
- ✅ **Production Ready**: Official OTel Collector binary
- ✅ **Debug Logging**: Built-in debug exporter
- ✅ **Scalable**: Can handle multiple exporters/receivers

## Future Enhancements

1. **LaunchAgent**: Auto-start OTel Collector on boot
2. **Grafana Dashboard**: Historical visualization
3. **Multi-Provider**: Add OpenAI, Codex endpoints
4. **Alerts**: Cost/token threshold notifications
5. **Release Build**: Code-sign for distribution

## References

- **Official Guide**: https://github.com/anthropics/claude-code-monitoring-guide
- **Claude Code Docs**: https://code.claude.com/docs/en/monitoring-usage
- **OTel Collector**: https://github.com/open-telemetry/opentelemetry-collector-releases
- **XRG Source**: https://gaucho.software/xrg/

---

**Deployment completed**: 2025-11-07 20:47
**Deployed by**: Claude Code + User (marc@2acrestudios.com)
**System**: macOS (Apple Silicon)
**Status**: Production-ready for personal use
