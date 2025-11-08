#!/bin/bash

echo "=== Verifying Live Telemetry ==="
echo ""

echo "First sample..."
FIRST=$(curl -s http://localhost:8889/metrics | grep 'claude_code_token_usage.*type="output"' | tail -1 | awk '{print $2}')
echo "Output tokens: $FIRST"

echo ""
echo "Waiting 15 seconds for new telemetry..."
sleep 15

echo "Second sample..."
SECOND=$(curl -s http://localhost:8889/metrics | grep 'claude_code_token_usage.*type="output"' | tail -1 | awk '{print $2}')
echo "Output tokens: $SECOND"

echo ""
if [ "$FIRST" != "$SECOND" ]; then
    echo "✅ VERIFIED: Telemetry is LIVE and updating!"
    DIFF=$((SECOND - FIRST))
    echo "   Generated $DIFF new output tokens during test"
else
    echo "⚠️  Same value - system working but needs Claude activity to see changes"
fi
