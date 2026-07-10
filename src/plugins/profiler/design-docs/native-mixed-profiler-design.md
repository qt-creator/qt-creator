# Native-mixed profiling for Qt Creator -- design

Status: living design. Strategy A (Section 5) is implemented in the standalone
`qtprofiler` viewer; Strategy B (Section 6) and the exact-clock and in-Creator
items remain future work.

This document covers two complementary ways to reach one goal -- a call stack /
flame graph that interleaves C++ and QML/JS frames -- the shared representation
and infrastructure they build on, clock correlation, the cross-platform plan,
and the plugin structure.

---

## 1. Goal and non-goals

**Goal.** Give the QML profiler the equivalent of the debugger's "native mixed"
mode: a single call stack / flame graph that interleaves C++ and QML/JS frames,
so time spent below the JS/engine boundary (slots, property getters, C++ models,
anything reached from JS) is attributable instead of opaque -- and, symmetrically,
JS reached from C++ is attributed under that C++ frame.

**Non-goals.**

- Replacing instrumentation. The existing QML-protocol timeline stays -- it gives
  exact binding/signal counts and durations that sampling cannot. The merged
  stacks are a *complementary* view, not a replacement.
- A new rendering stack. The timeline, flame-graph and call-tree widgets are
  reused as-is.
- A single canonical clock in v1 (see Section 7).

---

## 2. Two strategies to the same stack

Both strategies produce the same end state -- a merged stack rendered by the
shared widgets -- and differ only in *where the JS frames come from*.

| | **Strategy A: concurrent capture + splice** | **Strategy B: producer-side reconstruction** |
| --- | --- | --- |
| JS frames from | the QML profiler, recorded concurrently | perfparser + qtdeclarative (jitdump / V4 frame walk) |
| Coverage | JS the QML profiler instruments (bindings, signals, JS functions) | all JS, including uninstrumented |
| Platforms | cross-platform via the `Sampler` abstraction | Linux/perf first; per-OS symbolication |
| Producer changes | none for frames (a clock change is wanted) | substantial (jitdump emission, interpreter frame exposure) |
| Status | **implemented** (qtprofiler viewer) | future |

They coexist: where a producer already supplies reconstructed JS frames, the
Strategy-A splicer finds no engine run to replace and passes the sample through.
The source-bearing, `kind`-tagged frame contract (Section 3) is identical on both
sides.

---

## 3. The common representation: a JS frame is a source-bearing location

The key finding that makes the consumer side almost trivial: **a JS frame is
already representable as an ordinary frame with a source location and a name**, in
both data models.

*Perf/perfparser (Strategy B).* A sample's stack is `PerfEvent::frames()` -- a
`QList<qint32>` of **location ids**, innermost first. Each resolves through two
tables populated by ordinary wire events:

```cpp
// perfeventtype.h
struct Location {                 // LocationDefinition
    quint64 address = 0;
    qint32 file = -1;             // string id  <-- source file
    quint32 pid = 0;
    qint32 line = -1;             // <-- source line
    qint32 column = -1;           // <-- source column
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

A JS frame is a location id whose `Location.file`/`line`/`column` point at the
`.qml`/`.js` source and whose `Symbol.name` is the JS function. The model keys on
location *id*, not a real instruction address, so JIT/dynamic code needs no ELF
address (`address` can be the JIT address or `0`).

*Sampler `SampleTraceData` (Strategy A).* Per tick, a `ThreadSample { quint64
tsUs; tid; bool running; QList<int> frames; }` where `frames` are root-first
indices into a `labels` table, and a `Label { name, file, line, module, offset }`
already carries a source file and line. A JS frame is just a `Label` with a
`.qml`/`.js` `file` and no `module`.

**Consequence:** the consumer (timeline model, flame graph, statistics, call
tree) needs *almost no change* to display merged stacks. The work is concentrated
on producing the frames -- either by splicing (A) or by the producer (B).

---

## 4. Shared infrastructure (confirmed against the tree)

- `Timeline::TimelineModel` (`src/libs/tracing/timelinemodel.h`) is generic. Its
  storage unit is `Range { qint64 start; qint64 duration; int selectionId; int
  parent; int endIndex; }`; `computeNesting()` requires perfectly nested ranges --
  call stacks satisfy this for free.
- `FlameGraphWidget` (`src/libs/tracing/flamegraphwidget.h`) takes any
  `QAbstractItemModel`; it is decoupled from `TimelineModel`.
- The `qmlprofiler` and `perfprofiler` plugins are merged into one `profiler`
  plugin (`src/plugins/profiler`; see Section 10). Non-QML `TimelineModel`
  subclasses already exist (`CtfTimelineModel`, `PerfTimelineModel`).
  `PerfTimelineModel::updateFrames()` diffs consecutive samples' frame arrays,
  opening/closing ranges as frames change.
- A `Sampler` abstraction (`sampler.h`) unifies capture backends: `CallStackSampler`
  (macOS mach sampler), `PerfSampler` (Linux perf), and `QmlProfilerSampler` (QML
  debug protocol, all platforms). `RecordingSession` (`sampler.h`) models one
  target feeding several captures: it carries `pid`, `serverUrl` (QML debug
  channel), `launchCommand`, `stop`, `progress`, `result`, plus the sampling
  interval, requested QML features, and a `markStarted()` monotonic anchor.
  `recordRecipe()` wraps two hooks -- `prepareLaunch()` and `captureRecipe()` --
  so a composite backend can compose captures without re-implementing launch.
  `launchThenCapture()` (`samplerrecipe.cpp`) launches the target once and runs a
  capture recipe in parallel.
- `SampleTraceData` consumers aggregate purely from `frames` + `labels`:
  `CallTreeModel` merges root-first frame paths into a tree with `weight`/`self`;
  `CpuUsageModel` drives the timeline; `SamplerViewManager` wires them onto one
  `TimelineZoomControl` and re-aggregates on selection via
  `CallTreeModel::setTimeRange()`.
- perf acquisition is Linux-only in practice (`perfprofilerruncontrol.cpp` shells
  out to `perf record`). perfparser and qtdeclarative/V4 are **not** in this
  checkout, so Strategy-B producer work lands in other repos.

---

## 5. Strategy A -- concurrent capture and cross-stream splice (implemented)

Because both profilers run on the same process at the same time, we have, for any
timestamp T:

- from the **native sampler**: the C++ call stack S(T), containing a contiguous
  run of engine frames (`QV4::...` / `QtQml`) wherever JS is executing; and
- from the **QML profiler**: the nested range stack R(T) -- the QML ranges
  (`Javascript`, `Binding`, `HandlingSignal`, `Creating`) whose `[start, end)`
  contains T, innermost last. This *is* the JS call stack at T, with exact source
  locations.

**Splice**: in each native sample, replace the engine-frame run with R(T)'s frames
(as JS `Label`s):

```
main  ->  QQmlBinding::evaluate  ->  [onClicked]  ->  [compute()]  ->  MyModel::data  -> ...
   \______ C++ above engine ______/  \____ JS from QML profiler ____/  \_ C++ below engine (sampled) _/
```

This yields exact JS frames (from instrumentation, not from guessing at
interpreter internals) *and* the C++ callees JS reached (from the sampler). It is
possible only because both streams are captured together.

### 5.1 Engine-boundary detection

The splice must know *where* in S(T) to insert R(T):

1. **Marker frames.** Recognise the contiguous run of engine frames by
   module/symbol (`QtQml`, `QV4::`, and the `JITCode:QtQml` marker); replace that
   run with R(T).
2. **No-engine passthrough.** If no engine run is found, leave S(T) untouched --
   there is no JS to splice. Likewise if no QML range is active at T, keep the
   engine frames (engine housekeeping stays visible).
3. **Nested engine runs** (JS -> C++ -> JS) are the tricky case; the QML range
   nesting disambiguates which JS frames belong to which run.

### 5.2 Replace vs interleave

**Replace** the engine machinery frames by default (cleaner for app developers),
with an option to **reveal** the engine frames (interleave) for engine hackers.
Purely how the splicer composes the output `frames` list; no consumer change.

### 5.3 Architecture

- **`CombinedSampler : Sampler`** (`combinedsampler.{h,cpp}`) launches one target
  and records it with a native sampler and the QML profiler in parallel:
  `recordRecipe()` -> `launchThenCapture(session, Group{ parallel, nativeCapture,
  qmlCapture, forwarder })`, run all-and-succeed so a per-side failure surfaces
  its real error. Each sub-capture gets a child `RecordingSession` sharing the
  launched pid and QML channel; the forwarder mirrors stop/progress/`started`.
  `CombinedSamplerSettings` exposes both backends' options (native interval + QML
  feature toggles), carried to the sub-captures on the session. Output: a bundle
  directory holding the sampler CTF2 trace, the QML `.qtd`, and a `manifest.json`
  with the clock offset (Section 7). `isCombinedTrace()` recognises it on load.
- **`mergeQmlIntoSamples()`** (`samplemerge.{h,cpp}`) is a pure function over
  `SampleTraceData` + a flat list of `QmlRange { startUs, endUs, name, file, line,
  parent }` + `MergeOptions { revealEngineFrames, sampleTimeOffsetUs }`. It
  rewrites each sample's `frames` per Sections 5.1/5.2, interning one JS `Label`
  per distinct (name, file, line). Keeping it a pure function makes it unit-testable
  with synthetic inputs and makes the merged result a plain `SampleTraceData` that
  saves and reloads.
- **`CombinedTraceLoader`** (`combinedtraceloader.{h,cpp}`) turns a bundle into one
  merged trace: it loads the `.qtd` into a `QmlProfilerModelManager`, replays the
  events to reconstruct the time-varying QML call stack as `QmlRange`s, applies the
  manifest's clock offset, merges, and writes the result as a fresh CTF2 trace.
- **View.** The qtprofiler viewer offers the combined backend and a
  `Format::Combined` layout that shows the QML profiler views *and* the sampler
  views together (the merged trace is an ordinary `SampleTraceData`, so the sampler
  views need no new code). See Section 8 for colouring.

### 5.4 Implemented / not yet

Implemented: `CombinedSampler`, `mergeQmlIntoSamples()` (+ unit tests),
`CombinedTraceLoader`, the `Format::Combined` view, JS/C++ colouring with a
legend, and the correlation-anchor clock offset (Section 7, approach 2).

Not yet: the exact common-clock (Section 7, approach 1), and an in-Creator
perspective. The same `CombinedSampler` + `mergeQmlIntoSamples()` can later back
one `RunWorkerFactory` for a `COMBINED_PROFILER_RUN_MODE` whose recipe parallelises
`createRecipe(QML_PROFILER_RUNNER)` and the native sampler, feeding a single
`Core::Perspective`.

Limits: only JS the QML profiler instruments is spliced; alignment is only as good
as the clock anchor (Section 7); and hot JS that is JIT-compiled has anonymous
native frames (symbolicators cannot name JIT'd JS), lowering the splice rate --
forcing the V4 interpreter improves it, and Strategy B addresses it properly.

---

## 6. Strategy B -- producer-side reconstruction (future)

Get the JS frames from the producer, so the merge is exact for *all* JS and does
not depend on the QML profiler running.

**What the producer must do**, per raw native stack:

1. **Recognise V4 frames** -- the contiguous run of `QV4::...` (interpreter) or
   JIT'd regions (compiled JS).
2. **Replace or annotate** that run with synthetic JS location ids, emitting
   `LocationDefinition` / `SymbolDefinition` / `StringDefinition` for each new JS
   function the first time it is seen.
3. **Reconstruct the JS call frames** so nested JS calls become multiple frames,
   not one opaque `interpret` blob:
   - *JIT'd JS* -- via perf jitdump (`jit-PID.dump`) / `perf-PID.map`; the easier
     path.
   - *Interpreted JS* -- the hard path: walk the V4 JS call frames out of the
     interpreter state (the knowledge the native-mixed dumpers already encode).

This work lives in perfparser (symbolication, jitdump ingestion) and qtdeclarative
(jitdump emission, interpreter frame exposure) -- neither in this checkout. The
contract this document fixes ("JS frames are source-bearing locations with a
`kind`") is what lets that proceed independently.

---

## 7. Clock correlation

The QML debug-protocol timeline and the sampler clock differ. The timeline model's
absolute base (`TimelineTraceManager::traceStart/traceEnd`) is fine; the work is
aligning the two *sources*.

1. **Common monotonic clock (preferred, future).** Have both timestamp against the
   same base -- macOS `mach_absolute_time`, elsewhere `CLOCK_MONOTONIC` (perf
   supports `-k CLOCK_MONOTONIC`). The sampler already reads a host monotonic
   clock; the QML engine timestamps would need to be convertible to that base --
   the one qtdeclarative-side change worth making. Makes the splice exact.
2. **Correlation anchor (implemented fallback).** Stamp a process-wide
   `steady_clock` instant when each capture goes live (`RecordingSession::
   markStarted()`), compute the fixed offset, and store it in the bundle manifest;
   the loader applies it before the range lookup. Residual skew remains because the
   go-live instants lag each trace's true zero (the QML debug connect especially);
   dense JS coverage in the target keeps the splice populated despite it.

The manifest carries the offset so the merge is reproducible on reload. If
alignment is too coarse for per-sample splicing, the sampler and QML views still
correlate on one shared `TimelineZoomControl` with selection-driven
re-aggregation, which is clock-tolerant.

---

## 8. Consumer-side changes and colouring

- **Model:** none required to *display* merged stacks -- JS frames arrive as
  ordinary frames.
- **Colour JS vs C++.** Strategy A derives the kind from the label (a `.qml`/`.js`
  `file` with no `module` is JS) and tints JS frames with the accent text colour
  in the call tree and its heaviest-stack pane, with a fixed-height JS/C++ legend
  shown only for merged traces. Strategy B would add a `kind` field to `Symbol`
  (one `qint32`, versions cleanly) -- or use a `"[QML]"`/`"[JS]"` `Symbol.binary`
  convention to avoid a wire-format change -- and map it to the same colour role.
- **Statistics / call tree:** optionally a "group by language" or JS/C++ filter.
- **Details popup:** show the source location (already carried).

---

## 9. Cross-platform plan

The Creator/UI/model layer is platform-neutral and the QML-protocol transport is
cross-platform. Strategy A is already cross-platform through the `Sampler`
abstraction (mach sampler on macOS, perf on Linux). Strategy B decomposes the
native side into two independently-portable layers:

- **Layer 1 -- raw sampling (per-OS):** Linux perf (have it); Windows ETW or
  in-proc `StackWalk64`/DbgHelp; macOS `xctrace`/Instruments, `sample`/`spindump`,
  or an own `thread_get_state` sampler (needs entitlements).
- **Layer 2 -- symbolication (per-object-format, the underrated cost):** ELF/DWARF
  (perfparser); PE/PDB and Mach-O/dSYM are real new backends.

V4 frame reconstruction is OS-independent (it reads the engine), written once
behind the interchange. For an OS-neutral interchange, either generalize the
perfparser binary protocol into a "stack sample" protocol or use Chrome Trace
Format `sample`/`stackFrames` records (the current `ctfloader` only handles
duration phases, so sampled records need loader work). Pragmatically, Windows/macOS
arrive via file import before live capture.

---

## 10. Plugin structure

The QML profiler plugin was generalised to `profiler` (`src/plugins/profiler`,
namespace `Profiler`) and `perfprofiler` folded into it, so the perf and QML
profilers share one tool/perspective/trace clock. A single fused perspective needs
one tool owning both streams on one `TimelineZoomControl` and one trace clock;
separate plugins/perspectives cannot share zoom/selection cleanly. perfparser is an
external process, so the merge did not pull elfutils into Creator itself. (Some
sampler files remain in the older `QmlProfiler::Internal` namespace; unifying that
is outstanding.)

---

## 11. Milestones / status

Strategy A (done, in the qtprofiler viewer):

1. **CombinedSampler** -- one target, two parallel captures, a bundle (sampler
   CTF2 + QML `.qtd` + manifest).
2. **Clock anchor** -- the correlation offset in the manifest (Section 7/2).
3. **mergeQmlIntoSamples** -- the splicer + boundary detection, unit-tested.
4. **Combined view** -- QML + sampler views together; JS/C++ colouring + legend.

Future:

5. **Common monotonic clock** -- the qtdeclarative-side change for exact
   per-sample alignment; converges with Strategy B's producer track.
6. **In-Creator perspective** -- `COMBINED_PROFILER_RUN_MODE` + a fused perspective.
7. **Strategy B** -- perfparser jitdump ingestion + V4 frame reconstruction;
   qtdeclarative emits jitdump.
8. **Cross-platform Strategy B** -- the OS-neutral interchange; Windows/macOS via
   file import first.

---

## 12. Testing

- **Unit: the splicer.** `mergeQmlIntoSamples()` against synthetic inputs -- a
  hand-built `SampleTraceData` (with an engine-frame run) plus a hand-built QML
  range list -- asserting the merged `frames`/`labels` for replace and interleave
  modes, the no-JS passthrough, label interning, and the clock offset. Lives in
  `tests/auto/qtprofiler/tst_samplemerge`.
- **Integration:** a combined capture of a small QML+C++ app
  (`tests/manual/sampler-testapp`), asserting a known JS->C++ path appears merged.
- **Clock:** a fixed synthetic offset in the merge test locks alignment behaviour
  independent of live capture.

---

## 13. Open questions / risks

- **Clock skew** (Section 7) is the dominant risk for exact splicing; the shared
  zoom/selection fallback keeps the feature useful even if alignment stays coarse.
- **Engine-boundary detection** across platforms and Qt versions -- the
  `QtQml`/`QV4::` marker set must be validated; nested JS<->C++ runs are tricky.
- **JIT'd JS** has anonymous native frames; forcing the interpreter is a workaround,
  Strategy B the real fix.
- **Overhead** of running the debug protocol *and* sampling on one process -- read
  the merged view as attribution, not absolute cost.
- **Interpreter reconstruction** (Strategy B) is the highest-risk producer item;
  jitdump-only (JIT'd code) is a viable reduced-scope first cut.
- **`kind` field vs convention / `SampleTraceData` extension** -- deriving the kind
  from `file`/`module` avoids a format bump; an explicit field is cleaner.

---

## Appendix: key code references

| What | Where |
| --- | --- |
| Generic timeline model + `Range` | `src/libs/tracing/timelinemodel.h`, `timelinemodel_p.h` |
| Perfect-nesting requirement | `TimelineModel::computeNesting()` |
| Trace clock | `TimelineTraceManager` (`traceStart`/`traceEnd`) |
| Flame-graph widget (any QAbstractItemModel) | `src/libs/tracing/flamegraphwidget.h` |
| Sampler interface + `RecordingSession` (+ `markStarted`) | `src/plugins/profiler/sampler.h` |
| Launch-then-capture composition | `src/plugins/profiler/samplerrecipe.cpp` |
| Combined backend + bundle/manifest | `src/plugins/profiler/combinedsampler.{h,cpp}` |
| QML->native splicer (+ options) | `src/plugins/profiler/samplemerge.{h,cpp}` |
| Bundle load + merge + write | `src/plugins/profiler/combinedtraceloader.{h,cpp}` |
| Native sample model (`ThreadSample`, `Label`) | `src/plugins/profiler/sampletrace.h` |
| Merged call tree + JS/C++ colouring | `src/plugins/profiler/calltreemodel.{h,cpp}`, `calltreeview.{h,cpp}` |
| Sampler views + selection re-aggregation | `src/plugins/profiler/samplerviewmanager.{h,cpp}` |
| Multi-backend viewer + combined layout | `src/tools/qtprofiler/qtprofilerwindow.cpp` |
| QML range model (source locations, nesting) | `src/plugins/profiler/qmlprofilermodelmanager.h`, `qmlprofilertimelinemodel.*` |
| Perf wire `Location`/`Symbol` (Strategy B) | `perfeventtype.h`, `perfprofilertracemanager.h` |
| Perf sample shape + stack diffing | `perfevent.h`, `perftimelinemodel.cpp` (`updateFrames()`) |
| perf acquisition (Linux-only) | `perfprofilerruncontrol.cpp` |
| CTF / Chrome-JSON loader | `ctfloader.cpp` |
