#!/usr/bin/env python3
"""
Simple HTTP server with COOP and COEP headers for SharedArrayBuffer support.

This server is required for WebAssembly threading to work in browsers.
"""

import http.server
import socketserver
import os
import sys

PORT = 8080
DIRECTORY = "output"

class COOPCOEPHandler(http.server.SimpleHTTPRequestHandler):
    """HTTP handler that adds required headers for SharedArrayBuffer."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        # Required headers for SharedArrayBuffer and WebAssembly threads
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')

        # Disable caching for development
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')

        super().end_headers()

    def log_message(self, format, *args):
        """Custom log format with colors."""
        message = format % args
        print(f"[{self.log_date_time_string()}] {message}")

def main():
    # Check if output directory exists
    if not os.path.exists(DIRECTORY):
        print(f"❌ Error: '{DIRECTORY}' directory not found!")
        print("Please run ./build.sh first to generate the output files.")
        sys.exit(1)

    # Check if required files exist
    required_files = ['index.html', 'main.js', 'wasm_bridge.js', 'wasm_bridge.wasm']
    missing_files = [f for f in required_files if not os.path.exists(os.path.join(DIRECTORY, f))]

    if missing_files:
        print(f"⚠️  Warning: Missing files in '{DIRECTORY}': {', '.join(missing_files)}")
        print("Run ./build.sh to generate all required files.")

    # Start server
    with socketserver.TCPServer(("", PORT), COOPCOEPHandler) as httpd:
        print("=" * 60)
        print(f"  🚀 WASM Demo Server Running")
        print("=" * 60)
        print(f"  📁 Serving directory: {DIRECTORY}/")
        print(f"  🌐 URL: http://localhost:{PORT}")
        print(f"  🔒 COOP/COEP headers: ENABLED ✓")
        print(f"  ⚡ SharedArrayBuffer: ENABLED ✓")
        print("=" * 60)
        print("\n👉 Open http://localhost:{} in your browser\n".format(PORT))
        print("Press Ctrl+C to stop the server.\n")

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\n⏹️  Server stopped.")
            sys.exit(0)

if __name__ == "__main__":
    main()

