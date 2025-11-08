#!/usr/bin/env python3
"""
Claude Code OTel to Prometheus Bridge for XRG
Receives Claude Code's OpenTelemetry metrics and exposes them as Prometheus format
at http://localhost:4318/v1/metrics for XRG to poll.
"""

import json
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Lock
from datetime import datetime

# Global storage for metrics
metrics_lock = Lock()
token_metrics = {
    'input': 0,
    'output': 0,
    'cache_read': 0,
    'cache_creation': 0
}
cost_metrics = 0.0
last_update = datetime.now()


class OTelReceiver(BaseHTTPRequestHandler):
    """Receives OTLP metrics from Claude Code"""

    def do_POST(self):
        global token_metrics, cost_metrics, last_update

        print(f"POST request received: path={self.path}, content-type={self.headers.get('Content-Type')}")

        # Only accept OTLP metrics
        if self.path != '/v1/metrics':
            print(f"  -> Rejecting: wrong path (expected /v1/metrics)")
            self.send_error(404)
            return

        content_length = int(self.headers.get('Content-Length', 0))
        if content_length == 0:
            self.send_error(400)
            return

        # Read OTLP payload
        body = self.rfile.read(content_length)

        try:
            # Parse JSON format OTLP (if using http/json protocol)
            if self.headers.get('Content-Type', '').startswith('application/json'):
                data = json.loads(body)
                self._process_otlp_json(data)
            # For protobuf format, you'd need to decode using protobuf library
            else:
                # Simplified: assume JSON for now
                data = json.loads(body)
                self._process_otlp_json(data)

            self.send_response(200)
            self.end_headers()

        except Exception as e:
            print(f"Error processing OTLP data: {e}")
            self.send_error(500)

    def do_GET(self):
        """Expose metrics in Prometheus format for XRG"""

        if self.path != '/v1/metrics':
            self.send_error(404)
            return

        with metrics_lock:
            # Generate Prometheus text format
            prometheus_output = self._generate_prometheus_format()

        self.send_response(200)
        self.send_header('Content-Type', 'text/plain; version=0.0.4')
        self.end_headers()
        self.wfile.write(prometheus_output.encode('utf-8'))

    def _process_otlp_json(self, data):
        """Extract token metrics from OTLP JSON payload"""
        global token_metrics, cost_metrics, last_update

        with metrics_lock:
            # Navigate OTLP structure: resourceMetrics -> scopeMetrics -> metrics
            for resource_metric in data.get('resourceMetrics', []):
                for scope_metric in resource_metric.get('scopeMetrics', []):
                    for metric in scope_metric.get('metrics', []):
                        name = metric.get('name', '')

                        # Debug: Print all metric names we receive
                        print(f"Received metric: {name}")

                        # Process token usage metrics
                        if 'token' in name.lower() and 'usage' in name.lower():
                            print(f"  -> Processing as token metric")
                            self._extract_token_values(metric)

                        # Process cost metrics
                        elif 'cost' in name.lower():
                            print(f"  -> Processing as cost metric")
                            self._extract_cost_values(metric)

            last_update = datetime.now()

    def _extract_token_values(self, metric):
        """Extract token counts from metric data points"""
        global token_metrics

        # Handle sum/gauge metrics
        for data_type in ['sum', 'gauge']:
            if data_type in metric:
                for data_point in metric[data_type].get('dataPoints', []):
                    # Get token type from attributes
                    token_type = None
                    for attr in data_point.get('attributes', []):
                        if attr.get('key') == 'token_type':
                            token_type = attr.get('value', {}).get('stringValue')

                    if token_type in token_metrics:
                        value = data_point.get('asInt', data_point.get('asDouble', 0))
                        token_metrics[token_type] = int(value)

    def _extract_cost_values(self, metric):
        """Extract cost from metric data points"""
        global cost_metrics

        for data_type in ['sum', 'gauge']:
            if data_type in metric:
                for data_point in metric[data_type].get('dataPoints', []):
                    value = data_point.get('asDouble', data_point.get('asInt', 0.0))
                    cost_metrics = float(value)

    def _generate_prometheus_format(self):
        """Generate Prometheus text format output"""
        output = []

        # Token usage metrics
        output.append('# HELP claude_code_token_usage Token usage by type')
        output.append('# TYPE claude_code_token_usage counter')
        output.append(f'claude_code_token_usage{{type="input"}} {token_metrics["input"]}')
        output.append(f'claude_code_token_usage{{type="output"}} {token_metrics["output"]}')
        output.append(f'claude_code_token_usage{{type="cache_read"}} {token_metrics["cache_read"]}')
        output.append(f'claude_code_token_usage{{type="cache_creation"}} {token_metrics["cache_creation"]}')

        # Cost metrics
        output.append('# HELP claude_code_cost_usage Total cost in USD')
        output.append('# TYPE claude_code_cost_usage gauge')
        output.append(f'claude_code_cost_usage {cost_metrics}')

        # Metadata
        output.append(f'# Last update: {last_update.isoformat()}')

        return '\n'.join(output) + '\n'

    def log_message(self, format, *args):
        """Log all requests"""
        print(f"HTTP Request: {format % args}")


def main():
    server_address = ('127.0.0.1', 4318)
    httpd = HTTPServer(server_address, OTelReceiver)

    print(f"Claude Code OTel Bridge running on http://{server_address[0]}:{server_address[1]}")
    print(f"Receiving OTLP at: POST http://{server_address[0]}:{server_address[1]}/v1/metrics")
    print(f"Exposing Prometheus at: GET http://{server_address[0]}:{server_address[1]}/v1/metrics")
    print("XRG will poll the GET endpoint for metrics")
    print("\nPress Ctrl+C to stop")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down...")
        httpd.shutdown()


if __name__ == '__main__':
    main()
