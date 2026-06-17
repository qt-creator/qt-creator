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

## 5. If the experiment passes: what building CDB support involves

In rough order, and with the honest caveats:

1. **Service round-trip in `cdbbridge.py`.** Give `sendInterpreterRequest`/
   `fetchInterpreterResult` a CDB path: issue the calls via `cdbext.call`
   (marshalling strings into target memory if literals are unreliable), read
   `qt_qmlDebugMessageBuffer`/`Length`, hex-decode. This is the part the
   experiment de-risks.

2. **Interpreter breakpoints + the "QML stop".** Implement the equivalents of
   gdb's resolver / `InterpreterMessageBreakpoint`: a breakpoint on
   `qt_qmlDebugConnectorOpen` that enables the service and resolves pending QML
   breakpoints, and one on `qt_qmlDebugMessageAvailable` whose hit means "a QML
   event — read the message and maybe stay stopped."

   **Caveat (orchestration model):** gdb hangs this off `gdb.events.stop` and
   lldb off its event loop. CDB has no in-debugger Python event loop — stops
   are reported to the C++ `CdbEngine`. So this wiring likely lives in
   `cdbengine.cpp` / the extension, which must recognize a stop at those
   symbols and call into the bridge. Budget for engine-level work, not just a
   `cdbbridge.py` mirror.

3. **QML->C++ step-in (needs Qt >= 6.12 hook).** Arm
   `qt_v4NativeCallHookEnabled`, break at `qt_v4AboutToCallNativeMethodHook`,
   read `receiverMeta->d.static_metacall`, break there, step into the method.
   CDB can do each step; the descent must be driven from the engine (see the
   caveat above). Mirror `handleNativeCallHook` (gdb) /
   `handleNativeMethodStepInto` (lldb).

4. **Mixed stack splice.** Port `fetchStack`'s splice + metacall dimming:
   splice the interpreter `backtrace` frames in at the
   `qt_qmlDebugMessageAvailable` / `QV4::Moth::VME` frame and mark the metacall
   machinery run. Straightforward once the backtrace works.

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

1. **`.call` reliability/volume.** The protocol issues several inferior calls
   per stop; cdb `.call` is historically fragile and used sparingly today. The
   experiment is precisely about this.
2. **Orchestration model.** No in-debugger event loop on CDB; expect engine /
   extension work, not a pure Python mirror.
3. **No skip mechanism** for C++->QML step-in.
4. Small audience for Windows/MSVC native-mixed QML debugging — weigh the
   effort against that.

Report back the experiment outcome (which stage, literal vs pointer args, raw
backtrace output, cdb + Qt versions) so the go/no-go is on record.
