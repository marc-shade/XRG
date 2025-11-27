#!/bin/bash

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  XRG AI Token Monitoring - System Status Check"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check 1: OTel Collector / Bridge
echo "1️⃣  OpenTelemetry Collector / Bridge"
if pgrep -f otelcol-contrib > /dev/null; then
    PID=$(pgrep -f otelcol-contrib)
    echo "   ✅ Collector Running (PID: $PID)"
    echo "      Config: $(pwd)/otel-collector-xrg.yaml"
    echo "      Ports: 4317 (gRPC), 4318 (HTTP), 8889 (Prometheus)"
elif pgrep -f "claude-otel-bridge.py" > /dev/null; then
    PID=$(pgrep -f "claude-otel-bridge.py")
    echo "   ✅ Bridge Running (PID: $PID)"
    echo "      Script: $(pwd)/claude-otel-bridge.py"
    echo "      Ports: 4318 (HTTP Input & Prometheus Output)"
else
    echo "   ❌ NOT RUNNING"
    echo "      Start: ./start-ai-monitoring.sh"
fi
echo ""

# Check 2: Metrics Endpoint
echo "2️⃣  Prometheus Metrics Endpoint"
ENDPOINT_URL=""
if curl -s -m 2 http://localhost:8889/metrics > /dev/null 2>&1; then
    ENDPOINT_URL="http://localhost:8889/metrics"
elif curl -s -m 2 http://localhost:4318/v1/metrics > /dev/null 2>&1; then
    ENDPOINT_URL="http://localhost:4318/v1/metrics"
fi

if [ -n "$ENDPOINT_URL" ]; then
    echo "   ✅ Responding at $ENDPOINT_URL"

    # Check for Claude metrics
    if curl -s "$ENDPOINT_URL" | grep -q claude_code_token_usage; then
        echo "   ✅ Claude Code metrics detected"

        # Show current stats
        TOKENS=$(curl -s "$ENDPOINT_URL" | grep 'claude_code_token_usage.*type="output"' | tail -1 | awk '{print $2}')
        COST=$(curl -s "$ENDPOINT_URL" | grep 'claude_code_cost_usage' | awk '{sum += $2} END {print sum}')
        if [ -z "$COST" ]; then COST="0"; fi
        echo "      Output tokens: ${TOKENS:-0}"
        echo "      Total cost: \$${COST:-0}"
    else
        echo "   ⚠️  No Claude Code metrics (start a Claude session)"
    fi
else
    echo "   ❌ NOT RESPONDING (Checked ports 8889 and 4318)"
fi
echo ""

# Check 3: Claude Code Telemetry
echo "3️⃣  Claude Code Telemetry Configuration"
if [ "$CLAUDE_CODE_ENABLE_TELEMETRY" = "1" ]; then
    echo "   ✅ Enabled"
    echo "      Protocol: ${OTEL_EXPORTER_OTLP_PROTOCOL:-not set}"
    echo "      Endpoint: ${OTEL_EXPORTER_OTLP_ENDPOINT:-not set}"
    echo "      Export interval: ${OTEL_METRIC_EXPORT_INTERVAL:-60000}ms"
else
    echo "   ❌ NOT ENABLED"
    echo "      Set: export CLAUDE_CODE_ENABLE_TELEMETRY=1"
    echo "      Check: ~/.zshrc lines 279-297"
fi
echo ""

# Check 4: XRG Application
echo "4️⃣  XRG Application"
if pgrep -f "XRG.app" > /dev/null; then
    PID=$(pgrep -f "XRG.app")
    echo "   ✅ Running (PID: $PID)"

    # Check AI Token graph setting
    AI_ENABLED=$(defaults read com.gauchosoft.XRG showAITokenGraph 2>/dev/null)
    if [ "$AI_ENABLED" = "1" ]; then
        echo "   ✅ AI Token graph enabled"
    else
        echo "   ⚠️  AI Token graph disabled"
        echo "      Enable: Preferences → Graphs → Show AI Tokens"
    fi

    # Check endpoint setting
    CUSTOM_ENDPOINT=$(defaults read com.gauchosoft.XRG aiTokenOTelEndpoint 2>/dev/null)
    if [ -n "$CUSTOM_ENDPOINT" ]; then
        echo "      Endpoint: $CUSTOM_ENDPOINT (custom)"
    else
        echo "      Endpoint: http://localhost:8889/metrics (default)"
    fi
else
    echo "   ❌ NOT RUNNING"
    echo "      Start: open ~/Library/Developer/Xcode/DerivedData/.../Debug/XRG.app"
fi
echo ""

# Overall Status
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
ALL_GOOD=true

if ! pgrep -f otelcol-contrib > /dev/null && ! pgrep -f "claude-otel-bridge.py" > /dev/null; then ALL_GOOD=false; fi
if [ -z "$ENDPOINT_URL" ]; then ALL_GOOD=false; fi
if [ "$CLAUDE_CODE_ENABLE_TELEMETRY" != "1" ]; then ALL_GOOD=false; fi
if ! pgrep -f "XRG" > /dev/null; then ALL_GOOD=false; fi

if $ALL_GOOD; then
    echo "  ✅ ALL SYSTEMS OPERATIONAL"
    echo ""
    echo "  📊 View AI token usage in real-time in XRG window"
    echo "  🔗 Metrics: $ENDPOINT_URL"
    echo "  📝 Docs: $(pwd)/OTEL_DEPLOYMENT_COMPLETE.md"
else
    echo "  ⚠️  SOME COMPONENTS NOT RUNNING"
    echo ""
    echo "  Fix issues above, then run this script again"
fi
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
