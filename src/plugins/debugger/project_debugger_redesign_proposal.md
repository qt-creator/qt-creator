# Debugger: decouple backends from DebuggerEngine via a minimal DebuggerEngineInterface

## Motivation

`DebuggerEngine` has 62 virtual methods (43 `public`, only 9 pure). Its 7
subclasses each override a different, non-uniform subset, so shared-base
changes never affect all of them the same way. The 8 handler classes hold a
raw `DebuggerEngine *` with unrestricted access. The engine also aggregates
GUI concerns (actions, docks, tooltips) directly, so it can't be constructed
without a full Creator process (a bare `GdbEngine` crashes today - its ctor
touches `Core::ActionManager`).

**Goal:** construct and drive a debugger backend without any GUI attached, so
backends become unit-testable.

## The `Process` / `ProcessInterface` analogy

`Utils::Process` already solves this for subprocesses: `Process` (rich,
concrete) owns a `ProcessInterface *` (3 pure virtuals, `private` + `friend`,
no public getters); results flow back via signals, config in via one
protected `ProcessSetupData`.

- **`DebuggerEngine`** = the `Process` role. Owns handlers, actions, docks,
  the state machine. Target end state: `final`, no longer subclassed.
- **`DebuggerEngineInterface`** = the `ProcessInterface` role. Narrow,
  mostly-pure-virtual, implemented once per backend and composed in.

`ProcessInterface` itself took **54 commits over ~4 years** to shrink from 21
methods to 3. Expect the same here; treat the current interface as a first
cut, not a target.

## Design rules that have earned their keep

- **Enum-dispatched commands, not new virtuals.** `execute(ExecutionRequest)`,
  `refresh(RefreshRequest)`, `changeBreakpoint(...)`. When something new needs
  a command/response round trip, add an enum value - `RefreshKind::FullBacktrace`
  is the precedent - not a method. 16 virtuals today; the four that could still
  collapse (`fetchDisassembly`/`createSnapshot`/`watchPoint`/`accessMemory`)
  deliberately don't, because their replies are typed and `GdbMi` would
  degrade them.
- **No GUI types, no generic `QVariant` payloads**, and responses carry real
  data rather than a bool.
- **Command virtuals only enqueue** and return - never mutate fragile
  single-shot state synchronously. So a `DebuggerEngine` slot reacting to a
  signal may call straight back in, reentrantly.
- **Capabilities are data, per start mode.** `DebuggerEngineSetupData::capabilities`
  plus `attachToCoreCapabilities`, which is a full *override* (not a subset)
  for `AttachToCore`, because a core file has no live process - and a
  capability could equally be post-mortem-only. Composed from a local
  `coreCaps` so no capability is named twice.

## Status: five backends

| Engine | Impl | Coverage |
|---|---|---|
| Gdb | `GdbImpl` | Launch, attach-to-process, core files, remote (plain + extended), native-mixed, Windows breadth. `target qnx` implemented but never run against a real QNX agent. |
| Lldb | `LldbImpl` | Full parity with `LldbEngine`. The two remote-attach modes still skipped are an lldb limitation below version 21 (`PlatformRemoteGDBServer::MakeUrl()`, llvm/llvm-project#142875) that real `LldbEngine` shares - both drive the same `lldbbridge.py` `setupInferior()` - so the tests are gated on the debugger's version, not on the backend. |
| Pdb | `PdbImpl` | Launch-only, and deliberately does *more* than `PdbEngine` (see parity below). |
| Qml | `QmlImpl` | Attach-only over TCP, V8-debugger JSON. Breakpoints (incl. stopping on an uncaught JS throw, and in-place enable/disable via the negotiated `changebreakpoint`), stepping, `RunToLine`, full stack, locals + watchers (incl. recursive container expansion), the Inspector object tree, value assignment, source files, detach. |
| Cdb | `CdbImpl` | Drafted against real `cdbengine.cpp`; disassembly and the initial-session setup verified live, the rest still being worked through against a real `cdb.exe` off-tree. |
| Uvsc | — | Not started. |

Still opt-in and gdb-only in production: `QTC_USE_GENERIC_DEBUGGER` swaps
only `GdbEngine`. The other Impls exist for the test harness so far.

## Progress estimate

Weighting each engine by its own size (LOC as an effort proxy) against
functional completeness:

Measured against *what the interface asks of each backend*, not against the old
engine method's line count: `GenericDebuggerEngine` absorbed a lot of that work
once, for everyone, so sizing a gap by the engine's own method body double-counts
it. Every figure below now comes from the same two-step audit - the enumerable
axes (capabilities, `RefreshKind`s, `ExecutionCommand`s, start modes, outbound
signals, async records, test rows) *plus* a method-by-method diff against the
engine being ported. That second step matters: it moved `Gdb` and `Qml` down by
5-7 points each, because features hide in private helpers
(`fetchDisassemblerByCli*`, the output collector, `constructLogItemTree`,
`memorizeRefs`, `handleVersion`) that no axis lists. `Lldb` and `Pdb` survived it
unchanged.

| Engine | Weight | Functional | Adds to total |
|---|---|---|---|
| Gdb | 5641 | ~93% | 35.9 |
| Cdb | 3340 | ~48% in-tree (~65% reported off-tree) | 11.0 |
| Qml | 2577 | ~88% | 15.5 |
| Lldb | 1390 | ~100% | 9.5 |
| Uvsc | 962 | 0% | 0 |
| Pdb | 702 | ~100% | 4.8 |

**≈77% of the per-engine porting; ≈61-65% of the whole migration** - or ≈81% and
≈64-68% if `Cdb`'s off-tree figure is used instead. The gap between per-engine and
whole-migration is the work no per-engine number captures: flipping
`QTC_USE_GENERIC_DEBUGGER` on by default, then making `DebuggerEngine` `final`
and deleting the old virtual surface.

`Cdb` is given twice on purpose. The in-tree number is what this stack's
`CdbImpl` does and is auditable here; the higher one is reported from separate
ongoing work against a real `cdb.exe` and cannot be checked from this tree. At
22.9% of the weight it is also the single largest uncertainty in the total.

LOC is a weak proxy - `lldbimpl`/`pdbimpl` are *larger* than the engines they
replace (heavy comments, no inherited machinery), so raw ratios overstate
progress. What genuinely carries over per engine is the interface shape plus
`GenericDebuggerEngine`'s model glue; the wire protocol starts from zero every
time, which is why Cdb and Uvsc are the expensive ones left.

## Capability parity

No Impl under-claims relative to the engine it ports:

- `GdbImpl` matches (27, split core/non-core), `QmlImpl` matches exactly (4).
- `LldbImpl` adds `ResetInferior` (`LldbEngine` has no `executeReset()`) and
  under-claims nothing: `ReverseSteppingCapability` and `SnapshotCapability` are
  commented out in `LldbEngine` too (`//| ReverseSteppingCapability`,
  `//return cap == SnapshotCapability;`), and `AddWatcherWhileRunningCapability`
  is claimed only by `GdbEngine`/`QmlEngine`. `execute(RecordReverse)` is
  unported to match - `lldbbridge.py` has no reverse-execution support at all.
- `PdbImpl` adds `JumpToLine`/`RunToLine`/`ReloadModuleSymbols`/`ResetInferior`.
  The first three exist in `PdbEngine` but were never claimed, so the feature
  was unreachable from the UI.

Every over-claim has a real body *and* a passing dedicated test. Note the
converse hazard, hit for real: a *claimed* capability can still have no
plumbing - `CreateFullBacktraceCapability` was advertised by `GdbImpl`/`LldbImpl`
while `GenericDebuggerEngine` connected nothing, so the menu item did nothing.

## Remaining work, in cost order

1. **Uvsc** - from zero, vendor-specific protocol.
2. **CdbImpl** - verify against a real `cdb.exe`. No longer blocked on the test
   harness: `initTestCase()` resolves MSVC itself (`cl.exe` via
   `QTC_MSVC_ENV_BAT`, dropping the `Backend::Cdb` row only when absent) and the
   generated inferior has `_WIN32` branches, so the row does run on Windows.
3. **QmlImpl: the Inspect mode's GUI half** - the `QmlInspectorTool` service
   behind the select/show-app-on-top actions, plus jump-to-definition. Needs
   `Core`, the editor and a `QQuickWindow` app, so no headless test can reach
   it. Its protocol half *is* ported: `RefreshKind::InspectorTree` carries the
   live object tree (engines, root contexts, per-object fetch on expansion,
   property watches), and both `executeDebuggerCommand()` and
   `assignValueInDebugger()` evaluate against an Inspector object while the
   inferior runs. (Container expansion likewise, at any depth:
   `RefreshRequest::expandedINames` carries the view's expanded set, so a
   container is reported expandable up front and each level's members are
   fetched only once that level is expanded, matching the
   `handleScope()`/`insertSubItems()` `isExpandedIName()` gating.)
4. **QmlImpl: script-URL resolution** - the `toFileInProject` gap below. Small,
   but the only open QML defect a user would actually hit.
5. **Flip the default**, then delete `DebuggerEngine`'s virtual surface.
6. **Cross-cutting gaps**: no command-timeout watchdog anywhere in the new
   stack; configurable debuginfod not wired into the Impls.

## Known open defects

- **Failure-path `InferiorEvent`s never emitted**: only `StopFailed`, and only in
  `GdbImpl` (provoking it needs a command timeout, i.e. the watchdog below) -
  `LldbImpl` reports it from lldbbridge.py's own "inferiorstopfailed"
  state, mirroring `LldbEngine`. Two others are *correctly*
  never emitted by a backend and should be left alone:
  `EngineSpontaneousShutdown` is derived by
  `DebuggerEngine::notifyDebuggerProcessFinished()` from engine state (the
  debugger process dying while `InferiorRunOk`), which both Impls already feed
  via `engineProcessFinished()`; and `EngineIll`, whose only source in a real
  engine is `LldbEngine::continueInferior()`'s `ResultFail` branch, is
  deliberately reported as `InferiorIll`/`RunFailed` instead - a failed continue
  means the target cannot run, not that the debugger is broken, and it is what
  `GdbImpl` does.
- **`GdbImpl`'s remaining gaps**, from the method-level audit:
  - the **user's gdb init script** (`loadInitScript`) is never loaded, and the
    **disassembly flavour** (`intelFlavor()`, "set disassembly-flavor
    intel/att") is never sent, so both settings are silently ignored. Each needs
    a field in the start data to carry a preference the backend cannot read
    itself - the same shape, best done together.
  - the **copy-dumpers-to-target path** of `startGdb()`, for remote targets
    needing helpers on the far side.
  - the **output collector** (`--tty=`, `readDebuggeeOutput`), used by
    `GdbEngine` for every plain local run. Inferior output is *not* lost without
    it - `GdbImpl` surfaces gdb's `@` console records - so this is a routing
    difference, not missing functionality.
  - remaining MI async records. Of `GdbEngine`'s 18 classes `GdbImpl` now handles
    seven; of the rest, four have empty bodies in `GdbEngine` itself and five
    only produce status-bar text. `breakpoint-created`/`-deleted` are done (see
    `breakpointEvent()`'s requestId 0), and `*running` needs nothing here: run
    state comes from the issuing command's own callback, so the intermediate
    notifications real code filters out are never produced.
- **`QmlImpl`'s remaining gaps**, likewise:
  - no **script-URL to project-file mapping** (`toFileInProject`, 3 call sites in
    `qmlengine.cpp`). A v8 script name is a resource URL (`qrc:/main.qml`), and
    it reaches the view unresolved: a stack frame's `file` carries the raw name
    and `locationChanged()` reduces it to a bare basename, so jumping to a QML
    frame and the current-line marker cannot find the file. The one gap here
    that is a user-visible defect rather than a missing extra, and the only one
    needing an engine-side decision - the mapping wants project access like
    `cleanupFullName()`, but a `qrc:` URL is not a filesystem path, so it cannot
    just reuse that path. Not covered by the tests, which assert on the stack's
    contents, never on file resolution.
  - the v8 **`version` handshake** *is* done, and `ChangeBreakpoint` is used:
    enabling/disabling an existing breakpoint goes through v8's own
    `changebreakpoint` instead of a clear + re-set, so the number a caller holds
    survives (see `togglesBreakpointEnabledInPlace()`). Still unused, though
    negotiated: `UnpausedEvaluate`/`ContextEvaluate`, so console evaluation while
    running never takes v8's own route, and there is no `updateCurrentContext`
    equivalent re-evaluating as the frame/object selection changes.
  - no **refs cache** (`memorizeRefs`/`refVals`), while the connect handshake
    does ask for `redundantRefs: false`. Explicit lookup-by-handle covers it in
    practice; it is the fragile half of a two-part mechanism.
  - no **console object tree** (`constructLogItemTree`, 40 references in
    `qmlengine.cpp`): evaluated objects print as flat text instead of an
    expandable tree. The largest single omission by code volume.
  - no **`updateScriptSource`**, so debugger-provided QML source is never pushed
    into a document - files not on disk cannot be shown.
- **`CdbImpl` (in-tree) is the least complete**: 7 of 14 applicable
  `RefreshKind`s (no `Threads`, `SourceFiles`, `ModuleSymbols`, `ModuleSections`,
  `StackSymbols`, `PeripheralRegisters`, `QmlStack`) and 1 of 6 start modes
  (`Launch` only). Absent against `CdbEngine`: stack-trace parsing, symbol
  resolution, breakpoint line correction, source-path-map merging, script
  messages, expression evaluation. Two capabilities look over-claimed against
  that - `AdditionalQmlStackCapability` without `RefreshKind::QmlStack`, and
  `TracePointCapability` with no tracepoint plumbing - the same hazard
  `CreateFullBacktraceCapability` hit before.
- **Remote attach needs lldb >= 21**, in `LldbImpl` and real `LldbEngine` alike:
  the `gdb-remote` route hangs against a bare `gdbserver --multi` and the
  platform route fails attach on an empty hostname
  (llvm/llvm-project#142875). Not a porting gap - the two tests skip on the
  debugger's own version now, so they start running by themselves once lldb 21
  is in use.
- **SIGINT/ptrace race on interrupt** - confirmed to affect real `GdbEngine`
  identically, so pre-existing, not a port regression. Closing it means either
  blocking the UI on every interrupt or a re-entrancy-risky nested event loop;
  deferred deliberately. A deferred `NeedsTemporaryStop` command whose stop
  never arrives now fails its callback instead of being dropped forever.
- **macOS can't disable PIE**, so `symbolAddress()`'s `nm`-derived addresses
  don't match runtime there. `-no-pie`/`-ldl` are Linux-only now (passing them
  on macOS broke the clang bot outright), but the `symbolAddress()`-based tests
  are expected to misbehave on macOS rather than skip cleanly.

## Migration strategy (strangler fig)

1. Keep narrowing the interface in small, revertible steps.
2. Build a `RefreshCoordinator`; make `updateAll()`/`updateLocals()` delegate
   to it - fixes the known `WatchHandler` concurrency bug centrally.
3. Port each backend against the real engine's logic rather than reusing it
   (can't subclass: both are `QObject`s).
4. Flip the default per engine once its path is a real alternative.
   **Milestone reached for Gdb:** the real IDE with `QTC_USE_GENERIC_DEBUGGER=1`
   launches, breaks and stops end-to-end. That took three bugs invisible to the
   direct-driving harness: missing `"-i mi"`, missing `claimInitialBreakpoints()`,
   missing environment/working-directory propagation. Locals/stack/watch views,
   memory/disassembly and richer breakpoint types are still only confirmed via
   `tst_backends.cpp`, not live.
5. Only then make `DebuggerEngine` `final` and delete the old virtual surface.

## Relation to BridgeEngine

`BridgeEngine` (`bridge/`, opt-in via `QTC_DEBUGGER_USE_BRIDGE`) speaks a
DAP-shaped JSON protocol to `bridge.py`, a server hosted inside gdb's own
Python. It abstracts a different axis than this series does: the *wire*, where
this abstracts the *engine boundary*. They are not alternatives to each other,
and the choice of which is load-bearing decides what the backends look like.

| | `BridgeEngine` | this series |
|---|---|---|
| what is written once | the protocol | the engine |
| per-debugger code | Python, inside the debugger | C++, behind `DebuggerEngineInterface` |
| engine-side surface | all 43 `DebuggerEngine` virtuals | 16 interface methods |
| capabilities claimed | 6 | 29 (`GdbEngine`: 28) |
| headless tests | none | `tst_backends.cpp` |
| async pause while running | not supported yet | supported |

Two observations about the bridge, from its own history rather than from
preference:

- Its data plane already retreated from DAP. `9c7366c63df` replaced
  `variablesReference` with a native `qtc/fetchVariables` driving the real
  dumpers and reusing `updateLocalsView()` - "the same path GdbEngine uses".
  DAP cannot express iname identity, lazy expansion or watchers.
- "Written once" covers the wire and the C++ side only: `bridge.py` is
  `import gdb`/`gdb.execute()`, so each debugger still needs its own bridge,
  exactly as `lldbbridge.py` and `cdbbridge.py` do today.

**Target end state: one `BridgeImpl` behind this interface, for the four
debugger processes.** `GdbImpl`, `LldbImpl`, `PdbImpl` and `CdbImpl` are then
deleted rather than shrunk - a single translator maps interface calls to
protocol requests and protocol events back to `InferiorEvent`, while the
debugger-specific work (MI text parsing, console scraping for `tbreak` numbers,
version quirks) lives in each debugger's Python bridge.

`QmlImpl` stays. It is the one backend with no debugger process to host a
bridge: the backend *is* the debuggee, reached over TCP through
`QmlDebugConnection` with the V8/V4 protocol, so a DAP-to-V4 translator would
have to live on this side anyway - which is `QmlImpl` under another name. That
the interface accommodates a backend with no bridge at all is itself an argument
for it being the load-bearing boundary.

`GenericDebuggerEngine` and `tst_backends.cpp` are unaffected either way, which
is what makes the transition checkable: build `BridgeImpl` against the same
interface, run the same test rows against both, and delete `GdbImpl` when they
match. `GdbEngine`, `BridgeEngine` as a standalone engine, and
`DapEngine`-for-gdb all retire; `DapEngine` keeps its real job, foreign DAP
adapters.

Cost of that path, not yet paid: three more bridges (lldb, cdb, pdb) and async
support, which `bridge.py` does not have.

## Testing strategy

- **Real-thing-first.** Every behavioural claim is checked against a real
  gdb/lldb/python3/QML session, not read off the old engine. That discipline is
  what caught the bugs that mattered - silently dropped MI commands, a
  malformed memory write, dumper-load ordering races, `target-async` breaking
  `gdbbridge.py`'s own resolver continue.
- **`tst_backends.cpp` drives the Impls directly**, bypassing
  `GenericDebuggerEngine`, data-driven over a `Backend` enum. It must **never
  branch on backend identity** - differences belong in the Impls (covered by
  the interface) or in `InferiorTestData`. `stopAtBreakpoint()` picks
  launch-vs-attach from declared start modes so shared bodies need no branch.
- **Every capability gets its own isolated `testXxxCapability()`**, even where
  that duplicates existing coverage. Two remain untested: `BreakModule` (needs
  a working CdbImpl) and `AddWatcherWhileRunning` for remote mode.
- **Inferior line numbers come from marker comments**, never hardcoded - the
  checked-in `.qml` inferiors are scanned via `BACKENDS_TEST_SOURCE_DIR`.
  Hardcoding them was a standing trap: adding a copyright header shifted every
  one.
- **Timeouts:** the suite sets ctest `TIMEOUT 0`, so a hang is bounded only by
  qtestlib's own per-function watchdog (5 min default, `QTEST_FUNCTION_TIMEOUT`),
  which names the function and prints a backtrace. A whole-run overrun instead
  shows up as CI killing the step with no test named.
- **`BreakHandler` has zero test coverage** anywhere - unrelated to this port,
  but it is what chains `acceptsBreakpoint()` to an actual insert.

## Explicit non-goals / scope guardrails

- Not a rewrite - each step ships and reverts independently.
- A second non-trivial correctness fix in a row during any step = stop and
  reassess, not "keep patching" (this already happened once, reverted).
- No fixed target method count or deadline - `ProcessInterface` took 4 years.
