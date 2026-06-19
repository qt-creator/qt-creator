# Native-mixed profiling for Qt Creator -- design proposal

Status: draft for review
Scope of this document: the *seam* between a native sampler and Creator's
timeline, the wire-protocol question, the V4 symbolication contract, the
plugin merge, and the cross-platform plan. It deliberately stops short of an
implementation plan -- that follows once the seam is agreed.

---

## 1. Goal and non-goals

**Goal.** Give the QML profiler the equivalent of the debugger's "native mixed"
mode: a single call stack / flame graph that interleaves C++ and QML/JS frames,
so time spent below the JS/engine boundary (slots, property getters, C++ models,
anything reached from JS) is attributable instead of opaque.

**Non-goals.**

- Replacing instrumentation. The existing QML-protocol timeline stays -- it
  gives exact binding/signal counts and durations that sampling cannot. The
  merged stacks are a *second*, complementary view.
- A new rendering stack. The timeline and flame-graph widgets are reused as-is.
- Live cross-platform capture in v1 (see Section 8).

---

## 2. Where we are (confirmed against the tree)

- `Timeline::TimelineModel` (`src/libs/tracing/timelinemodel.h`) is generic. Its
  storage unit is `Range { qint64 start; qint64 duration; int selectionId; int
  parent; int endIndex; }`. `selectionId` is an opaque grouping key.
  `computeNesting()` requires perfectly nested ranges -- call stacks satisfy this
  for free.
- `FlameGraphWidget` (`src/libs/tracing/flamegraphwidget.h`) takes any
  `QAbstractItemModel`; it is decoupled from `TimelineModel`.
- Two non-QML `TimelineModel` subclasses already exist: `CtfTimelineModel`
  (JSON/CTF) and `PerfTimelineModel` (native sampled stacks). The CTF visualizer
  *lives inside the `profiler` plugin* (the former qmlprofiler, now namespace
  `Profiler`) -- precedent for an umbrella plugin.
- `PerfTimelineModel::updateFrames()` (`perftimelinemodel.cpp:289`) diffs
  consecutive samples' frame arrays against `m_currentStack`, opening/closing
  ranges as frames change. It consumes any `PerfEvent` with `frames()` +
  `timestamp()`.
- perf acquisition is Linux-only in practice (`perfprofilerruncontrol.cpp:108`
  shells out to `perf record`); supports remote Linux / QDB.
- The perfparser tool and qtdeclarative/V4 are **not** in this checkout
  (`src/tools/perfparser/` is an empty submodule) -- so producer-side work lands
  in other repos.

---

## 3. Central finding: a JS frame is *already representable*

A perf sample's stack is `PerfEvent::frames()` -- a `QList<qint32>` of **location
ids**, innermost first. Each location id resolves through two tables that are
populated by ordinary wire events:

```cpp
// perfeventtype.h
struct Location {                 // LocationDefinition
    quint64 address = 0;
    qint32 file = -1;             // string id  <-- source file
    quint32 pid = 0;
    qint32 line = -1;            // <-- source line
    qint32 column = -1;          // <-- source column
    qint32 parentLocationId = -1; // inlining chain
};

// perfprofilertracemanager.h
struct Symbol {                   // SymbolDefinition
    qint32 name;                  // string id <-- function name
    qint32 binary;                // string id <-- "binary" / module
    qint32 path;
    bool isKernel;
};
```

A QML/JS frame is therefore expressible **without any new sample-side format**: it
is just a location id whose `Location.file`/`line`/`column` point at the `.qml`/
`.js` source and whose `Symbol.name` is the JS function name. The model keys on
location *id*, not on a real instruction address, so synthetic (JIT/dynamic) code
needs no real ELF address -- `address` can be the JIT address or `0`.

**Consequence:** the consumer side (timeline model, flame graph, statistics) needs
*almost no change* to display merged stacks. The work is concentrated on the
producer.

---

## 4. The seam

### 4.1 What the producer must do

1. **Recognize V4 frames** in each raw native stack -- the contiguous run of
   `QV4::...` frames (interpreter) or JIT'd regions (compiled JS).
2. **Replace or annotate** that run with synthetic JS location ids (decision
   below), emitting `LocationDefinition` / `SymbolDefinition` / `StringDefinition`
   for each new JS function the first time it is seen.
3. **Reconstruct the JS call frames** so multiple nested JS calls become multiple
   frames, not one opaque `interpret` blob:
   - **JIT'd JS** -- via perf jitdump (`jit-PID.dump`) / `perf-PID.map`, which
     maps JIT address ranges to function names. This is the comparatively easy
     path.
   - **Interpreted JS** -- the hard path: walk the V4 JS call frames out of the
     interpreter state, exactly the knowledge the native-mixed *dumpers* already
     encode. The native C++ stack only shows repeated `interpret` frames; the JS
     frames must be read from the engine's frame chain.

### 4.2 Replace vs interleave

Two presentations of a mixed stack:

- **Replace** the V4 machinery frames with the JS frames they implement
  (`onClicked` -> `compute()` instead of `QV4::Function::call` ->
  `interpret` -> ...). Cleaner for the typical QML developer.
- **Interleave** -- keep the C++ engine frames *and* show the JS frames, so engine
  overhead is visible. Better for engine hackers.

Recommendation: **replace by default, with an option to reveal engine frames.**
Implementable purely in how the producer composes the `frames()` list, so the
choice does not touch the model.

### 4.3 The one genuinely new wire field (optional but wanted)

To colour C++ vs JS frames and to let statistics group by language, the consumer
needs to know a location's *kind*. Options, smallest first:

- **Convention only:** a distinguished `Symbol.binary` string such as `"[QML]"` /
  `"[JS]"`. Zero protocol change; slightly hacky.
- **A `kind`/flag field** on `Symbol` (e.g. `enum SymbolKind { Native, JsJit,
  JsInterpreted }`). One field added to `SymbolDefinition`
  (`operator<<`/`>>` in perfprofilertracemanager.h) plus a default for backward
  compatibility.

Recommendation: the `kind` field -- it is one `qint32`, versions cleanly, and
keeps colouring/statistics honest. Everything else in the wire protocol
(`PerfEventType::Feature`: `Sample`, `LocationDefinition`, `SymbolDefinition`,
`StringDefinition`, ...) stays as-is.

### 4.4 Clock correlation

The two streams use different clocks: the QML debug protocol timeline vs the
sampler's clock (perf uses `CLOCK_MONOTONIC` or TSC). The timeline model's
absolute base (`TimelineTraceManager::traceStart/traceEnd`) is fine; the work is
aligning the two *sources*:

- Best: have both reference a common monotonic clock at capture (perf supports
  `-k CLOCK_MONOTONIC`; the QML engine timestamps would need to be expressed in,
  or convertible to, the same base).
- Fallback: a correlation anchor -- a known event emitted into both streams (e.g.
  a marker at profiling start) to compute a fixed offset; accept residual skew.

For v1 we can sidestep tight alignment by presenting the merged flame graph
(aggregate, clock-tolerant) first, and only later overlay sampled stacks onto the
instrumentation *timeline* where exact alignment matters.

---

## 5. Consumer-side changes (small)

- **Model:** none required to *display* merged stacks -- JS frames arrive as
  ordinary location ids. If the `kind` field is added, surface it via a model role.
- **Flame graph:** add a `kind`-based colour role so JS vs C++ is visually
  distinct (`PerfProfilerFlameGraphModel` already maps roles).
- **Statistics / call tree:** optionally a "group by language" or a JS/C++ filter.
- **Details popup:** show the source location (already carried in `Location`).

---

## 6. Folding `perfprofiler` into the `profiler` plugin

The QML profiler plugin has already been generalised: it is now `profiler`
(`src/plugins/profiler`, namespace `Profiler`) and is no longer QML-only. So this
is folding `perfprofiler` into the existing `profiler` plugin, not creating a new
one.

- **Not required to share the model** -- that already happens via
  `src/libs/tracing`.
- **Required for a single fused perspective** -- the merged view needs one tool
  owning both streams on one `TimelineZoomControl` and one trace clock; separate
  plugins/perspectives cannot share zoom/selection cleanly.
- **Precedent:** the CTF visualizer already lives inside `profiler`.
- **V4 glue wants to live here:** the symbolication contract is QML-engine-aware,
  so it belongs next to the existing profiler code.
- **Cost is low:** perfparser is an external process, so merging does not pull
  elfutils into Creator itself; the diff is mechanical (perspective, tool,
  run-control registration, build files).

Recommendation: do the merge as an early, separate change so the fused view has a
home, but *after* the v1 model/flame-graph slice has proven the rendering.

---

## 7. Producer-side note (out of this checkout)

The V4 recognition + reconstruction work lives in perfparser (symbolication,
jitdump ingestion) and qtdeclarative (jitdump emission, interpreter frame
exposure). Neither is in this libtracing checkout. The contract this document
fixes -- "JS frames are location ids with source-bearing `Location`s and a `kind`
on `Symbol`" -- is what lets that work proceed independently.

---

## 8. Cross-platform plan

Decompose the native side into two independently-portable layers; the
Creator/UI/model layer is already platform-neutral, and the QML-protocol
transport is cross-platform.

**Layer 1 -- raw sampling (per-OS kernel facility):**

- Linux: perf (have it).
- Windows: ETW (`SampledProfile` + stackwalk via `wpr`/xperf), or in-proc
  `StackWalk64`/DbgHelp.
- macOS: no perf; `xctrace`/Instruments time-profiler, `sample`/`spindump`, or an
  own `thread_get_state` sampler (needs entitlements). Live capture is the painful
  one here.

**Layer 2 -- symbolication (per-object-format -- the underrated cost):**

- ELF/DWARF (Linux): perfparser does it.
- PE/PDB (Windows), Mach-O/dSYM (macOS): perfparser is libdw/elfutils-centric, so
  these are real new backends.

**Key property:** V4 frame reconstruction is OS-independent (it reads the engine),
so it is written once behind the interchange and reused on every platform. Only
Layer 1 and the C++ side of Layer 2 are per-OS.

**Interchange strategy.** Two neutral formats already have hooks in-tree and map
to the two model shapes:

- *Sampled-stack interchange* -> reuse `PerfTimelineModel`/flame graph. Either
  generalize the perfparser binary protocol into an OS-neutral "stack sample"
  protocol, or use Chrome Trace Format's `sample`/`stackFrames` records. Best fit
  for native-mixed, but the current `ctfloader` only handles duration phases
  (B/E/X/C/i, `ctfloader.cpp:132`); sampled records need loader work.
- *Duration-event interchange* (Chrome B/E/X JSON -> `CtfTimelineModel`) already
  works, but it is not sampling, so it is the wrong tool for stack profiles.

So "import a Chrome-JSON file" is cheap cross-platform support only for *duration*
traces; for true sampled native-mixed we extend an ingest path regardless.
Pragmatically, Windows/macOS arrive via *file import* (ETW/xctrace -> interchange)
before live capture.

---

## 9. Proposed milestones

1. **M1 -- agree this seam** (this document): JS-frame-as-location-id, the
   `Symbol.kind` field, replace-by-default presentation, the clock approach.
2. **M2 -- Creator-side vertical slice (Linux, no producer changes):** feed a
   synthetic merged trace (real C++ frames + injected JS location ids) through a
   `PerfTimelineModel`-shaped path; add `kind`-based colouring; confirm the fused
   flame graph + timeline render. De-risks the whole UI claim in this checkout.
3. **M3 -- fold `perfprofiler` into the `profiler` plugin:** one
   tool/perspective/zoom/clock.
4. **M4 -- producer (Linux):** perfparser jitdump ingestion + V4 interpreter
   frame reconstruction; qtdeclarative emits jitdump. End-to-end real native-mixed
   on Linux.
5. **M5 -- clock correlation** for overlaying sampled stacks on the
   instrumentation timeline.
6. **M6 -- cross-platform:** define the OS-neutral interchange; Windows
   (ETW->interchange) and macOS (xctrace->interchange) via file import first.

---

## 10. Open questions / risks

- **Interpreter reconstruction** is the highest-risk producer item; jitdump-only
  (JIT'd code) is a viable reduced-scope first cut if interpreter walking slips.
- **Clock skew** between QML-protocol and sampler streams; mitigated by deferring
  timeline overlay (M5) behind the aggregate flame graph (M2/M4).
- **Symbolication backends** (PDB, dSYM) are substantial; cross-platform is
  genuinely a separate effort, not a flag flip.
- **`kind` field vs convention:** confirm whether a protocol-version bump is
  acceptable, or whether the `"[QML]"`-binary convention is preferred to avoid
  touching the wire format at all.

---

## Appendix: key code references

| What | Where |
| --- | --- |
| Generic timeline model + `Range` | `src/libs/tracing/timelinemodel.h`, `timelinemodel_p.h` |
| Perfect-nesting requirement | `TimelineModel::computeNesting()` |
| Trace clock | `TimelineTraceManager` (`traceStart`/`traceEnd`) |
| Flame-graph widget (any QAbstractItemModel) | `src/libs/tracing/flamegraphwidget.h` |
| Wire feature enum | `PerfEventType::Feature` (`perfeventtype.h:17`) |
| `Location` struct (source-bearing) | `perfeventtype.h:75` |
| `Symbol` struct (+ serialization) | `perfprofilertracemanager.h:67`, `:205` |
| Sample shape (`frames()` = location ids) | `perfevent.h:41` |
| Stack diffing | `perftimelinemodel.cpp:289` (`updateFrames()`) |
| perf acquisition (Linux-only) | `perfprofilerruncontrol.cpp:108` |
| CTF / Chrome-JSON loader | `ctfloader.cpp:132` |
