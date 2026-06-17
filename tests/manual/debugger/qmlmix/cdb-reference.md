# CDB native-mixed: architecture, protocol, and build-out reference

Companion to `cdb-experiment.md`. Read that first for the go/no-go
experiment. This file is the deeper context — useful if you are driving the
experiment with Claude Code or, once it passes, building the real CDB support.

All paths are in the qt-creator repo unless noted; Qt paths are in qtdeclarative.

---

## 1. How native mixed works (GDB / LLDB today)

A single native debugger session debugs the C++ inferior *and* the QML
interpreter. The QML side is reached through the in-process
**`qmldbg_native`** plugin, which exposes the **"NativeQmlDebugger"** service.
The debugger drives that service purely by **calling functions in the
inferior** and reading a reply buffer — no sockets, no second process.

Flow at a glance:

1. App launched with `-qmljsdebugger=native,services:NativeQmlDebugger`
   (and `QV4_FORCE_INTERPRETER=1` so JS stays introspectable).
2. Debugger breaks at `qt_qmlDebugConnectorOpen` (startup) and calls
   `qt_qmlDebugEnableService("NativeQmlDebugger")`.
3. To set a QML breakpoint it sends a `setbreakpoint` request.
4. When QML hits it, the plugin calls `qt_qmlDebugMessageAvailable()` — the
   debugger has a breakpoint there, so it stops; that is the "QML stop".
5. At any stop the debugger sends `backtrace` and reads the QML frames, and
   issues `stepin`/`stepover`/`stepout`/`continue` to step within QML.
6. QML->C++ step-in: the qtdeclarative hook `qt_v4AboutToCallNativeMethodHook`
   (Qt >= 6.12) lets the debugger break just before a JS->C++ dispatch and
   descend into the C++ method. C++->QML uses "machinery skips" + an armed
   interpreter step.

The experiment only exercises steps 1-5's *service round-trip*. Steps 6 (the
hook + skips) are the harder, later work.

---

## 2. The service protocol (what the calls are)

Source of truth: `share/qtcreator/debugger/dumper.py`
(`sendInterpreterRequest`, `fetchInterpreterResult`) and the Qt side
`qtdeclarative/src/plugins/qmltooling/qmldbg_native/qqmlnativedebugconnector.cpp`.

Request: build `{"command": <cmd>, "arguments": <argsdict>}`, hex-encode the
UTF-8 bytes, pass as `hexData`:

```
qt_qmlDebugSendDataToService("NativeQmlDebugger", "<hex>")
```

Reply: read globals `qt_qmlDebugMessageBuffer` (a `const char *`) and
`qt_qmlDebugMessageLength` (`int`). The buffer is one or more
`"<service> <len> <payload>"` records; the `NativeQmlDebugger` payload is JSON.
Then call `qt_qmlDebugClearBuffer()`.

Commands the bridge uses: `setbreakpoint`, `removebreakpoint`, `backtrace`,
`stepin`, `stepover`, `stepout`, `continue`. (`backtrace` is synchronous —
fills the buffer and returns. A hit breakpoint is asynchronous — calls
`qt_qmlDebugMessageAvailable`.)

Hex encoding is plain: `json.dumps(...).encode().hex()`.

The `extern "C"` entry points (unmangled, in the `qmldbg_native` module) are
listed in the experiment doc.

---

## 3. Where the code lives

Shared base (engine-agnostic):
- `share/qtcreator/debugger/dumper.py`
  - `sendInterpreterRequest`, `fetchInterpreterResult`,
    `parseAndEvaluateAllowingCalls`, `insertInterpreterBreakpoint`,
    `resolvePendingInterpreterBreakpoint`
  - stepping entry points: `executeStep`, `executeNext`, `executeStepOut`,
    `executeNativeMixedStepOut`, `armInterpreterStepIn`, `disarmInterpreterStep`
  - stack helper: `isInterpreterMachineryFrame`, `extractInterpreterStack`
  - no-op hooks overridden per engine: `setupMachinerySkips`,
    `armNativeCallStepIn`, `disarmNativeCallStepIn`, `atNativeToQmlBoundary`,
    `doFinish`

GDB impl: `share/qtcreator/debugger/gdbbridge.py`
  - `interpreterStopHandler` (the central `gdb.events.stop` handler),
    `InterpreterMessageBreakpoint`, `NativeMethodCallBreakpoint`,
    `setupMachinerySkips` (`skip -rfu`), `handleNativeCallHook`,
    `atNativeToQmlBoundary`, `isQmlCallMachineryFrame`, `fetchStack` (splice +
    metacall dimming)

LLDB impl: `share/qtcreator/debugger/lldbbridge.py`
  - `loop`/`handleEvent` (event pump), `handleNativeMethodStepInto`
    (synchronous descent) + `waitForNativeStop`, `executeStep`/`executeNext`/
    `executeStepOut`, `setupMachinerySkips` (`step-avoid-regexp`),
    `atNativeToQmlBoundary`, `fetchStack` (splice + dimming)

CDB (what exists / what you extend):
- `share/qtcreator/debugger/cdbbridge.py` — `parseAndEvaluate`,
  `nativeParseAndEvaluate`, `callHelper`; uses `cdbext.call(...)`.
- `src/libs/qtcreatorcdbext/` — the C++ extension that hosts the dumper Python.
  `extensioncontext.cpp` `ExtensionContext::call` wraps cdb's `.call`;
  `pycdbextmodule.cpp` exposes `cdbext.call(...)` to Python.
- `src/plugins/debugger/cdb/cdbengine.cpp` — the C++ engine that drives cdb.

Reference orchestration (gdb + lldb, end to end, with the same checks you are
proving): `tests/auto/debugger/nativemixed_driver.py` and the autotest
`tests/auto/debugger/tst_nativemixed.cpp`. The fixture is
`tests/manual/debugger/qmlmix`.

The hook: `qtdeclarative/src/qml/jsruntime/qv4qobjectwrapper.cpp`
(`qt_v4AboutToCallNativeMethodHook`, `qt_v4NativeCallHookEnabled`).

---

## 4. Automating the experiment via `cdbext.call` (closer to the real path)

The raw-`.call` stages A-C answer the primitive question. A second, more
representative check is to drive the same round-trip through the dumper's own
call path, which is what a real bridge would use:

- Load the extension and import the bridge the way `tst_dumpers.cpp` does
  (search it for `qtcreatorcdbext` and `!qtcreatorcdbext.script`).
- From `!qtcreatorcdbext.script`, try:
  ```python
  import cdbext
  cdbext.call('<MOD>!qt_qmlDebugEnableService("NativeQmlDebugger")')
  cdbext.call('<MOD>!qt_qmlDebugSendDataToService("NativeQmlDebugger","<hex>")')
  # then read qt_qmlDebugMessageBuffer / qt_qmlDebugMessageLength via cdbext
  ```
- If `cdbext.call` handles the string args and the buffer read works, the
  bridge's `sendInterpreterRequest`/`fetchInterpreterResult` can be given a
  CDB path with little friction.

If Claude Code is available on the box, hand it this file plus
`nativemixed_driver.py` and `cdbbridge.py`; ask it to add a temporary
`nativemixed` scenario that runs inside `!qtcreatorcdbext.script` and reports
the backtrace. That is the cleanest way to get a representative result.

---

## 5. Building CDB support (experiment passed — green light)

The feasibility experiment came back **GO** (Stage C): the full service
round-trip is drivable from cdb via inferior `.call`, confirmed with a real
QML backtrace. Two findings from it are now load-bearing for the build-out:

- **String literals are rejected by cdb `.call`** (`String literals not
  allowed`). Every string argument (service name, hex payload) **must be
  marshalled into target memory and passed by pointer.** Not optional.
- **There is no `.call`-able allocator** (`malloc` has no usable PDB
  prototype). The experiment used the `.dvalloc`/`eb` commands; a bridge needs
  the extension to allocate + write target memory.

In rough order, and with the honest caveats:

0. **(Done, shared) The call seam.** `dumper.py` now routes the service
   inferior calls through `callServiceFunction(function, args)` and
   `readServiceVariable(name)`. GDB/LLDB inline string literals; **CDB overrides
   both** (`cdbbridge.py`, drafted but UNTESTED) to module-qualify the symbol
   (`qmldbg_native[d]!...`) and pass marshalled pointers.

1. **Service round-trip — DONE and VALIDATED on Windows.** The two missing
   extension primitives are added: `cdbext.writeRawMemory(addr, bytes)`
   (mirrors `readRawMemory` via `IDebugDataSpaces::WriteVirtual`) and
   `cdbext.allocate(size)` (`ExtensionContext::allocateMemory` runs `.dvalloc`
   and parses the address). `cdbbridge.py` implements `marshalString()`
   (allocate -> write bytes+NUL -> return address) and `callServiceFunction`/
   `readServiceVariable` against them; `fetchInterpreterResult` reads
   `qt_qmlDebugMessageBuffer`/`Length` + `readRawMemory`, hex-decodes, clears.
   Verified end to end with `cdb-roundtrip.cdb` (June 2026): enable service ->
   setbreakpoint -> backtrace returns a live QML frame. The draft needed no
   code changes; the `(char *)` cast, the `.dvalloc` parse and `cdbext.call`
   return handling all worked as written.

2. **Interpreter breakpoints + the "QML stop".** Implement the equivalents of
   gdb's resolver / `InterpreterMessageBreakpoint`: a breakpoint on
   `qt_qmlDebugConnectorOpen` that enables the service and resolves pending QML
   breakpoints, and one on `qt_qmlDebugMessageAvailable` whose hit means "a QML
   event — read the message and maybe stay stopped."

   **Caveat (orchestration model):** gdb hangs this off `gdb.events.stop` and
   lldb off its event loop. CDB has no in-debugger Python event loop — stops
   are reported to the C++ `CdbEngine`. So this wiring lives in `cdbengine.cpp`.
   Confirmed hook points (June 2026 walk-through): set the internal breakpoints
   on the two symbols via cdb (`bu`), then in `CdbEngine::examineStopReason`
   (cdbengine.cpp ~1629) / `processStop` (~578) recognize a stop at them
   **by function/address** — they are not `breakHandler` breakpoints, so the
   existing `reason == "breakpoint"` + `breakpointId` path won't match them.
   On match, invoke the bridge (`resolvePendingInterpreterBreakpoint` /
   `handleInterpreterMessage`) via an extension `script` command and then
   continue or stay. `handleInterpreterMessage` is shared and already works on
   cdb (it uses the now-validated `fetchInterpreterResult`).

3. **QML->C++ step-in (needs Qt >= 6.12 hook).** Arm
   `qt_v4NativeCallHookEnabled`, break at `qt_v4AboutToCallNativeMethodHook`,
   read `receiverMeta->d.static_metacall`, break there, step into the method.
   CDB can do each step; the descent must be driven from the engine (see the
   caveat above). Mirror `handleNativeCallHook` (gdb) /
   `handleNativeMethodStepInto` (lldb).

4. **Mixed stack splice — NOT a `fetchStack` port on cdb.** gdb/lldb do the
   splice in their Python `fetchStack`; cdb has **no** `fetchStack` (neither
   `cdbbridge.py` nor base `dumper.py` defines one). cdb stacks come from the
   C++ extension `stack` command via `CdbEngine::reloadFullStack` /
   `handleStackTrace` (cdbengine.cpp ~1449). So the splice (interleave the
   interpreter `backtrace` frames at the `qt_qmlDebugMessageAvailable` /
   `QV4::Moth::VME` frame + mark the metacall machinery run) must be done in
   `handleStackTrace` (combine the native stack with the service `backtrace`,
   which now works) or by teaching the extension `stack` command a native-mixed
   mode. Building block: the extension already has a `qmlstack` command +
   `AdditionalQmlStackCapability` (an on-demand, separate QML stack via the old
   `qt_v4StackTrace` path) - reusable for the QML frames, but it is not the
   inline splice.

5. **C++->QML step-in — the doubtful one.** This relies on "machinery skips":
   gdb `skip -rfu <regex>`, lldb `step-avoid-regexp`. **CDB has no equivalent.**
   Expect this direction to need a different approach (e.g. a temporary
   breakpoint on interpreter re-entry instead of skip-and-step) or to be left
   unsupported initially. Do not promise parity here.

Validation target once built: extend `tests/auto/debugger/tst_nativemixed.cpp`
to recognize the CDB engine and reuse the same checks (the test already gates
hook-dependent checks on Qt >= 6.12).

---

## 6. Known risks, ranked

1. **`.call` reliability/volume — CLEARED.** The experiment drove the full
   round-trip via `.call` with the pointer-marshalling workaround; reliable
   enough. The residual cost is the mandatory string marshalling (needs the new
   extension memory-write/allocate APIs in §5.1).
2. **Orchestration model.** No in-debugger event loop on CDB; expect engine /
   extension work, not a pure Python mirror.
3. **No skip mechanism** for C++->QML step-in.
4. Small audience for Windows/MSVC native-mixed QML debugging — weigh the
   effort against that.

Report back the experiment outcome (which stage, literal vs pointer args, raw
backtrace output, cdb + Qt versions) so the go/no-go is on record.

---

## 7. Continuing from here (the Windows inner loop)

The service round-trip is **built and validated** (June 2026, on a real
Windows/cdb/DbgEng box): the extension primitives (`cdbext.writeRawMemory`,
`cdbext.allocate`) and the `cdbbridge.py` overrides drive enable-service ->
setbreakpoint -> backtrace and return a live QML frame, with no code changes
needed beyond the original draft.

### Build and load the patched extension
- Build just the extension: `cmake --build <build> --target qtcreatorcdbext`.
  The DLL lands in `<build>/lib/qtcreatorcdbext64/qtcreatorcdbext.dll`
  (32-bit: `qtcreatorcdbext32`). A full Creator configure works fine and is
  not otherwise needed for this test.
- Drive cdb directly with that DLL (no full Creator needed). The validated
  command script is `cdb-roundtrip.cdb` next to this file - edit its `<...>`
  paths and run it with `cdb -cf cdb-roundtrip.cdb <fixture>\qmlmixtest.exe
  -qmljsdebugger=native,services:NativeQmlDebugger` plus the env shown in the
  script header (`QV4_FORCE_INTERPRETER=1`, `QT_QPA_PLATFORM=offscreen`,
  `_NT_SYMBOL_PATH` at the Qt `bin` + `plugins\qmltooling`).

### Two cdb-script gotchas (cost real time - heed them)
1. **`;` is a cdb command separator.** Put ONE python statement per
   `!qtcreatorcdbext.script` line; `theDumper = Dumper(); theDumper.setupDumpers()`
   on one line silently runs only the first half.
2. **`!qtcreatorcdbext.pid` is required** at the stop where you operate - it
   hooks the DbgEng output callbacks that `.call` / `allocate` depend on.
   Without it, `allocate` fails with "Attempt to allocate with no output
   hooked". (`tst_dumpers.cpp` calls `.pid` for the same reason.)

### Done / next milestone
Validation target once the engine side exists: extend
`tests/auto/debugger/tst_nativemixed.cpp` to recognize the CDB engine and reuse
the same checks (it already gates hook-dependent checks on Qt >= 6.12).

The remaining work is the **engine-level wiring** (§5 items 2+: the resolver /
QML-stop, then stepping; and §6 risks #2/#3). No in-debugger event loop on CDB
means it lives in `CdbEngine` + the extension, and it needs live Windows
iteration - a separate effort from this validated foundation.

### Context
Confirmed experiment findings + raw transcript: Gerrit change 745640
(`cdb-experiment-results.md`); the essentials are summarized in §5.
