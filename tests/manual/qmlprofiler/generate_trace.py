#!/usr/bin/env python3
# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""
Generate a synthetic QML profiler trace (.qtd or .qzt) for load-testing and UI development.

Produces a trace that resembles a real profiler run of a large Qt Quick application:
startup compilation and component creation, steady-state animation frames, bindings,
signal handlers, scene-graph frames, memory allocations, and pixmap cache events.

The output format is determined by the file extension: .qtd for XML, .qzt for binary.
"""

import argparse
import heapq
import os
import random
import struct
import sys
import xml.etree.ElementTree as ET
import zlib

PROFILER_FILE_VERSION = "1.02"
US_PER_S = 1_000_000  # microseconds per second

# ---------------------------------------------------------------------------
# Simulated source tree of a large Qt Quick application
# ---------------------------------------------------------------------------

# (qrc-path, component-name, [source lines used for binding/signal locations])
QML_COMPONENTS = [
    ("qrc:/main.qml",                       "main",               [10, 22, 35, 48]),
    ("qrc:/ui/AppWindow.qml",               "AppWindow",          [8,  15, 20, 45, 60]),
    ("qrc:/ui/NavigationBar.qml",           "NavigationBar",      [12, 18, 30, 42]),
    ("qrc:/ui/NavigationItem.qml",          "NavigationItem",     [5,  10, 15, 20]),
    ("qrc:/ui/StatusBar.qml",               "StatusBar",          [6,  12, 18]),
    ("qrc:/ui/ToolBar.qml",                 "ToolBar",            [8,  14, 25, 36]),
    ("qrc:/views/DashboardView.qml",        "DashboardView",      [15, 30, 50, 80, 110]),
    ("qrc:/views/DetailView.qml",           "DetailView",         [20, 40, 60, 75]),
    ("qrc:/views/ListingView.qml",          "ListingView",        [10, 25, 45, 70, 90]),
    ("qrc:/views/SettingsView.qml",         "SettingsView",       [12, 18, 35, 52]),
    ("qrc:/views/SearchView.qml",           "SearchView",         [8,  16, 28, 44]),
    ("qrc:/components/Card.qml",            "Card",               [6,  12, 20, 30]),
    ("qrc:/components/Avatar.qml",          "Avatar",             [4,  8,  14]),
    ("qrc:/components/Badge.qml",           "Badge",              [3,  6,  10]),
    ("qrc:/components/Button.qml",          "Button",             [5,  10, 16, 24]),
    ("qrc:/components/Spinner.qml",         "Spinner",            [4,  8,  13]),
    ("qrc:/components/TextField.qml",       "TextField",          [7,  14, 22, 32]),
    ("qrc:/components/Slider.qml",          "Slider",             [6,  12, 19]),
    ("qrc:/components/Toggle.qml",          "Toggle",             [4,  8,  13]),
    ("qrc:/components/ProgressBar.qml",     "ProgressBar",        [5,  10, 16]),
    ("qrc:/components/Tooltip.qml",         "Tooltip",            [6,  12, 18]),
    ("qrc:/delegates/ListDelegate.qml",     "ListDelegate",       [8,  15, 25, 35]),
    ("qrc:/delegates/GridDelegate.qml",     "GridDelegate",       [7,  14, 22, 30]),
    ("qrc:/delegates/CompactDelegate.qml",  "CompactDelegate",    [5,  10, 16]),
    ("qrc:/models/DataModel.qml",           "DataModel",          [12, 20, 30]),
    ("qrc:/models/FilterModel.qml",         "FilterModel",        [10, 18, 28]),
    ("qrc:/utils/ThemeManager.qml",         "ThemeManager",       [8,  14, 22]),
    ("qrc:/utils/LocaleHelper.qml",         "LocaleHelper",       [6,  12, 18]),
    ("qrc:/effects/FadeEffect.qml",         "FadeEffect",         [5,  10, 15]),
    ("qrc:/effects/SlideEffect.qml",        "SlideEffect",        [5,  10, 15]),
]

# (property, binding expression) -- used for Binding event detail text
BINDINGS = [
    ("width",              "parent.width - 2 * margin"),
    ("height",             "contentHeight + padding"),
    ("visible",            "model.count > 0"),
    ("color",              "highlighted ? theme.accent : theme.surface"),
    ("opacity",            "enabled ? 1.0 : 0.5"),
    ("text",               "model.displayName"),
    ("font.pixelSize",     "root.baseFontSize * scaleFactor"),
    ("anchors.leftMargin", "sidebar.visible ? sidebar.width : 0"),
    ("x",                  "parent.width / 2 - width / 2"),
    ("y",                  "header.height + spacing"),
    ("radius",             "Settings.cornerRadius"),
    ("scale",              "pressed ? 0.95 : 1.0"),
    ("clip",               "contentWidth > width"),
    ("enabled",            "!busyIndicator.running"),
    ("spacing",            "Math.max(4, root.density * 8)"),
    ("z",                  "hovered ? 2 : 1"),
    ("source",             "imageProvider.url(model.id)"),
    ("rotation",           "orientation === Qt.Horizontal ? 0 : 90"),
]

# (handler-name, detail) -- used for HandlingSignal events
SIGNAL_HANDLERS = [
    ("onClicked",             ""),
    ("onCountChanged",        "updateDisplay()"),
    ("onModelChanged",        "reload()"),
    ("onCurrentIndexChanged", "loadDetails(currentIndex)"),
    ("onVisibleChanged",      "visible ? show() : hide()"),
    ("onWidthChanged",        "relayout()"),
    ("onActiveChanged",       "active ? start() : stop()"),
    ("Component.onCompleted", "initialize()"),
    ("onPressed",             ""),
    ("onReleased",            ""),
]

# (function-name, detail) -- used for Javascript events
JS_FUNCTIONS = [
    ("updateDisplay",  ""),
    ("reload",         ""),
    ("loadDetails",    ""),
    ("relayout",       ""),
    ("initialize",     ""),
    ("computeLayout",  ""),
    ("filterItems",    ""),
    ("sortByField",    ""),
]

PIXMAP_URLS = [
    "qrc:/images/logo.png",
    "qrc:/images/avatar_placeholder.png",
    "qrc:/images/background.jpg",
    "qrc:/images/hero_banner.jpg",
    "qrc:/icons/home.svg",
    "qrc:/icons/search.svg",
    "qrc:/icons/settings.svg",
    "qrc:/icons/back.svg",
    "qrc:/icons/close.svg",
    "qrc:/icons/menu.svg",
]

# (sgEventType value, display name)
# Indices match SceneGraphFrameType enum in qmlprofilereventtypes.h
SG_FRAME_TYPES = [
    (0, "SceneGraph Renderer"),     # SceneGraphRendererFrame
    (3, "SceneGraph Render Loop"),  # SceneGraphRenderLoopFrame
    (6, "SceneGraph Polish+Sync"),  # SceneGraphPolishAndSync
    (9, "SceneGraph Polish"),       # SceneGraphPolishFrame
]

# (QtMsgType value, label)
DEBUG_LEVELS = {0: "Debug", 1: "Warning", 2: "Critical"}

DEBUG_MESSAGES = [
    (1, "QML binding loop detected"),
    (1, "Image load error: file not found"),
    (0, "Model refreshed: 124 items"),
    (0, "View layout complete"),
    (2, "Network request timed out after 5000ms"),
    (1, "Delegate recycled with invalid index"),
    (0, "Theme changed to dark mode"),
    (1, "Property 'implicitWidth' was set on a non-writable object"),
    (0, "Animation controller: 3 active animations"),
    (2, "Out of memory allocating texture"),
]


# ---------------------------------------------------------------------------
# Event type registry
# ---------------------------------------------------------------------------

class TypeRegistry:
    """Assigns sequential indices to event type descriptors."""

    def __init__(self):
        self._types = []

    def add(self, **kwargs):
        idx = len(self._types)
        self._types.append({"index": idx, **kwargs})
        return idx

    @property
    def types(self):
        return self._types


def register_types(registry, rng):
    """
    Register all event types. Returns a dict with named index (sub-)collections:
        compile   : list of int, one per QML_COMPONENTS entry
        create    : list of int, one per QML_COMPONENTS entry
        bind      : dict (comp_idx, bind_idx) -> int
        signal    : dict (comp_idx, handler_idx) -> int  (first 12 components)
        js        : dict (comp_idx, func_idx) -> int     (first 8 components)
        anim      : int
        sg        : dict sg_type_value -> int
        pixmap    : dict (pixmap_idx, event_type) -> int
        mem       : dict mem_type_value -> int
        dbg       : dict type_idx -> (level, text)
    """
    idx = {}

    # Compiling (one per file)
    idx["compile"] = []
    for (path, name, _lines) in QML_COMPONENTS:
        i = registry.add(
            range_type="Compiling", message=None,
            displayname=f"Compiling {name}",
            filename=path, line=1, column=0, details="",
        )
        idx["compile"].append(i)

    # Creating (one per component)
    idx["create"] = []
    for (path, name, lines) in QML_COMPONENTS:
        i = registry.add(
            range_type="Creating", message=None,
            displayname=name,
            filename=path, line=lines[0], column=0, details="",
        )
        idx["create"].append(i)

    # Binding (per component * per binding expression)
    idx["bind"] = {}
    for ci, (path, name, lines) in enumerate(QML_COMPONENTS):
        for bi, (prop, expr) in enumerate(BINDINGS):
            line = lines[bi % len(lines)]
            col = rng.randint(5, 24)
            i = registry.add(
                range_type="Binding", message=None,
                displayname=f"{name}.{prop}",
                filename=path, line=line, column=col, details=expr,
                binding_type=0,  # QmlBinding
            )
            idx["bind"][(ci, bi)] = i

    # HandlingSignal (first 12 components * all handlers)
    idx["signal"] = {}
    for ci, (path, name, lines) in enumerate(QML_COMPONENTS[:12]):
        for si, (handler, code) in enumerate(SIGNAL_HANDLERS):
            line = lines[si % len(lines)]
            i = registry.add(
                range_type="HandlingSignal", message=None,
                displayname=f"{name}: {handler}",
                filename=path, line=line, column=0, details=code,
            )
            idx["signal"][(ci, si)] = i

    # Javascript (first 8 components * all functions)
    idx["js"] = {}
    for ci, (path, name, lines) in enumerate(QML_COMPONENTS[:8]):
        for ji, (fname, code) in enumerate(JS_FUNCTIONS):
            line = lines[ji % len(lines)] + 3
            i = registry.add(
                range_type="Javascript", message=None,
                displayname=f"{name}::{fname}()",
                filename=path, line=line, column=0, details=code,
            )
            idx["js"][(ci, ji)] = i

    # AnimationFrame point event (detailType 3 = AnimationFrame enum value)
    idx["anim"] = registry.add(
        range_type=None, message="Event",
        displayname="Animation Frame",
        filename="", line=0, column=0, details="",
        detail_elem="animationFrame", detail_val=3,
    )

    # SceneGraph frame events
    idx["sg"] = {}
    for (sg_val, sg_name) in SG_FRAME_TYPES:
        i = registry.add(
            range_type=None, message="SceneGraph",
            displayname=sg_name,
            filename="", line=0, column=0, details="",
            detail_elem="sgEventType", detail_val=sg_val,
        )
        idx["sg"][sg_val] = i

    # Pixmap cache events (start=3, finish=4, size-known=0, per pixmap URL)
    idx["pixmap"] = {}
    for pi, url in enumerate(PIXMAP_URLS):
        for (pval, pname) in [(3, "Load Start"), (4, "Load Finish"), (0, "Size Known")]:
            basename = url.rsplit("/", 1)[-1]
            i = registry.add(
                range_type=None, message="PixmapCache",
                displayname=f"Pixmap {pname}: {basename}",
                filename=url, line=0, column=0, details="",
                detail_elem="cacheEventType", detail_val=pval,
            )
            idx["pixmap"][(pi, pval)] = i

    # Memory allocation events (0=HeapPage, 1=LargeItem, 2=SmallItem)
    idx["mem"] = {}
    for (mval, mname) in [(0, "Heap Page"), (1, "Large Item"), (2, "Small Item")]:
        i = registry.add(
            range_type=None, message="MemoryAllocation",
            displayname=f"Memory: {mname}",
            filename="", line=0, column=0, details="",
            detail_elem="memoryEventType", detail_val=mval,
        )
        idx["mem"][mval] = i

    # Debug messages
    idx["dbg"] = {}
    for (level, text) in DEBUG_MESSAGES:
        label = DEBUG_LEVELS.get(level, "Info")
        i = registry.add(
            range_type=None, message="DebugMessage",
            displayname=f"[{label}] {text[:32]}",
            filename="", line=0, column=0, details=text,
            detail_elem="level", detail_val=level,
        )
        idx["dbg"][i] = (level, text)

    return idx


# ---------------------------------------------------------------------------
# XML helpers
# ---------------------------------------------------------------------------

def write_event_type(parent, t):
    """Append an <event> element for one type definition."""
    ev = ET.SubElement(parent, "event")
    ev.set("index", str(t["index"]))

    dn = ET.SubElement(ev, "displayname")
    dn.text = t["displayname"]

    tp = ET.SubElement(ev, "type")
    tp.text = t["range_type"] if t.get("range_type") else t.get("message", "Event")

    if t.get("filename"):
        ET.SubElement(ev, "filename").text = t["filename"]
        ET.SubElement(ev, "line").text = str(t["line"])
        ET.SubElement(ev, "column").text = str(t["column"])

    if t.get("details"):
        ET.SubElement(ev, "details").text = t["details"]

    if t.get("range_type") == "Binding":
        ET.SubElement(ev, "bindingType").text = str(t.get("binding_type", 0))
    elif t.get("detail_elem"):
        ET.SubElement(ev, t["detail_elem"]).text = str(t["detail_val"])


def append_range(parent, start, duration, type_idx):
    """Append a <range> element for a range event (has duration)."""
    r = ET.SubElement(parent, "range")
    r.set("startTime", str(start))
    r.set("duration", str(duration))
    r.set("eventIndex", str(type_idx))


def append_point(parent, timestamp, type_idx, **extra):
    """Append a <range> element for a point event (no duration)."""
    r = ET.SubElement(parent, "range")
    r.set("startTime", str(timestamp))
    r.set("eventIndex", str(type_idx))
    for k, v in extra.items():
        r.set(k, str(v))


# ---------------------------------------------------------------------------
# Event generation
# ---------------------------------------------------------------------------

def generate_events(idx, duration_s, fps, rng):
    """
    Return a sorted list of event dicts:
        {"start": int, "duration": int|None, "type_idx": int, "extra": dict}
    """
    trace_end = duration_s * US_PER_S
    frame_us = US_PER_S // fps
    events = []

    def range_ev(start, dur, tidx):
        events.append({"start": start, "duration": max(0, dur), "type_idx": tidx, "extra": {}})

    def point_ev(ts, tidx, **kw):
        events.append({"start": ts, "duration": None, "type_idx": tidx, "extra": kw})

    # ------------------------------------------------------------------
    # Startup phase: compilation (0 - ~1 s), creation (0.5 s - ~2 s)
    # ------------------------------------------------------------------
    t = 0
    for ci, _comp in enumerate(QML_COMPONENTS):
        dur = rng.randint(6_000, 90_000)
        range_ev(t, dur, idx["compile"][ci])
        t += dur + rng.randint(200, 3_000)
        if t > 1_200_000:
            break

    t = 400_000  # component creation starts at ~0.4 s
    for ci, (_path, _name, _lines) in enumerate(QML_COMPONENTS):
        create_dur = rng.randint(800, 25_000)
        range_ev(t, create_dur, idx["create"][ci])

        # Initial property bindings evaluated during creation
        bt = t + rng.randint(50, 300)
        for bi in rng.choices(range(len(BINDINGS)), k=rng.randint(3, 10)):
            if (ci, bi) not in idx["bind"]:
                continue
            bdur = rng.randint(40, 1_200)
            range_ev(bt, bdur, idx["bind"][(ci, bi)])
            bt += bdur + rng.randint(10, 150)

        # Component.onCompleted handler
        if (ci, 7) in idx["signal"]:
            sd = rng.randint(200, 4_000)
            range_ev(t + create_dur - 100, sd, idx["signal"][(ci, 7)])

        t += create_dur + rng.randint(300, 4_000)
        if t > 2_400_000:
            break

    # Initial memory allocations during startup
    mem_t = 50_000
    for _ in range(rng.randint(20, 50)):
        mval = rng.choice([0, 1, 2])
        amount = rng.choice([4_096, 8_192, 16_384, 32_768, 65_536, 131_072])
        point_ev(mem_t, idx["mem"][mval], amount=amount)
        mem_t += rng.randint(5_000, 50_000)

    # ------------------------------------------------------------------
    # Per-frame steady state
    # ------------------------------------------------------------------
    for frame in range(fps * duration_s):
        f0 = frame * frame_us
        if f0 >= trace_end:
            break

        # -- Animation frame point event --
        point_ev(f0 + rng.randint(0, 1_500), idx["anim"],
                 framerate=fps,
                 animationcount=rng.randint(1, 8),
                 thread=0)

        # -- Scene graph timing events --
        sg_offset = rng.randint(600, 3_000)
        for (sg_val, _sg_name) in SG_FRAME_TYPES:
            if sg_val not in idx["sg"]:
                continue
            t1 = rng.randint(500,  8_000)
            t2 = rng.randint(200,  3_000)
            t3 = rng.randint(100,  2_000)
            t4 = rng.randint(50,   1_000)
            t5 = rng.randint(50,     500)
            point_ev(f0 + sg_offset, idx["sg"][sg_val],
                     timing1=t1, timing2=t2, timing3=t3, timing4=t4, timing5=t5)
            sg_offset += rng.randint(100, 600)

        # -- Binding re-evaluations --
        for _ in range(rng.randint(4, 22)):
            ci = rng.randrange(len(QML_COMPONENTS))
            bi = rng.randrange(len(BINDINGS))
            if (ci, bi) not in idx["bind"]:
                continue
            bstart = f0 + rng.randint(800, frame_us - 1_000)
            # Occasional expensive binding (layout, model access, etc.)
            bdur = rng.randint(2_000, 18_000) if rng.random() < 0.03 \
                else rng.randint(15, 2_500)
            range_ev(bstart, bdur, idx["bind"][(ci, bi)])

        # -- Signal handler (~30% of frames) --
        if rng.random() < 0.30:
            ci = rng.randrange(12)
            si = rng.randrange(len(SIGNAL_HANDLERS))
            if (ci, si) in idx["signal"]:
                sstart = f0 + rng.randint(1_500, frame_us - 2_000)
                sdur = rng.randint(80, 2_500)
                range_ev(sstart, sdur, idx["signal"][(ci, si)])

                # JS called from within the signal handler (~50%)
                if rng.random() < 0.5:
                    jci = rng.randrange(8)
                    jji = rng.randrange(len(JS_FUNCTIONS))
                    if (jci, jji) in idx["js"]:
                        jdur = min(rng.randint(60, 4_000), sdur - 30)
                        if jdur > 0:
                            range_ev(sstart + 15, jdur, idx["js"][(jci, jji)])

        # -- Standalone JS execution (~8% of frames) --
        if rng.random() < 0.08:
            jci = rng.randrange(8)
            jji = rng.randrange(len(JS_FUNCTIONS))
            if (jci, jji) in idx["js"]:
                range_ev(f0 + rng.randint(0, frame_us), rng.randint(100, 6_000),
                         idx["js"][(jci, jji)])

        # -- Memory allocation/free (~5% of frames) --
        if rng.random() < 0.05:
            mval = rng.choice([0, 1, 2])
            mult = rng.choice([-1, -1, 1, 1, 1, 2, 4])
            amount = mult * rng.choice([4_096, 8_192, 16_384, 32_768])
            point_ev(f0 + rng.randint(0, frame_us), idx["mem"][mval], amount=amount)

        # -- Pixmap cache event (~2% of frames) --
        if rng.random() < 0.02:
            pi = rng.randrange(len(PIXMAP_URLS))
            load_start = f0 + rng.randint(0, frame_us // 2)
            if (pi, 3) in idx["pixmap"]:
                point_ev(load_start, idx["pixmap"][(pi, 3)])
            latency = rng.randint(8_000, 80_000)
            if (pi, 4) in idx["pixmap"] and load_start + latency < trace_end:
                point_ev(load_start + latency, idx["pixmap"][(pi, 4)])
            if (pi, 0) in idx["pixmap"]:
                w = rng.choice([16, 32, 64, 128, 256, 512])
                point_ev(load_start + rng.randint(500, 5_000),
                         idx["pixmap"][(pi, 0)],
                         width=w, height=w, refCount=1)

        # -- Debug message (~0.5% of frames) --
        if rng.random() < 0.005:
            type_idx = rng.choice(list(idx["dbg"]))
            _level, text = idx["dbg"][type_idx]
            point_ev(f0 + rng.randint(0, frame_us), type_idx, text=text)

    # ------------------------------------------------------------------
    # Clamp all timestamps to [0, trace_end] and sort
    # ------------------------------------------------------------------
    for e in events:
        e["start"] = max(0, min(e["start"], trace_end))
        if e["duration"] is not None:
            e["duration"] = max(0, min(e["duration"], trace_end - e["start"]))

    events.sort(key=lambda e: e["start"])
    return events


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# .qzt binary format
# ---------------------------------------------------------------------------
#
# QDataStream primitives (big-endian integers; strings use native-endian UTF-16
# via writeRawData, which is little-endian on all common Qt Creator platforms).

def _qs_int(fmt, v):
    return struct.pack(fmt, v)

def qs_quint8(v):  return _qs_int('>B', v)
def qs_qint32(v):  return _qs_int('>i', v)
def qs_quint32(v): return _qs_int('>I', v)
def qs_qint64(v):  return _qs_int('>q', v)

def qs_qstring(s):
    """Qt QDataStream QString: quint32 byte-length (BE) + UTF-16-LE content."""
    if s is None:
        return qs_quint32(0xFFFFFFFF)
    encoded = s.encode('utf-16-le')
    return qs_quint32(len(encoded)) + encoded

def qs_qbytearray(b):
    """Qt QDataStream QByteArray: quint32 length (BE) + raw bytes."""
    if b is None:
        return qs_quint32(0xFFFFFFFF)
    return qs_quint32(len(b)) + b

def qcompress(data):
    """Qt qCompress: quint32 original-size (BE) + zlib-compressed bytes."""
    return struct.pack('>I', len(data)) + zlib.compress(data)


# Enum mappings for binary serialization
_MSG_ENUM = {
    "Event": 0, "PixmapCache": 6, "SceneGraph": 7,
    "MemoryAllocation": 8, "DebugMessage": 9,
}
_RT_ENUM = {
    "Painting": 0, "Compiling": 1, "Creating": 2,
    "Binding": 3, "HandlingSignal": 4, "Javascript": 5,
}


def ser_event_type_qzt(t):
    """Serialize one QmlEventType for the .qzt binary block."""
    msg = _MSG_ENUM.get(t.get("message") or "", 0xFF)
    rt  = _RT_ENUM.get(t.get("range_type") or "", 0xFF)
    if t.get("range_type") == "Binding":
        detail = t.get("binding_type", 0)
    elif t.get("detail_val") is not None:
        detail = t["detail_val"]
    else:
        detail = -1
    return (qs_qstring(t["displayname"])
            + qs_qstring(t.get("details") or "")
            + qs_qstring(t.get("filename") or "")
            + qs_qint32(t.get("line", -1))
            + qs_qint32(t.get("column", -1))
            + qs_quint8(msg)
            + qs_quint8(rt)
            + qs_qint32(detail))


def _min_type(v):
    """Minimum QDataStream type code (0=1B, 1=2B, 2=4B, 3=8B) for a signed int."""
    if -128 <= v <= 127:       return 0
    if -32768 <= v <= 32767:   return 1
    if -(1 << 31) <= v < (1 << 31): return 2
    return 3

_INT_FMTS = ('>b', '>h', '>i', '>q')
_UINT_FMTS = ('>B', '>H', '>I', '>Q')

def _write_num(v, t, signed=True):
    return struct.pack(_INT_FMTS[t] if signed else _UINT_FMTS[t], v)


def ser_qmlevent(timestamp, type_idx, numbers):
    """
    Serialize a QmlEvent in the packed binary format used by .qzt.

    The first byte packs four 2-bit type selectors:
      bits [1:0] = timestamp width
      bits [3:2] = typeIndex width
      bits [5:4] = dataLength width
      bits [7:6] = per-element data width
    """
    ts_t  = _min_type(timestamp)
    ti_t  = _min_type(type_idx)
    n     = len(numbers)
    # data-length type: Qt uses signed comparison for quint16 values 0..127 -> OneByte
    dl_t  = _min_type(n) if n < 128 else (1 if n < 32768 else 2)
    dt_t  = max((_min_type(v) for v in numbers), default=0)

    type_byte = (ts_t) | (ti_t << 2) | (dl_t << 4) | (dt_t << 6)
    buf = struct.pack('>B', type_byte)
    buf += _write_num(timestamp, ts_t, signed=True)
    buf += _write_num(type_idx,  ti_t, signed=True)
    buf += _write_num(n,         dl_t, signed=False)
    for v in numbers:
        buf += _write_num(v, dt_t, signed=True)
    return buf


RANGE_START = 1   # Message::RangeStart
RANGE_END   = 4   # Message::RangeEnd


def _point_numbers(extra, t):
    """
    Convert a point event's extra dict to the QmlEvent number array.
    Matches the setNumber<qintN> calls in qmlprofilertracefile.cpp loadQtd.
    """
    msg  = t.get("message", "")
    dval = t.get("detail_val", -1)

    if msg == "Event":   # AnimationFrame
        return [int(extra.get("framerate", 0)),
                int(extra.get("animationcount", 0)),
                int(extra.get("thread", 0))]

    if msg == "SceneGraph":
        nums = [int(extra.get(f"timing{i+1}", 0)) for i in range(5)]
        while nums and nums[-1] == 0:
            nums.pop()
        return nums

    if msg == "PixmapCache":
        if dval == 0:   # PixmapSizeKnown
            return [int(extra.get("width",    0)),
                    int(extra.get("height",   0)),
                    int(extra.get("refCount", 0))]
        if dval in (1, 2):  # RefCountChanged / CacheCountChanged
            return [0, 0, int(extra.get("refCount", 0))]
        return []   # LoadingStarted / LoadingFinished / LoadingError

    if msg == "MemoryAllocation":
        return [int(extra.get("amount", 0))]

    if msg == "DebugMessage":
        utf8 = str(extra.get("text", "")).encode("utf-8")
        return [b if b < 128 else b - 256 for b in utf8]

    return []


def _interleave_events(raw_events, types):
    """
    Produce a flat (timestamp, type_idx, numbers) list in .qzt event order:
    range events become a RangeStart + deferred RangeEnd pair; point events
    carry their data numbers directly.  Sorted by timestamp with RangeEnd
    events flushed before the RangeStart of the next range at the same time.
    """
    def sort_key(e):
        return (e["start"], -(e["duration"] if e["duration"] is not None else 0))

    out      = []
    ends     = []   # min-heap of (end_timestamp, type_idx)

    for ev in sorted(raw_events, key=sort_key):
        while ends and ends[0][0] <= ev["start"]:
            end_ts, tidx = heapq.heappop(ends)
            out.append((end_ts, tidx, [RANGE_END]))

        if ev["duration"] is not None:
            out.append((ev["start"], ev["type_idx"], [RANGE_START]))
            heapq.heappush(ends, (ev["start"] + ev["duration"], ev["type_idx"]))
        else:
            nums = _point_numbers(ev["extra"], types[ev["type_idx"]])
            out.append((ev["start"], ev["type_idx"], nums))

    while ends:
        end_ts, tidx = heapq.heappop(ends)
        out.append((end_ts, tidx, [RANGE_END]))

    return out


def ser_qmlnote(type_idx, collapsed_row, start_time, duration, text):
    return (qs_qint32(type_idx)
            + qs_qint32(collapsed_row)
            + qs_qint64(start_time)
            + qs_qint64(duration)
            + qs_qstring(text))


def save_qzt(registry, events, trace_start, trace_end, output_path):
    """Write a .qzt binary trace file."""
    span  = (trace_end - trace_start) // 8
    notes = [
        (0, -1, trace_start,           span, "Startup: QML compilation and component creation"),
        (0, -1, trace_start + 2 * span, span, "Steady state: investigate binding overhead here"),
    ]

    # --- Block 1: compressed event types ---
    types_buf = qs_quint32(len(registry.types))
    for t in registry.types:
        types_buf += ser_event_type_qzt(t)

    # --- Block 2: compressed notes (QList<QmlNote>) ---
    notes_buf = qs_quint32(len(notes))
    for n in notes:
        notes_buf += ser_qmlnote(*n)

    # --- Blocks 3+: compressed events (32 MB chunks) ---
    flat = _interleave_events(events, registry.types)
    MAX_BLOCK = 1 << 25  # 32 MB uncompressed
    chunks    = []
    current   = bytearray()
    for (ts, tidx, nums) in flat:
        current += ser_qmlevent(ts, tidx, nums)
        if len(current) > MAX_BLOCK:
            chunks.append(bytes(current))
            current = bytearray()
    if current:
        chunks.append(bytes(current))

    with open(output_path, 'wb') as f:
        # QDataStream header (version Qt_5_5 for the header fields)
        f.write(qs_qbytearray(b"QMLPROFILER"))
        f.write(qs_qint32(18))        # Qt_5_12 data stream version
        # traceStart / traceEnd
        f.write(qs_qint64(trace_start))
        f.write(qs_qint64(trace_end))
        # Compressed blocks
        f.write(qs_qbytearray(qcompress(types_buf)))
        f.write(qs_qbytearray(qcompress(notes_buf)))
        for chunk in chunks:
            f.write(qs_qbytearray(qcompress(chunk)))


# ---------------------------------------------------------------------------
# .qtd XML format
# ---------------------------------------------------------------------------

def build_xml(registry, events, trace_start, trace_end):
    root = ET.Element("trace")
    root.set("version", PROFILER_FILE_VERSION)
    root.set("traceStart", str(trace_start))
    root.set("traceEnd", str(trace_end))

    event_data = ET.SubElement(root, "eventData")
    event_data.set("totalTime", str(trace_end - trace_start))
    for t in registry.types:
        write_event_type(event_data, t)

    model = ET.SubElement(root, "profilerDataModel")
    for e in events:
        if e["duration"] is not None:
            append_range(model, e["start"], e["duration"], e["type_idx"])
        else:
            append_point(model, e["start"], e["type_idx"], **e["extra"])

    note_data = ET.SubElement(root, "noteData")
    span = (trace_end - trace_start) // 8
    n1 = ET.SubElement(note_data, "note")
    n1.set("startTime", str(trace_start))
    n1.set("duration", str(span))
    n1.set("eventIndex", "0")
    n1.set("collapsedRow", "-1")
    n1.text = "Startup: QML compilation and component creation"

    n2 = ET.SubElement(note_data, "note")
    n2.set("startTime", str(trace_start + 2 * span))
    n2.set("duration", str(span))
    n2.set("eventIndex", "0")
    n2.set("collapsedRow", "-1")
    n2.text = "Steady state: investigate binding overhead here"

    return root


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("-o", "--output", default="large_trace.qtd",
                        help="Output file (default: large_trace.qtd)")
    parser.add_argument("--duration", type=int, default=30,
                        help="Trace duration in seconds (default: 30)")
    parser.add_argument("--fps", type=int, default=60,
                        help="Animation frame rate (default: 60)")
    parser.add_argument("--seed", type=int, default=None,
                        help="Random seed for reproducibility")
    parser.add_argument("-d", "--deterministic", action="store_true",
                        help="Use a fixed seed (0) for fully reproducible output; "
                             "overridden by --seed")
    args = parser.parse_args()

    if args.duration < 1 or args.duration > 3600:
        sys.exit("error: --duration must be between 1 and 3600")
    if args.fps < 1 or args.fps > 240:
        sys.exit("error: --fps must be between 1 and 240")

    seed = args.seed if args.seed is not None else (0 if args.deterministic else None)
    rng = random.Random(seed)
    registry = TypeRegistry()

    print(f"Registering event types for a {args.duration}s / {args.fps}fps trace...")
    idx = register_types(registry, rng)

    print(f"  {len(registry.types)} event types registered")
    print("Generating events...")

    events = generate_events(idx, args.duration, args.fps, rng)
    print(f"  {len(events)} events generated")

    trace_start = 0
    trace_end = args.duration * US_PER_S

    if args.output.endswith(".qzt"):
        print("Writing binary .qzt...")
        save_qzt(registry, events, trace_start, trace_end, args.output)
    else:
        print("Building XML tree...")
        root = build_xml(registry, events, trace_start, trace_end)
        print(f"Writing {args.output}...")
        ET.indent(root, space="  ")
        ET.ElementTree(root).write(args.output, encoding="unicode", xml_declaration=True)

    size_kb = os.path.getsize(args.output) // 1024
    print(f"Done. {args.output}: {size_kb} KB, "
          f"{len(registry.types)} types, {len(events)} events")


if __name__ == "__main__":
    main()
