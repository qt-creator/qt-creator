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
import shlex
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import yaml


FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
]


# Settings pre-seeded into a launched Creator's throwaway config so recordings
# are not cluttered by first-run prompts. TakeUITour and LinkWithQtInstallation
# are InfoBar ids suppressed via the global SuppressedWarnings list. (FakeVim is
# kept out of the way by not loading its plugin - see LAUNCH_ARGS - rather than
# by a setting, so typed text is inserted, not read as Vim commands.)
DEFAULT_SETTINGS = """\
[General]
SuppressedWarnings=TakeUITour, LinkWithQtInstallation
"""

# Plugins to skip when the runner launches Creator, so recordings are not
# affected by editing modes that reinterpret keystrokes.
LAUNCH_NOLOAD = ["FakeVim"]


def write_default_settings(settings_dir):
    ini = Path(settings_dir) / "QtProject" / "QtCreator.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    ini.write_text(DEFAULT_SETTINGS, encoding="utf-8")


def find_font():
    for f in FONT_CANDIDATES:
        if os.path.exists(f):
            return f
    return None


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


def fmt_offset(seconds):
    seconds = int(seconds)
    return "{:02d}:{:02d}".format(seconds // 60, seconds % 60)


class Recorder:
    """Records an X display to a video with ffmpeg x11grab while a scenario
    runs, then embeds the step timestamps as chapter markers."""

    def __init__(self, display, out_path, size=None, region=None, framerate=15):
        self.display = display
        self.raw_path = Path(out_path)
        self.size = size          # "WxH" grabbed from the display origin
        self.region = region      # (x, y, w, h) to crop to, e.g. the app window
        self.framerate = framerate
        self.proc = None
        self.log = None
        self.t0 = None
        self.duration = 0.0

    def start(self):
        cmd = ["ffmpeg", "-y", "-f", "x11grab", "-framerate", str(self.framerate)]
        input_spec = self.display
        if self.region:
            x, y, w, h = self.region
            # libx264 with yuv420p needs even dimensions.
            w, h = w - (w % 2), h - (h % 2)
            cmd += ["-video_size", "{}x{}".format(w, h)]
            input_spec = "{}+{},{}".format(self.display, x, y)
        elif self.size:
            cmd += ["-video_size", self.size]
        cmd += ["-i", input_spec,
                "-codec:v", "libx264", "-preset", "ultrafast", "-pix_fmt", "yuv420p",
                str(self.raw_path)]
        self.log = open(self.raw_path.with_suffix(".log"), "wb")
        self.proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                     stdout=self.log, stderr=self.log)
        # Wait until ffmpeg is actually writing frames before the run starts,
        # so early steps are not cut off. This waits on the file growing, not
        # on a fixed delay.
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise ScenarioError("ffmpeg exited during startup; see {}"
                                    .format(self.raw_path.with_suffix(".log")))
            if self.raw_path.exists() and self.raw_path.stat().st_size > 0:
                break
            time.sleep(0.1)
        self.t0 = time.monotonic()
        return self

    def stop(self, force=False):
        if not self.proc or self.proc.poll() is not None:
            return
        self.duration = time.monotonic() - (self.t0 or time.monotonic())
        try:
            if not force and self.proc.stdin:
                # "q" tells ffmpeg to stop and finalize the file cleanly.
                self.proc.stdin.write(b"q")
                self.proc.stdin.flush()
                self.proc.wait(timeout=10)
            else:
                self.proc.terminate()
                self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()
        finally:
            if self.log:
                self.log.close()

    def _keystroke_filters(self, key_events, font):
        """Builds ffmpeg drawtext filters that show each typed string as a
        bottom-centre bubble during its window. The text is passed via a file
        so it needs no filter-string escaping."""
        parts = []
        for i, (start, end, text) in enumerate(key_events):
            keyfile = self.raw_path.parent / "key_{}.txt".format(i)
            keyfile.write_text(text, encoding="utf-8")
            fontopt = "fontfile={}:".format(font) if font else ""
            parts.append(
                "drawtext={font}textfile={tf}:reload=0:fontcolor=white:fontsize=40:"
                "box=1:boxcolor=black@0.65:boxborderw=16:x=(w-text_w)/2:y=h-text_h-90:"
                "enable='between(t\\,{s:.3f}\\,{e:.3f})'".format(
                    font=fontopt, tf=keyfile, s=start, e=end))
        return ",".join(parts)

    def finalize(self, chapters, out_path, key_events=None, font=None):
        """Muxes chapter markers into out_path and burns in keystroke bubbles.
        Falls back to the raw recording if processing is unavailable."""
        out_path = Path(out_path)
        key_events = key_events or []
        if not self.raw_path.exists():
            return None
        if not chapters and not key_events:
            self.raw_path.replace(out_path)
            return out_path

        cmd = ["ffmpeg", "-y", "-i", str(self.raw_path)]
        meta = None
        if chapters:
            meta = self.raw_path.with_suffix(".ffmeta")
            lines = [";FFMETADATA1"]
            for i, (offset, title) in enumerate(chapters):
                start_ms = int(offset * 1000)
                next_off = chapters[i + 1][0] if i + 1 < len(chapters) else self.duration
                end_ms = max(int(next_off * 1000), start_ms + 1)
                safe_title = title.replace("=", " ").replace(";", " ").replace("#", " ")
                lines += ["[CHAPTER]", "TIMEBASE=1/1000",
                          "START={}".format(start_ms), "END={}".format(end_ms),
                          "title={}".format(safe_title)]
            meta.write_text("\n".join(lines) + "\n", encoding="utf-8")
            cmd += ["-i", str(meta), "-map_metadata", "1"]

        vf = self._keystroke_filters(key_events, font)
        if vf:
            # drawtext requires a re-encode; copy the (absent) audio through.
            cmd += ["-vf", vf, "-codec:v", "libx264", "-preset", "ultrafast",
                    "-pix_fmt", "yuv420p"]
        else:
            cmd += ["-codec", "copy"]
        cmd.append(str(out_path))

        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if result.returncode != 0 or not out_path.exists():
            # Processing failed - keep the plain recording so a video still exists.
            self.raw_path.replace(out_path)
            return out_path
        self.raw_path.unlink(missing_ok=True)
        if meta:
            meta.unlink(missing_ok=True)
        return out_path


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


def widget_identity(w):
    """The stable identity of a resolved widget for the baseline: which widget
    was acted on, not where it was or its transient state."""
    if not isinstance(w, dict):
        return {}
    return {"class": w.get("class"), "object_name": w.get("object_name"),
            "text": w.get("text")}


def compare_baseline(expected, actual):
    """Returns a list of human-readable mismatch strings (empty == match)."""
    diffs = []
    exp_steps = expected.get("steps", [])
    if len(exp_steps) != len(actual):
        diffs.append("step count changed: baseline {}, run {}"
                     .format(len(exp_steps), len(actual)))
    for exp, act in zip(exp_steps, actual):
        where = "step {} ({})".format(act["step"], act["describe"])
        if exp.get("tool") != act.get("tool"):
            diffs.append("{}: tool changed: {} -> {}"
                         .format(where, exp.get("tool"), act.get("tool")))
        if exp.get("check") != act.get("check"):
            diffs.append("{}: {} -> {}"
                         .format(where, json.dumps(exp.get("check")),
                                 json.dumps(act.get("check"))))
    return diffs


class Runner:
    def __init__(self, client, scenario, out_dir, scratch):
        self.client = client
        self.scenario = scenario
        self.out_dir = out_dir
        self.scratch = scratch
        self.shots_dir = out_dir / "shots"
        self.shots_dir.mkdir(parents=True, exist_ok=True)
        self.report = []          # (index, describe, call_line, note, shot_rel)
        self.checks = []          # per-step stable observations for --check
        self.pending = []         # (thread, holder) for blocking invoke_action
        self.step_no = 0
        self.video_t0 = None      # monotonic recording start, if recording
        self.video_rel = None     # report-relative path to the recording
        self.chapters = []        # (offset_seconds, describe) for video chapters
        self.dwell = 0.0          # seconds to linger after each step (video pacing)
        self.cursor_enabled = False  # glide the pointer onto targets (video mode)
        self.key_events = []      # (start, end, text) typed-key bubbles for the video

    def mark(self, describe):
        """Record the current offset into the recording as a chapter start."""
        if self.video_t0 is not None:
            self.chapters.append((time.monotonic() - self.video_t0, describe))

    def dwell_now(self):
        """Linger so a recording shows each state long enough to follow. Only
        active in video mode; a --check run never waits like this."""
        if self.dwell:
            time.sleep(self.dwell)

    def resolve_center(self, query):
        """Root-coordinate centre of the single widget matching query, or None."""
        _, structured, _ = self.client.call("find_widgets", query)
        if not structured or structured.get("count") != 1:
            return None
        w = structured["widgets"][0]
        return (int(w["x"] + w["width"] / 2), int(w["y"] + w["height"] / 2))

    def move_cursor(self, x, y):
        self.client.call("move_cursor", {"x": int(x), "y": int(y)})

    def point_at(self, query):
        """Glide the recorded cursor onto a target before acting on it."""
        if not self.cursor_enabled:
            return
        center = self.resolve_center(query)
        if center:
            self.move_cursor(center[0], center[1])
            time.sleep(min(0.4, self.dwell / 2) if self.dwell else 0.4)

    def subst(self, value):
        if isinstance(value, str):
            return value.replace("{scratch}", str(self.scratch))
        if isinstance(value, dict):
            return {k: self.subst(v) for k, v in value.items()}
        return value

    def record(self, describe, call_line, note="", shot_rel=None, tool=None, check=None):
        self.report.append((self.step_no, describe, call_line, note, shot_rel))
        # The baseline deliberately omits volatile fields (geometry, window
        # ids, transient visible/enabled state): a regression check must react
        # to behaviour changes, not to a window moving a few pixels.
        if check is not None:
            self.checks.append(
                {"step": self.step_no, "describe": describe, "tool": tool, "check": check})

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
            self.record(step["describe"], call_line, tool="call_action",
                        check={"dispatched": True},
                        note="Dispatched (opens a modal dialog; dismissed by a later step).")
        else:
            self.call_or_fail("call_action", {"id": action}, step["describe"])
            self.record(step["describe"], call_line, tool="call_action", check={"ok": True})

    def do_open(self, path, describe):
        self.call_or_fail("open_file", {"path": path}, describe)
        self.record(describe, 'open_file path="{}"'.format(path),
                    tool="open_file", check={"ok": True})

    def do_click(self, step):
        query = self.subst(step["click"])
        self.point_at(query)
        w = self.call_or_fail("click_widget", query, step["describe"])
        self.record(step["describe"], "click_widget " + json.dumps(query),
                    note="Resolved to: " + widget_desc(w),
                    tool="click_widget", check=widget_identity(w))

    def do_type(self, step):
        args = self.subst(step["type"])
        query = {k: v for k, v in args.items() if k != "input"}
        if query:
            self.point_at(query)
        w = self.call_or_fail("type_text", args, step["describe"])
        # Show the typed text as an on-screen bubble for the duration of the
        # step's dwell, so the recording makes the keystrokes visible.
        if self.video_t0 is not None:
            start = time.monotonic() - self.video_t0
            self.key_events.append((start, start + max(self.dwell, 1.5), args.get("input", "")))
        self.record(step["describe"], "type_text " + json.dumps(args),
                    note="Typed into: " + widget_desc(w),
                    tool="type_text", check=widget_identity(w))

    def do_press(self, step):
        # A shortcut/chord, e.g. press: "Ctrl+K" or press: {keys: Escape, ...query}.
        spec = step["press"]
        args = {"keys": spec} if isinstance(spec, str) else self.subst(spec)
        query = {k: v for k, v in args.items() if k != "keys"}
        if query:
            self.point_at(query)
        r = self.call_or_fail("press_keys", args, step["describe"])
        keys = (r or {}).get("keys", args.get("keys", ""))
        if self.video_t0 is not None:
            start = time.monotonic() - self.video_t0
            self.key_events.append((start, start + max(self.dwell, 1.5), keys))
        self.record(step["describe"], "press_keys " + json.dumps(args),
                    note="Pressed " + keys, tool="press_keys", check={"keys": keys})

    def do_select(self, step):
        args = self.subst(step["select"])
        self.point_at({k: v for k, v in args.items() if k != "item"})
        w = self.call_or_fail("select_combo_item", args, step["describe"])
        self.record(step["describe"], "select_combo_item " + json.dumps(args),
                    note="Selected in: " + widget_desc(w),
                    tool="select_combo_item", check=widget_identity(w))

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
        self.record(step["describe"], "widget_exists " + json.dumps(query), note=note,
                    tool="widget_exists", check={"exists": bool(exists), "count": count})

    def do_wait_for(self, step):
        query = self.subst(step["wait_for"])
        timeout = float(step.get("timeout", 15))
        deadline = time.monotonic() + timeout
        count = 0
        while time.monotonic() < deadline:
            r = self.call_or_fail("widget_exists", query, step["describe"])
            if r.get("exists"):
                self.record(step["describe"], "widget_exists " + json.dumps(query),
                            note="Appeared (count {}).".format(r.get("count")),
                            tool="wait_for", check={"appeared": True})
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
                        note="Pane not readable: {}.".format(reason),
                        tool="read_pane", check={"reason": reason})
            return
        text = r.get("text", "")
        artifact = self.out_dir / (slugify(name) + ".txt")
        artifact.write_text(text, encoding="utf-8")
        # The pane text itself is too volatile to baseline (timestamps, paths);
        # the stable check is that the pane was readable at all.
        self.record(step["describe"], 'read_pane name="{}"'.format(name),
                    note="{} chars saved to {}.".format(len(text), artifact.name),
                    tool="read_pane", check={"reason": reason})

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
                    shot_rel=rel, tool="screenshot",
                    check={"width": r.get("width"), "height": r.get("height"),
                           "window_title": r.get("window_title")})

    # --- driver --------------------------------------------------------

    def run(self):
        setup = self.scenario.get("setup", {})
        if setup.get("open"):
            self.step_no += 1
            self.mark("Open " + setup["open"])
            self.do_open(self.subst(setup["open"]), "Open " + setup["open"])
            self.dwell_now()

        for step in self.scenario.get("steps", []):
            self.step_no += 1
            self.mark(step["describe"])
            if "invoke_action" in step:
                self.do_invoke_action(step)
            elif "click" in step:
                self.do_click(step)
            elif "type" in step:
                self.do_type(step)
            elif "press" in step:
                self.do_press(step)
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
            self.dwell_now()

        # Collect any blocking invoke_action calls that a later step released.
        for t, holder, action in self.pending:
            t.join(timeout=10)
            if t.is_alive():
                raise ScenarioError('blocking action "{}" never returned - '
                                    'was it dismissed by a later step?'.format(action))
            is_error, _, text = holder.get("result", (False, None, ""))
            if is_error:
                raise ScenarioError('blocking action "{}" failed: {}'.format(action, text))

        # Let the recording rest on the final state before it stops.
        self.dwell_now()

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
        if self.video_rel:
            lines.append("## Recording")
            lines.append("")
            lines.append("[Watch the recording]({}) (chapters embedded in the file):"
                         .format(self.video_rel))
            lines.append("")
            for offset, describe in self.chapters:
                lines.append("- `{}` {}".format(fmt_offset(offset), describe))
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
    ap.add_argument("--check", action="store_true",
                    help="Regression mode: compare the run against the committed "
                         "baseline and fail on any mismatch")
    ap.add_argument("--update-baseline", action="store_true",
                    help="Write/overwrite the baseline next to the scenario from this run")
    ap.add_argument("--video", action="store_true",
                    help="Screen-record the run to tutorial.mp4 with step chapters "
                         "(records $DISPLAY via ffmpeg x11grab)")
    ap.add_argument("--video-size",
                    help="x11grab capture size WxH (default: auto-detected display size)")
    ap.add_argument("--video-dwell", type=float, default=1.5,
                    help="Seconds to linger on each step in video mode so the "
                         "recording is watchable (default 1.5; ignored without --video)")
    ap.add_argument("--no-cursor", action="store_true",
                    help="In video mode, do not glide the pointer onto targets")
    ap.add_argument("--window-manager", metavar="CMD",
                    help="Launch this window manager on $DISPLAY so the recording shows "
                         "window frames (e.g. 'twm'); terminated at the end. Use a dedicated "
                         "display, not your desktop. Implies full-display capture, since a "
                         "frame sits outside the app's client area.")
    ap.add_argument("--no-preseed", action="store_true",
                    help="Do not pre-seed the launched Creator's settings (only "
                         "relevant with --qtcreator)")
    ap.add_argument("--noload", action="append", default=[], metavar="PLUGIN",
                    help="Pass -noload PLUGIN to a launched Creator (repeatable; e.g. "
                         "--noload Profiler to skip a broken plugin, or --noload all "
                         "with --load)")
    ap.add_argument("--load", action="append", default=[], metavar="PLUGIN",
                    help="Pass -load PLUGIN to a launched Creator (repeatable)")
    args = ap.parse_args()

    scenario_path = Path(args.scenario).resolve()
    scenario = yaml.safe_load(scenario_path.read_text(encoding="utf-8"))
    scenario["_source"] = scenario_path.name
    name = scenario.get("name", scenario_path.stem)

    out_dir = Path(args.out) if args.out else scenario_path.parent / "out" / slugify(name)
    out_dir.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="scenario-"))

    recorder = None
    wm = None
    child = None
    settings = None
    if args.qtcreator:
        settings = scratch / "settings"
        settings.mkdir()
        launch = [args.qtcreator, "-settingspath", str(settings)]
        if not args.no_preseed:
            write_default_settings(settings)
            for plugin in LAUNCH_NOLOAD:
                launch += ["-noload", plugin]
        for plugin in args.noload:
            launch += ["-noload", plugin]
        for plugin in args.load:
            launch += ["-load", plugin]
        # McpServer is loaded last so it survives a user "-noload all".
        launch += ["-load", "McpServer", "-mcp-port", str(args.port)]
        child = subprocess.Popen(launch)

    exit_code = 0
    try:
        if args.window_manager:
            if not os.environ.get("DISPLAY"):
                raise ScenarioError("--window-manager needs a DISPLAY")
            # twm and friends adopt and decorate already-mapped windows on
            # startup, so this works whether Creator is launched or attached.
            wm = subprocess.Popen(shlex.split(args.window_manager),
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if not wait_for_port(args.host, args.port, args.timeout, child):
            raise ScenarioError("MCP port {} did not open within {}s"
                                .format(args.port, args.timeout))
        client = McpClient(args.host, args.port)
        client.initialize()
        runner = Runner(client, scenario, out_dir, scratch)

        if args.video:
            display = os.environ.get("DISPLAY")
            if not display:
                raise ScenarioError("--video needs a DISPLAY to record")
            # Crop the recording to the application window so it has no black
            # display margin (unless an explicit --video-size was requested, or
            # a window manager is drawing frames outside the client area).
            region = None
            if not args.video_size and not args.window_manager:
                _, wins, _ = client.call("list_windows", {})
                windows = wins.get("windows", []) if wins else []
                main = next((w for w in windows if "MainWindow" in w.get("class", "")),
                            windows[0] if windows else None)
                if main:
                    region = (main["x"], main["y"], main["width"], main["height"])
            recorder = Recorder(display, out_dir / "recording.mp4",
                                size=args.video_size, region=region)
            recorder.start()
            runner.video_t0 = recorder.t0
            runner.dwell = args.video_dwell
            runner.cursor_enabled = not args.no_cursor

        try:
            runner.run()
            status = "PASSED"
        except ScenarioError as e:
            status = "FAILED"
            exit_code = 1
            runner.record("(scenario aborted)", str(e))
            print("SCENARIO FAILED:", e, file=sys.stderr)

        if recorder:
            recorder.stop()
            video = recorder.finalize(runner.chapters, out_dir / "tutorial.mp4",
                                      key_events=runner.key_events, font=find_font())
            if video:
                runner.video_rel = os.path.relpath(video, out_dir)
                print("Recording: {}".format(video))

        baseline_path = scenario_path.with_suffix(".baseline.json")
        if status == "PASSED" and args.update_baseline:
            baseline_path.write_text(
                json.dumps({"name": name, "steps": runner.checks}, indent=2) + "\n",
                encoding="utf-8")
            print("Baseline written: {}".format(baseline_path))
        elif status == "PASSED" and args.check:
            if not baseline_path.exists():
                raise ScenarioError("no baseline at {} - run with --update-baseline first"
                                    .format(baseline_path))
            expected = json.loads(baseline_path.read_text(encoding="utf-8"))
            diffs = compare_baseline(expected, runner.checks)
            if diffs:
                status = "CHECK FAILED"
                exit_code = 1
                print("CHECK FAILED against {}:".format(baseline_path), file=sys.stderr)
                for d in diffs:
                    print("  " + d, file=sys.stderr)
                    runner.record("(check mismatch)", d)
            else:
                status = "CHECK PASSED"

        report = runner.write_report()
        print("{}: {}".format(status, name))
        print("Tutorial: {}".format(report))
    finally:
        if recorder and recorder.proc and recorder.proc.poll() is None:
            recorder.stop(force=True)
        # Only the Creator we launched ourselves is ours to stop; an attached
        # instance is left running for the caller.
        if child and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=10)
            except subprocess.TimeoutExpired:
                child.kill()
        if wm and wm.poll() is None:
            wm.terminate()
            try:
                wm.wait(timeout=5)
            except subprocess.TimeoutExpired:
                wm.kill()

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
