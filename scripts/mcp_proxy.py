#!/usr/bin/env python3

# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""
Stdio<->HTTP proxy for the Qt Creator built-in MCP server.

Discovers a running Qt Creator instance at startup by scanning loopback
listening sockets owned by qtcreator processes and probing each with MCP
initialize to confirm.  Bridges MCP stdio transport (newline-delimited
JSON on stdin/stdout) to the Streamable-HTTP transport (POST / + SSE
GET /) used by Qt Creator 19+.

If the backend stops responding (e.g. the targeted Qt Creator instance
was restarted and came back on a different auto-selected port), the proxy
re-runs discovery, replays the cached initialize handshake to establish a
fresh session, and continues -- transparently to the MCP client.  This
keeps the connection alive across the frequent restarts typical when
working on Qt Creator itself, including when several instances are running.

Usage: python3 mcp_proxy.py [--prefer-cwd]

  --prefer-cwd  Query each candidate's get_current_project tool and
                prefer the instance whose active project path is a
                prefix of the current working directory.  Falls back
                to the first responding instance.

Register as a stdio MCP server in your MCP client's configuration, e.g.:

  {
    "mcpServers": {
      "qtcreator": {
        "type": "stdio",
        "command": "python3",
        "args": ["/path/to/scripts/mcp_proxy.py", "--prefer-cwd"]
      }
    }
  }
"""

import sys
import json
import os
import re
import subprocess
import threading
import http.client
import time

MCP_VERSION = "2025-11-25"
PROBE_TIMEOUT = 3
REQUEST_TIMEOUT = 120   # covers long build / run operations
_write_lock = threading.Lock()


def _log(*args):
    print("qtcreator-mcp-proxy:", *args, file=sys.stderr, flush=True)


def _send(obj):
    with _write_lock:
        sys.stdout.write(json.dumps(obj, separators=(',', ':')) + '\n')
        sys.stdout.flush()


def _qtcreator_loopback_ports_unix():
    try:
        pids = set(subprocess.run(
            ['pgrep', 'qtcreator'], capture_output=True, text=True
        ).stdout.split())
    except OSError:
        return []
    if not pids:
        return []
    try:
        ss_out = subprocess.run(['ss', '-tlnp'], capture_output=True, text=True).stdout
    except OSError:
        return []
    ports = []
    for line in ss_out.splitlines():
        m = re.search(r'pid=(\d+)', line)
        if not m or m.group(1) not in pids:
            continue
        cols = line.split()
        if len(cols) < 4 or not cols[3].startswith('127.0.0.1:'):
            continue
        try:
            ports.append(int(cols[3].split(':')[1]))
        except (IndexError, ValueError):
            pass
    return ports


def _qtcreator_loopback_ports_windows():
    try:
        tasklist = subprocess.run(
            ['tasklist', '/fi', 'IMAGENAME eq qtcreator.exe', '/fo', 'csv', '/nh'],
            capture_output=True, text=True
        ).stdout
    except OSError:
        return []
    pids = set()
    for line in tasklist.splitlines():
        m = re.match(r'"[^"]+","(\d+)"', line)
        if m:
            pids.add(m.group(1))
    if not pids:
        return []
    try:
        netstat = subprocess.run(['netstat', '-ano'], capture_output=True, text=True).stdout
    except OSError:
        return []
    ports = []
    for line in netstat.splitlines():
        cols = line.split()
        if len(cols) < 5 or cols[0] != 'TCP' or cols[3] != 'LISTENING':
            continue
        if not cols[1].startswith('127.0.0.1:'):
            continue
        if cols[4] not in pids:
            continue
        try:
            ports.append(int(cols[1].split(':')[1]))
        except (IndexError, ValueError):
            pass
    return ports


def _qtcreator_loopback_ports():
    if sys.platform == 'win32':
        return _qtcreator_loopback_ports_windows()
    return _qtcreator_loopback_ports_unix()


def _probe_mcp(port):
    body = json.dumps({
        'jsonrpc': '2.0', 'id': 0, 'method': 'initialize',
        'params': {
            'protocolVersion': MCP_VERSION,
            'capabilities': {},
            'clientInfo': {'name': 'qtcreator-mcp-proxy', 'version': '1'},
        },
    }).encode()
    try:
        c = http.client.HTTPConnection('127.0.0.1', port, timeout=PROBE_TIMEOUT)
        c.request('POST', '/', body, {
            'Content-Type': 'application/json',
            'Accept': 'application/json, text/event-stream',
            'Content-Length': str(len(body)),
        })
        r = c.getresponse()
        sid = r.getheader('mcp-session-id')
        r.read()
        c.close()
        if r.status == 200 and sid:
            return sid
    except OSError:
        pass
    return None


def _delete_session(port, sid):
    try:
        c = http.client.HTTPConnection('127.0.0.1', port, timeout=PROBE_TIMEOUT)
        c.request('DELETE', '/', headers={'mcp-session-id': sid})
        c.getresponse().read()
        c.close()
    except OSError:
        pass


def _find_best_port(prefer_cwd):
    ports = _qtcreator_loopback_ports()
    _log(f"candidate ports: {ports}")
    if not ports:
        _log("no qtcreator process found or socket scan unavailable")
        return None, None

    candidates = [(port, sid) for port in ports if (sid := _probe_mcp(port))]
    _log(f"responsive MCP candidates: {[p for p, _ in candidates]}")
    if not candidates:
        _log("no responsive Qt Creator MCP server found")
        return None, None

    if not prefer_cwd or len(candidates) == 1:
        for p, s in candidates[1:]:
            _delete_session(p, s)
        port, sid = candidates[0]
        _log(f"using Qt Creator MCP on port {port}")
        return port, sid

    cwd_norm = os.path.normcase(os.path.normpath(os.getcwd()))
    _log(f"cwd for --prefer-cwd matching: {cwd_norm!r}")
    best, best_len = candidates[0], -1
    for port, sid in candidates:
        body = json.dumps({
            'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
            'params': {'name': 'get_current_project', 'arguments': {}},
        }).encode()
        try:
            c = http.client.HTTPConnection('127.0.0.1', port, timeout=PROBE_TIMEOUT)
            c.request('POST', '/', body, {
                'Content-Type': 'application/json',
                'Accept': 'application/json, text/event-stream',
                'Content-Length': str(len(body)),
                'mcp-session-id': sid,
            })
            r = c.getresponse()
            result = json.loads(r.read())
            c.close()
        except (OSError, json.JSONDecodeError) as e:
            _log(f"port {port}: get_current_project failed ({e})")
            continue
        result_obj = result.get('result', {})
        data = result_obj.get('structuredContent')
        if not isinstance(data, dict):
            data = None
            for item in result_obj.get('content', []):
                if item.get('type') == 'text':
                    try:
                        parsed = json.loads(item.get('text', ''))
                    except (json.JSONDecodeError, TypeError):
                        parsed = None
                    if isinstance(parsed, dict):
                        data = parsed
                    break
        if not data:
            _log(f"port {port}: get_current_project returned no project data; result={result!r}")
            continue
        project_dir = data.get('projectDirectory', '')
        if not project_dir:
            _log(f"port {port}: get_current_project missing projectDirectory; data={data!r}")
            continue
        project_dir_norm = os.path.normcase(os.path.normpath(project_dir))
        match = cwd_norm == project_dir_norm or cwd_norm.startswith(project_dir_norm + os.sep)
        _log(f"port {port}: project={data.get('projectName')!r} dir={project_dir_norm!r} match={match} dir_len={len(project_dir_norm)} best_len={best_len}")
        if match and len(project_dir_norm) > best_len:
            best, best_len = (port, sid), len(project_dir_norm)

    for p, s in candidates:
        if (p, s) != best:
            _delete_session(p, s)
    _log(f"using Qt Creator MCP on port {best[0]} (best_len={best_len})")
    return best


class _Proxy:
    def __init__(self, port, probe_sid, prefer_cwd):
        self._port = port
        self._probe_sid = probe_sid
        self._prefer_cwd = prefer_cwd
        self._sid = None
        self._sse_started = False
        self._init_request = None
        # Bumped on every successful re-discovery; lets a thread tell whether
        # another thread already reconnected while its request was failing.
        self._generation = 0
        self._reconnect_lock = threading.Lock()

    def _do_post(self, msg):
        # Single HTTP POST attempt against the current backend.  Returns the
        # list of parsed responses, or None if the backend is unreachable.
        body = json.dumps(msg, separators=(',', ':')).encode()
        headers = {
            'Content-Type': 'application/json',
            'Accept': 'application/json, text/event-stream',
            'Content-Length': str(len(body)),
        }
        if self._sid:
            headers['mcp-session-id'] = self._sid
        try:
            c = http.client.HTTPConnection('127.0.0.1', self._port, timeout=REQUEST_TIMEOUT)
            c.request('POST', '/', body, headers)
            r = c.getresponse()
        except OSError as e:
            _log(f"POST failed: {e}")
            return None
        sid = r.getheader('mcp-session-id')
        if sid:
            self._sid = sid
        content_type = r.getheader('content-type', '')
        raw = r.read()
        c.close()
        if not raw:
            return []
        if 'text/event-stream' in content_type:
            results = []
            for line in raw.decode().splitlines():
                if line.startswith('data: '):
                    try:
                        results.append(json.loads(line[6:]))
                    except json.JSONDecodeError:
                        pass
            return results
        try:
            return [json.loads(raw)]
        except json.JSONDecodeError:
            return []

    def _post(self, msg):
        results = self._do_post(msg)
        if results is not None:
            return results
        # The backend went away -- typically this Qt Creator instance was
        # restarted and came back on a different auto-selected port.  Find the
        # matching instance again, replay the cached initialize handshake to
        # establish a fresh session, then retry the request once.
        if not self._rediscover(self._generation):
            return []
        results = self._do_post(msg)
        return results if results is not None else []

    def _rediscover(self, gen):
        with self._reconnect_lock:
            if gen != self._generation:
                # Another thread already reconnected since the caller failed.
                return True
            port, probe_sid = _find_best_port(self._prefer_cwd)
            if not port:
                _log("re-discovery found no running Qt Creator MCP server")
                return False
            _log(f"re-discovered Qt Creator MCP on port {port}, re-initializing session")
            self._port = port
            self._sid = None
            _delete_session(port, probe_sid)
            if self._init_request is not None:
                self._do_post(self._init_request)
                self._do_post({'jsonrpc': '2.0', 'method': 'notifications/initialized'})
            self._generation += 1
            return True

    def _start_sse(self):
        self._sse_started = True
        def worker():
            while True:
                sid = self._sid
                if not sid:
                    time.sleep(0.2)
                    continue
                try:
                    c = http.client.HTTPConnection('127.0.0.1', self._port, timeout=None)
                    c.request('GET', '/', headers={
                        'Accept': 'text/event-stream',
                        'mcp-session-id': sid,
                    })
                    r = c.getresponse()
                    if r.status != 200:
                        c.close()
                        time.sleep(1)
                        continue
                    for raw_line in r:
                        line = raw_line.decode().rstrip('\r\n')
                        if line.startswith('data: '):
                            try:
                                _send(json.loads(line[6:]))
                            except json.JSONDecodeError:
                                pass
                    c.close()
                except OSError as e:
                    # Backend gone; re-discover so server-initiated messages
                    # keep flowing across restarts even while idle.
                    _log(f"SSE stream closed ({e}), reconnecting")
                    if not self._rediscover(self._generation):
                        time.sleep(1)
        threading.Thread(target=worker, daemon=True).start()

    def run(self):
        _delete_session(self._port, self._probe_sid)
        for raw_line in sys.stdin:
            raw_line = raw_line.strip()
            if not raw_line:
                continue
            try:
                msg = json.loads(raw_line)
            except json.JSONDecodeError:
                continue
            if msg.get('method') == 'initialize':
                self._init_request = msg
            is_notification = 'id' not in msg
            responses = self._post(msg)
            if self._sid and not self._sse_started:
                self._start_sse()
            if not is_notification:
                for obj in responses:
                    _send(obj)


if __name__ == '__main__':
    prefer_cwd = '--prefer-cwd' in sys.argv
    port, probe_sid = _find_best_port(prefer_cwd)
    if not port:
        sys.exit(1)
    _Proxy(port, probe_sid, prefer_cwd).run()
