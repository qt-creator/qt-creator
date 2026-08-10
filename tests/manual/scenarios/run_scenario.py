#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Run a declarative UI scenario against a real Qt Creator and emit a tutorial.

A scenario is a YAML file of semantic steps ("click the OK button in the
Execute Extension Command dialog"). The runner drives Qt Creator's built-in
MCP server over HTTP - using the widget-addressing tools (find_widgets,
click_widget, type_text, select_combo_item) and the observation tools
(widget_exists, read_pane, screenshot) - and writes a Markdown tutorial next
to the captured screenshots. Because the document is generated from what
actually ran, it cannot drift from the behaviour it describes.

The runner attaches to an already-running Creator given its --port, or, with
--qtcreator, launches one itself (with a throwaway settings directory). It
never manages the X display; run it under whatever DISPLAY you set up (e.g.
Xvfb for headless use).

See README.md for the scenario format and examples.
"""

import argparse
import http.client
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import yaml


class McpClient:
    """Minimal MCP streamable-HTTP client for Qt Creator's McpServer.

    One session is established with initialize(); every call opens its own TCP
    connection but reuses the session id, so a blocking call (a modal dialog
    that will not return until a later step dismisses it) can run on its own
    connection without stalling the others.
    """

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.session_id = None

    def _rpc(self, obj):
        conn = http.client.HTTPConnection(self.host, self.port, timeout=120)
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json, text/event-stream",
        }
        if self.session_id:
            headers["mcp-session-id"] = self.session_id
        conn.request("POST", "/", json.dumps(obj), headers)
        resp = conn.getresponse()
        sid = resp.getheader("mcp-session-id")
        if sid:
            self.session_id = sid
        body = resp.read().decode("utf-8", "replace")
        conn.close()
        return json.loads(body) if body else {}

    def initialize(self):
        self._rpc({
            "jsonrpc": "2.0", "id": 1, "method": "initialize",
            "params": {"protocolVersion": "2025-11-25", "capabilities": {},
                       "clientInfo": {"name": "run_scenario", "version": "1"}},
        })

    def call(self, tool, arguments):
        """Calls a tool and returns (is_error, structured_result, text)."""
        reply = self._rpc({
            "jsonrpc": "2.0", "id": 2, "method": "tools/call",
            "params": {"name": tool, "arguments": arguments},
        })
        result = reply.get("result", {})
        is_error = bool(result.get("error") or result.get("isError"))
        structured = result.get("structuredContent")
        text = ""
        content = result.get("content") or []
        if content and isinstance(content, list):
            text = content[0].get("text", "")
        if structured is None and text:
            try:
                structured = json.loads(text)
            except ValueError:
                structured = None
        return is_error, structured, text


class ScenarioError(Exception):
    pass


def slugify(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")[:40] or "step"


def widget_desc(w):
    """One-line description of a resolved widget for the tutorial."""
    if not isinstance(w, dict):
        return ""
    state = "visible" if w.get("visible") else "hidden"
    if not w.get("enabled", True):
        state += ", disabled"
    name = w.get("object_name") or ""
    return '{cls} "{name}" at {x},{y} {w}x{h} ({state})'.format(
        cls=w.get("class", "?"), name=name, x=w.get("x"), y=w.get("y"),
        w=w.get("width"), h=w.get("height"), state=state)


class Runner:
    def __init__(self, client, scenario, out_dir, scratch):
        self.client = client
        self.scenario = scenario
        self.out_dir = out_dir
        self.scratch = scratch
        self.shots_dir = out_dir / "shots"
        self.shots_dir.mkdir(parents=True, exist_ok=True)
        self.report = []          # (index, describe, call_line, note, shot_rel)
        self.pending = []         # (thread, holder) for blocking invoke_action
        self.step_no = 0

    def subst(self, value):
        if isinstance(value, str):
            return value.replace("{scratch}", str(self.scratch))
        if isinstance(value, dict):
            return {k: self.subst(v) for k, v in value.items()}
        return value

    def record(self, describe, call_line, note="", shot_rel=None):
        self.report.append((self.step_no, describe, call_line, note, shot_rel))

    def call_or_fail(self, tool, args, describe):
        is_error, structured, text = self.client.call(tool, args)
        if is_error:
            raise ScenarioError("step {}: {} failed: {}".format(
                self.step_no, tool, text or structured))
        return structured

    # --- step handlers -------------------------------------------------

    def do_invoke_action(self, step):
        action = step["invoke_action"]
        call_line = 'call_action id="{}"'.format(action)
        if step.get("blocks"):
            # A modal exec() dialog blocks this call until a later step
            # dismisses it; fire it on its own connection and move on. The
            # server keeps serving requests from its nested event loop.
            holder = {}

            def worker():
                holder["result"] = self.client.call("call_action", {"id": action})

            t = threading.Thread(target=worker, daemon=True)
            t.start()
            self.pending.append((t, holder, action))
            self.record(step["describe"], call_line,
                        note="Dispatched (opens a modal dialog; dismissed by a later step).")
        else:
            self.call_or_fail("call_action", {"id": action}, step["describe"])
            self.record(step["describe"], call_line)

    def do_open(self, path, describe):
        self.call_or_fail("open_file", {"path": path}, describe)
        self.record(describe, 'open_file path="{}"'.format(path))

    def do_click(self, step):
        query = self.subst(step["click"])
        w = self.call_or_fail("click_widget", query, step["describe"])
        self.record(step["describe"], "click_widget " + json.dumps(query),
                    note="Resolved to: " + widget_desc(w))

    def do_type(self, step):
        args = self.subst(step["type"])
        w = self.call_or_fail("type_text", args, step["describe"])
        self.record(step["describe"], "type_text " + json.dumps(args),
                    note="Typed into: " + widget_desc(w))

    def do_select(self, step):
        args = self.subst(step["select"])
        w = self.call_or_fail("select_combo_item", args, step["describe"])
        self.record(step["describe"], "select_combo_item " + json.dumps(args),
                    note="Selected in: " + widget_desc(w))

    def do_expect(self, step, want_present):
        query = self.subst(step["expect" if want_present else "expect_gone"])
        r = self.call_or_fail("widget_exists", query, step["describe"])
        exists, count = r.get("exists"), r.get("count")
        if want_present and not exists:
            raise ScenarioError("step {}: expected a widget for {} but found none"
                                 .format(self.step_no, query))
        if not want_present and count:
            raise ScenarioError("step {}: expected no widget for {} but found {}"
                                 .format(self.step_no, query, count))
        verb = "present" if want_present else "absent"
        note = "Confirmed {} (count {}).".format(verb, count)
        if want_present and r.get("first"):
            note += " " + widget_desc(r["first"])
        self.record(step["describe"], "widget_exists " + json.dumps(query), note=note)

    def do_wait_for(self, step):
        query = self.subst(step["wait_for"])
        timeout = float(step.get("timeout", 15))
        deadline = time.monotonic() + timeout
        count = 0
        while time.monotonic() < deadline:
            r = self.call_or_fail("widget_exists", query, step["describe"])
            if r.get("exists"):
                self.record(step["describe"], "widget_exists " + json.dumps(query),
                            note="Appeared (count {}).".format(r.get("count")))
                return
            count += 1
        raise ScenarioError("step {}: timed out after {}s waiting for {}"
                            .format(self.step_no, timeout, query))

    def do_read_pane(self, step):
        name = step["read_pane"]
        r = self.call_or_fail("read_pane", {"name": name}, step["describe"])
        reason = r.get("reason")
        if reason != "ok":
            self.record(step["describe"], 'read_pane name="{}"'.format(name),
                        note="Pane not readable: {}.".format(reason))
            return
        text = r.get("text", "")
        artifact = self.out_dir / (slugify(name) + ".txt")
        artifact.write_text(text, encoding="utf-8")
        self.record(step["describe"], 'read_pane name="{}"'.format(name),
                    note="{} chars saved to {}.".format(len(text), artifact.name))

    def do_capture(self, step):
        spec = step.get("capture")
        query = self.subst(spec) if isinstance(spec, dict) else {}
        shot = self.shots_dir / "{:02d}-{}.png".format(self.step_no, slugify(step["describe"]))
        args = dict(query)
        args["path"] = str(shot)
        args["embed"] = False
        r = self.call_or_fail("screenshot", args, step["describe"])
        rel = os.path.relpath(shot, self.out_dir)
        self.record(step["describe"], "screenshot " + json.dumps(query),
                    note="Captured {}x{} of \"{}\".".format(
                        r.get("width"), r.get("height"), r.get("window_title")),
                    shot_rel=rel)

    # --- driver --------------------------------------------------------

    def run(self):
        setup = self.scenario.get("setup", {})
        if setup.get("open"):
            self.step_no += 1
            self.do_open(self.subst(setup["open"]), "Open " + setup["open"])

        for step in self.scenario.get("steps", []):
            self.step_no += 1
            if "invoke_action" in step:
                self.do_invoke_action(step)
            elif "click" in step:
                self.do_click(step)
            elif "type" in step:
                self.do_type(step)
            elif "select" in step:
                self.do_select(step)
            elif "expect" in step:
                self.do_expect(step, True)
            elif "expect_gone" in step:
                self.do_expect(step, False)
            elif "wait_for" in step:
                self.do_wait_for(step)
            elif "read_pane" in step:
                self.do_read_pane(step)
            elif "capture" in step:
                self.do_capture(step)
            else:
                raise ScenarioError("step {}: no known action in {}"
                                    .format(self.step_no, list(step)))

        # Collect any blocking invoke_action calls that a later step released.
        for t, holder, action in self.pending:
            t.join(timeout=10)
            if t.is_alive():
                raise ScenarioError('blocking action "{}" never returned - '
                                    'was it dismissed by a later step?'.format(action))
            is_error, _, text = holder.get("result", (False, None, ""))
            if is_error:
                raise ScenarioError('blocking action "{}" failed: {}'.format(action, text))

    def write_report(self):
        lines = []
        lines.append("# {}".format(self.scenario.get("name", "Scenario")))
        lines.append("")
        if self.scenario.get("intent"):
            lines.append("> " + self.scenario["intent"].strip().replace("\n", " "))
            lines.append("")
        lines.append("_Generated by run_scenario.py from `{}`._".format(
            self.scenario.get("_source", "")))
        lines.append("")
        for idx, describe, call_line, note, shot in self.report:
            lines.append("## Step {} - {}".format(idx, describe))
            lines.append("")
            lines.append("    " + call_line)
            lines.append("")
            if note:
                lines.append(note)
                lines.append("")
            if shot:
                lines.append("![step {}]({})".format(idx, shot))
                lines.append("")
        report_path = self.out_dir / "report.md"
        report_path.write_text("\n".join(lines), encoding="utf-8")
        return report_path


def wait_for_port(host, port, timeout, child=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            c = http.client.HTTPConnection(host, port, timeout=2)
            c.connect()
            c.close()
            return True
        except OSError:
            if child and child.poll() is not None:
                raise ScenarioError("Qt Creator exited before the MCP port opened "
                                    "(exit {}).".format(child.returncode))
            time.sleep(0.2)
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("scenario", help="Path to the scenario YAML file")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8765,
                    help="MCP port of a running Qt Creator (default 8765)")
    ap.add_argument("--out", help="Output directory for the tutorial "
                    "(default: <scenario-dir>/out/<name>)")
    ap.add_argument("--qtcreator", help="Path to a Qt Creator binary to launch "
                    "(otherwise attach to --port)")
    ap.add_argument("--timeout", type=float, default=60,
                    help="Seconds to wait for the MCP port")
    args = ap.parse_args()

    scenario_path = Path(args.scenario).resolve()
    scenario = yaml.safe_load(scenario_path.read_text(encoding="utf-8"))
    scenario["_source"] = scenario_path.name
    name = scenario.get("name", scenario_path.stem)

    out_dir = Path(args.out) if args.out else scenario_path.parent / "out" / slugify(name)
    out_dir.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="scenario-"))

    child = None
    settings = None
    if args.qtcreator:
        settings = scratch / "settings"
        settings.mkdir()
        child = subprocess.Popen(
            [args.qtcreator, "-settingspath", str(settings),
             "-load", "McpServer", "-mcp-port", str(args.port)])

    exit_code = 0
    try:
        if not wait_for_port(args.host, args.port, args.timeout, child):
            raise ScenarioError("MCP port {} did not open within {}s"
                                .format(args.port, args.timeout))
        client = McpClient(args.host, args.port)
        client.initialize()
        runner = Runner(client, scenario, out_dir, scratch)
        try:
            runner.run()
            status = "PASSED"
        except ScenarioError as e:
            status = "FAILED"
            exit_code = 1
            runner.record("(scenario aborted)", str(e))
            print("SCENARIO FAILED:", e, file=sys.stderr)
        report = runner.write_report()
        print("{}: {}".format(status, name))
        print("Tutorial: {}".format(report))
    finally:
        # Only the Creator we launched ourselves is ours to stop; an attached
        # instance is left running for the caller.
        if child and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=10)
            except subprocess.TimeoutExpired:
                child.kill()

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
