#!/usr/bin/env python3
"""
Local HTTP server for mini-rpc docs with /mini-rpc/ base path support.

Usage:
    cd docs && python3 server.py          # Listen on port 8000
    cd docs && python3 server.py 8081     # Listen on port 8081

Handles /mini-rpc/ prefix (GitHub Pages subdirectory) so all resources load correctly.
"""

import http.server
import os
import sys

PREFIX = '/mini-rpc/'

class Handler(http.server.SimpleHTTPRequestHandler):
    directory = os.getcwd()

    def do_GET(self):
        path = self.path

        # Root -> redirect to /mini-rpc/
        if path == '/':
            self.send_response(302)
            self.send_header('Location', '/mini-rpc/')
            self.end_headers()
            return

        # Strip /mini-rpc/ prefix -> serve /assets/... instead of /mini-rpc/assets/...
        if path.startswith(PREFIX):
            self.path = path[len(PREFIX):]

        return super().do_GET()

    def log_message(self, fmt, *args):
        if '404' not in str(args):
            print(f"{self.address_string()} {fmt % args}", flush=True)

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    print(f"Docs: http://localhost:{port}/mini-rpc/")
    print(f"Press Ctrl+C to stop.\n", flush=True)
    http.server.HTTPServer(('0.0.0.0', port), Handler).serve_forever()
