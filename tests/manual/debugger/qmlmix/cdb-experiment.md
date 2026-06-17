# CDB native-mixed feasibility experiment

## Purpose

Native combined (C++ + QML in one debugger session) debugging works in Qt
Creator with **GDB** and **LLDB**. The mechanism: a single debugger drives both
the C++ inferior and an in-process QML interpreter via the `qmldbg_native`
plugin's "NativeQmlDebugger" service. The debugger talks to that service by
**calling functions in the inferior** (enable the service, send a request,
read a reply buffer).

The open question for Windows/MSVC (**CDB**) is the make-or-break primitive:

> Can CDB reliably make these inferior function calls — including ones that
> take string arguments — at a QML stop, and get a QML backtrace back?

If yes, building out a CDB native-mixed bridge is worthwhile. If `.call` is
flaky here, the whole approach is a dead end on CDB and not worth the work.

**This experiment does NOT need any Qt Creator code changes.** It pokes the
service directly from a raw `cdb` session. You only need a built test app and
a debug-info Qt. It is staged so you stop as soon as you get a clear no-go.

You do not need Qt Creator's debugger bridge for stages A–C; it is raw `cdb`.

---

## Prerequisites

1. **Windows with MSVC** and the **Debugging Tools for Windows** (`cdb.exe`,
   usually `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`).
2. **A debug Qt 6 build** (shared, with PDBs) that includes the
   `qmldbg_native` plugin. A normal Qt install has it
   (`<qt>/plugins/qmltooling/qmldbg_native.dll`). PDBs are required so `.call`
   has function prototypes.
   - For the *later* stepping work you would also need the qtdeclarative
     `qt_v4AboutToCallNativeMethodHook` patch (targeted for Qt 6.12). **This
     experiment does not need the hook** — it only exercises the service.
3. **The `qmlmix` test app** from this repo:
   `tests/manual/debugger/qmlmix`. Build it **Debug** with your MSVC Qt kit
   (it produces `qmlmixtest.exe`). Keep the source tree around — you need the
   absolute path to `Main.qml`.

> If you have Claude Code on the Windows box, point it at
> `cdb-reference.md` (next to this file) for architecture,
> source-file pointers, and the exact service protocol. It can automate
> stages A–C and, if they pass, scaffold the CDB bridge.

---

## Background you need for the commands

All service entry points are **`extern "C"`** (no C++ mangling) and live in the
`qmldbg_native` plugin module. Signatures:

```c
void        qt_qmlDebugConnectorOpen();                 // breakpoint target (startup)
bool        qt_qmlDebugEnableService(const char *name); // "NativeQmlDebugger"
bool        qt_qmlDebugSendDataToService(const char *serviceName, const char *hexData);
const char *qt_qmlDebugMessageBuffer;                   // global pointer to the reply
int         qt_qmlDebugMessageLength;                   // global reply length
void        qt_qmlDebugClearBuffer();
void        qt_qmlDebugMessageAvailable();              // breakpoint target (a QML break/event)
```

Requests are JSON, hex-encoded, passed as the `hexData` argument. Replies land
in `qt_qmlDebugMessageBuffer` as `"<service> <len> <json>"`. Synchronous
requests like `backtrace` fill the buffer and return; asynchronous events
(a hit QML breakpoint) call `qt_qmlDebugMessageAvailable`.

Find the actual module name once inside cdb (it may be `qmldbg_native` or have
a suffix):

```
x *!qt_qmlDebugEnableService
```

Use whatever module cdb prints (call it `MOD` below, e.g. `qmldbg_native`).

---

## Launch

Run the app under cdb with the QML debug service enabled and JS forced to the
interpreter (so the QML stack is introspectable):

```
set QV4_FORCE_INTERPRETER=1
cdb.exe -g -G qmlmixtest.exe -qmljsdebugger=native,services:NativeQmlDebugger
```

(`-g`/`-G` skip the initial/final breakpoints.) You should land at the cdb
prompt with the process loaded.

---

## Stage A — does `.call` work at all here?

Goal: confirm cdb can inject a call in this process/build before involving the
service.

```
bu Qt6Core!qVersion
g
.call Qt6Core!qVersion()
g
```

- The `bu`/`g` just gets you to a clean stop early; then `.call qVersion()` +
  `g` should print a returned `char *` (the Qt version string).
- **No-go if:** `.call` errors with "unable to resolve"/"no type information"
  (missing PDBs) or corrupts the process. Fix PDBs / debug build first; if it
  still fails for a Qt function, `.call` is unusable here -> **STOP, report A
  failed.**
- **Go:** you see a version string. Continue.

---

## Stage B — `.call` with a string argument (the crux)

This is the single most important check: passing a C string literal through
`.call`.

```
bu MOD!qt_qmlDebugConnectorOpen
g
```

(You should stop in `qt_qmlDebugConnectorOpen` during startup. The QML engine /
service is being set up.) Now enable the service:

```
.call MOD!qt_qmlDebugEnableService("NativeQmlDebugger")
g
```

- **Go:** the call returns (a `bool`, typically `true`) and the process is
  intact (`k` shows a sane stack, `g` would continue normally).
- **No-go if:** cdb cannot pass the string literal (error about argument types
  / strings), or the process faults during/after the call. **This is the
  likely failure point.** If it fails, try the explicit-memory workaround in
  "If string literals fail" below before concluding. If that also fails,
  **STOP, report B failed** — the service protocol cannot be driven from cdb.

---

## Stage C — full round trip: get a QML backtrace

With the service enabled, set a QML breakpoint, run to it, and read a backtrace.

### C.1 Set a QML breakpoint via the service

Pick an executed JS line. In `Main.qml`, the `Timer.onTriggered` handler fires
repeatedly — grep for `MARKER: handler-js` and note its line number (example
below uses **49**). Build the request hex (any Python):

```python
import json
req = {"command": "setbreakpoint",
       "arguments": {"file": r"C:\full\path\to\Main.qml", "line": 49, "condition": ""}}
print(json.dumps(req).encode().hex())
```

Then, at the `qt_qmlDebugConnectorOpen` stop:

```
.call MOD!qt_qmlDebugSendDataToService("NativeQmlDebugger","<HEX_FROM_ABOVE>")
g
```

### C.2 Run to the QML breakpoint

```
bu MOD!qt_qmlDebugMessageAvailable
g
```

The QML breakpoint firing calls `qt_qmlDebugMessageAvailable`, so cdb should
stop there. (If it never stops, the breakpoint was not set — revisit C.1 / the
line number.)

### C.3 Ask for a backtrace and read the reply

Disable the message breakpoint first so the synchronous reply does not recurse:

```
bd *                       (or: bd <id-of-qt_qmlDebugMessageAvailable>)
```

The `backtrace` request hex is fixed (`{"command":"backtrace","arguments":{"limit":10}}`):

```
.call MOD!qt_qmlDebugSendDataToService("NativeQmlDebugger","7b22636f6d6d616e64223a20226261636b7472616365222c2022617267756d656e7473223a207b226c696d6974223a2031307d7d")
g
```

Now read the reply buffer and length:

```
dd MOD!qt_qmlDebugMessageLength L1
da poi(MOD!qt_qmlDebugMessageBuffer)
```

- **GO (the answer we want):** `da` prints a string like
  `NativeQmlDebugger <len> {"frames":[{"function":"onTriggered",...}, ...]}`
  with recognizable QML frames (function names, the `Main.qml` file, line
  numbers). That proves the full service round-trip works on cdb.
- **No-go if:** the buffer is empty / length 0 / garbage, or the `.call`
  faulted. Report C failed with the raw `da` output.

Optionally clear the buffer to confirm the no-arg call too:

```
.call MOD!qt_qmlDebugClearBuffer()
g
```

---

## If string literals fail (Stage B/C workaround to try before giving up)

`.call` may refuse string literals but still accept pointer arguments. Write
the bytes into target memory and pass the address:

1. Allocate scratch in the debuggee (e.g. reuse a large static buffer, or
   `.dvalloc 1000` to allocate a page — note the returned address `ADDR`).
2. Write the ASCII string + NUL with `eb`:
   ```
   eb ADDR 4e 61 74 69 76 65 51 6d 6c 44 65 62 75 67 67 65 72 00   ; "NativeQmlDebugger\0"
   ```
3. Call with the address cast to `char *`:
   ```
   .call MOD!qt_qmlDebugEnableService((char *)0xADDR)
   g
   ```

For the hex-data argument do the same with the request's ASCII bytes. If the
pointer form works where literals do not, that is still a **GO** — the bridge
would just always marshal strings into target memory (it already hex-encodes,
so this is a natural fit).

---

## Decision

| Result | Meaning | Next |
|--------|---------|------|
| A fails | `.call` unusable in this build | Stop. Likely PDB/build issue; if intrinsic, CDB native-mixed is not viable. |
| B fails (both literal and pointer forms) | cannot pass args to inferior calls | **Stop.** Service protocol cannot be driven from cdb. |
| C fails after B passed | calls work but the round-trip/buffer does not | Capture raw output; investigate framing before deciding. |
| **C succeeds** | the service round-trip works on cdb | **Green light.** Proceed to build the CDB bridge (see reference doc, "If it passes"). |

Please record, for whoever continues this: the exact cdb version, the Qt
version, whether literals or the pointer workaround were needed, and the raw
`da` backtrace output.
