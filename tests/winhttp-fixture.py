#!/usr/bin/env python3
"""Serve deterministic regression responses on loopback; no external requests."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def do_GET(self):
        if self.path == '/headers':
            body = b'x'
            self.send_response(200)
            self.send_header('Connection', 'A' * 100)
            self.send_header('Proxy-Connection', 'B' * 40)
        elif self.path == '/proxy.pac':
            body = b'function FindProxyForURL(url, host) { return "DIRECT"; }\n'
            self.send_response(200)
            self.send_header('Content-Type', 'application/x-ns-proxy-autoconfig')
        else:
            self.send_error(404)
            return
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        if self.path == '/headers':
            self.close_connection = True

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8765)
    args = parser.parse_args()
    with ThreadingHTTPServer(('127.0.0.1', args.port), Handler) as server:
        print(f'Wine regression fixture on 127.0.0.1:{server.server_port}', flush=True)
        server.serve_forever()
