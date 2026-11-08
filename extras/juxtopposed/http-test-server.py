#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The local HTTP servers of extras/juxtopposed/http-request-test.cpp: three
# of them, on 127.0.0.1, 127.0.0.2 (another host) and 127.0.0.1 again (the
# same host, another port), all answering:
#   /echo                   the request as JSON: its method, path, headers, body
#   /redirect?code=&to=     that status, Location: to
#   /loop                   302 to itself
#   /auth                   401 Basic until an Authorization comes, then /echo
#   /slow-headers?ms=       its answer after ms
#   /slow-body?ms=&bytes=   the headers at once, then a byte every ms
#   /stall?ms=              the headers and a byte, then nothing for ms
#   /big?bytes=             that many bytes
# It prints "PORTS <first> <second> <third>" once they listen, then serves
# until its input closes (the test ends) or it is killed.
import json
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass

    def query(self):
        return {k: v[0] for k, v in parse_qs(urlsplit(self.path).query, keep_blank_values=True).items()}

    def body(self):
        length = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(length) if length else b""

    def send(self, code, data=b"", headers=()):
        self.send_response(code)
        for name, value in headers:
            self.send_header(name, value)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def echo(self, body):
        answer = {
            "method": self.command,
            "path": self.path,
            "host": self.server.server_address[0],
            "port": self.server.server_address[1],
            "headers": {k: v for k, v in self.headers.items()},
            "body": body.decode("utf-8", "replace"),
        }
        self.send(200, json.dumps(answer).encode(), [("Content-Type", "application/json")])

    def handle_any(self):
        path = urlsplit(self.path).path
        q = self.query()
        body = self.body()
        try:
            if path == "/echo":
                self.echo(body)
            elif path == "/redirect":
                self.send(int(q.get("code", "302")), b"moved", [("Location", q.get("to", "/echo"))])
            elif path == "/loop":
                self.send(302, b"again", [("Location", "/loop")])
            elif path == "/auth":
                if "Authorization" in self.headers:
                    self.echo(body)
                else:
                    self.send(401, b"who?", [("WWW-Authenticate", 'Basic realm="test"')])
            elif path == "/slow-headers":
                time.sleep(int(q.get("ms", "1000")) / 1000)
                self.echo(body)
            elif path == "/slow-body":
                count = int(q.get("bytes", "100"))
                self.send_response(200)
                self.send_header("Content-Length", str(count))
                self.end_headers()
                self.wfile.flush()
                for _ in range(count):
                    time.sleep(int(q.get("ms", "100")) / 1000)
                    self.wfile.write(b"x")
                    self.wfile.flush()
            elif path == "/stall":
                self.send_response(200)
                self.send_header("Content-Length", "1000")
                self.end_headers()
                self.wfile.write(b"x")
                self.wfile.flush()
                time.sleep(int(q.get("ms", "5000")) / 1000)
            elif path == "/big":
                self.send(200, b"y" * int(q.get("bytes", "1000")))
            else:
                self.send(404, b"no such path")
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass

    do_GET = do_POST = do_PUT = do_HEAD = do_DELETE = handle_any


class Server(ThreadingHTTPServer):
    daemon_threads = True

    def handle_error(self, request, client_address):
        # (the test cancels requests: their connections end without a word)
        pass


def main():
    servers = [Server((host, 0), Handler) for host in ("127.0.0.1", "127.0.0.2", "127.0.0.1")]
    for server in servers:
        server.daemon_threads = True
        threading.Thread(target=server.serve_forever, daemon=True).start()
    print("PORTS %s" % " ".join(str(server.server_address[1]) for server in servers), flush=True)
    # (until the test closes its input, or kills it)
    sys.stdin.read()


if __name__ == "__main__":
    main()
