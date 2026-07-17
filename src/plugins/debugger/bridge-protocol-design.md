# Debugger bridge protocol: a DAP-shaped internal protocol

Status: Draft / proposal for discussion
Scope: the wire protocol between the Debugger plugin (C++) and the Python
"bridge" scripts (gdbbridge.py, lldbbridge.py, cdbbridge.py, pdbbridge.py)

## 1. Summary

Evolve the ad hoc protocol between the Debugger plugin and its Python bridges
into a single, event-driven JSON protocol that:

- uses DAP (Debug Adapter Protocol) structures and vocabulary verbatim for the
  control/lifecycle plane, and
- uses native, purpose-built structures for the two domains where DAP's model
  is a poor fit: breakpoint registration and variable inspection.

The protocol is internal. Wire compatibility with foreign DAP clients is an
explicit non-goal. DAP is treated as a design reference and a source of proven
structures, not as a contract we owe fidelity to.

## 2. Background: what exists today

There are two generations of debugger-to-backend communication living side by
side in src/plugins/debugger/.

### 2.1 The classic bridge protocol (GDB, LLDB, CDB, PDB)

Outbound (plugin -> bridge). A DebuggerCommand carries a `function` name and a
QJsonValue `args` object; argsToPython() renders it to a Python dict literal and
the bridge dispatches the call. So the outbound direction is already
JSON-in-all-but-framing (see debuggerprotocol.h, DebuggerCommand).

Inbound (bridge -> plugin). dumper.py accumulates a GDB/MI-like structured text
stream via put()/putField() (name="...",type="...",value="...",children=[...])
which the plugin parses into GdbMi trees (debuggerprotocol.h, class GdbMi).
GDB piggybacks these on MI console-stream records (~"..."); LLDB frames them
with `@\n ... @\n` sentinels (lldbbridge.py report()); both share dumper.py and
the identical value grammar. This inbound stream carries semantics DAP has no
place for: stable dotted `iname` identity, `encoding` tags decoded by
DebuggerEncoding, childtype/childnumchild compression, lazy numchild,
editability, per-item display formats.

The control plane is hand-written per backend (GdbEngine, LldbEngine,
PdbEngine, and the CDB path each reimplement stepping, breakpoints, stack,
threads) and is tightly coupled to DebuggerEngine's explicit state machine
(CHECK_STATE assertions, notifyInferior*/notifyBreakpoint* transitions).

### 2.2 The existing DAP engine

src/plugins/debugger/dap/ already contains a working DapEngine + DapClient,
plus gdbdapengine, lldbdapengine, pydapengine, cmakedapengine. It talks real
DAP (Content-Length framed JSON, dapclient.cpp) to *native* DAP adapters
(gdb -i dap, lldb-dap, debugpy). Crucially it uses DAP's own scopes/variables/
evaluate and setBreakpoints -- it does not use dumper.py at all -- so it gets
the adapter's native (weaker) pretty-printing and a flat breakpoint model.

This is the reference for "Creator as a DAP client to foreign adapters". It
stays as-is under this proposal (see section 6).

## 3. Motivation

- The control plane is duplicated four times and is the fiddly, stateful,
  error-prone part of each engine. It should be written once.
- The classic inbound format is bespoke and undocumented; onboarding is costly.
- DAP is a proven vocabulary for exactly the control plane, and Creator already
  has the transport (DapClient) and a DAP-client mental model for foreign
  adapters. Sharing that vocabulary lets one client engine serve both our own
  bridges and foreign adapters, differing only in the data-plane path.

## 4. Goals and non-goals

Goals:

- One event-driven JSON transport between plugin and bridges.
- One control-plane implementation, driven by events, shared across backends
  and (for the control plane) with the foreign-adapter client path.
- Preserve the full richness of the current dumper and breakpoint models --
  no capability or display-quality regression relative to today.
- Keep breakpoint registration and variable inspection in shapes that match
  Creator's internal model directly (no wrap/unwrap, no impedance layer).

Non-goals:

- Wire compatibility with foreign DAP clients driving our bridge. We will not
  implement a DAP "floor" (stock setBreakpoints, variablesReference-based
  variables) on the bridge, dual-stack the data plane, publish a launch-args
  schema, or provide capability discovery for unknown peers. The only client of
  the bridge is Creator, shipped in lockstep with it.
- Replacing the foreign-adapter DAP client path (dap/ engine). That stays.

## 5. Design

### 5.1 Layering: control plane DAP-shaped, data plane native

The seam is the control/data plane boundary, chosen because that is exactly
where DAP's fit and its (internal) value coincide:

- Control/lifecycle plane -> DAP verbatim. Well-fitting, stable, and shareable
  with the foreign-adapter client.
- Data/inspection plane -> native. DAP's model here (declarative whole-source
  breakpoint replace; ephemeral variablesReference) is both ill-fitting and,
  since there is no foreign consumer, of no interop value.

One sentence for maintainers: we speak DAP for run control; we speak
Qt-native for inspection.

### 5.2 Transport and framing

Reuse the existing DapClient framing (Content-Length headers + JSON body, seq /
request_seq correlation, event dispatch; dapclient.cpp). Messages are DAP
"request"/"response"/"event" objects. Native messages are ordinary requests and
events under a reserved `qtc/` command prefix and `qtc*` body fields; they use
the same envelope, they are simply not part of the DAP spec.

### 5.3 Control plane (DAP verbatim)

Requests: initialize, launch, attach, configurationDone, continue, next,
stepIn, stepOut, pause, threads, stackTrace, scopes (see note in 5.5),
disconnect, terminate, restart, readMemory, writeMemory, disassemble,
setExpression/setVariable (value assignment), evaluate (repl/console commands).

Events: initialized, stopped, continued, exited, terminated, output, thread,
module, breakpoint (async breakpoint changes -- consumed, see 5.4).

The DebuggerEngine state machine is driven from these events (stopped ->
notifyInferiorStopOk and the fetch sequence; continued -> running; exited/
terminated -> shutdown), rather than from parsing MI records. The existing
DapEngine already drives the same state machine this way, so the pattern is
proven.

Notes on control-plane items that need care (from the GdbEngine audit):

- pause: the platform-specific interrupt logic (exec-interrupt vs
  target-async, Windows signal-by-pid, QNX; GdbEngine::interruptInferior)
  becomes the bridge's problem. This removes C++ from the plugin.
- run-to-line / run-to-function: no DAP request; emulate with a one-shot
  breakpoint + continue, exactly as GdbEngine does today with tbreak.
- executeReturn (force return), record/reverse enable, jump-to-line: partially
  or not covered by DAP; carried as qtc/ control requests where needed.
- token barrier (GdbEngine m_oldestAcceptableToken): the "these fetches are
  stale because we resumed" semantics is re-expressed as a client-side stop
  generation counter, not a protocol feature.

### 5.4 Breakpoints (native, incremental, tree-shaped)

Keep the DAP `stopped` event and hit reporting (control plane). Replace only
the *registration* model, which is where DAP is a poor fit.

Rationale. DAP registration is declarative and whole-source-replace, and its
`Breakpoint` is flat. Creator's model is incremental and tree-shaped: a
BreakpointItem can bind to multiple SubBreakpointItem locations, each
independently enable-able (breakhandler.h). Forcing this through DAP
setBreakpoints requires recomputing the full per-source set on every change and
correlating a positional response array back onto individual breakpoints. The
current DapEngine attempts this and demonstrates the traps: it correlates by
"file:line" (dapengine.cpp handleBreakpointResponse) rather than by request/
response position, which breaks on relocation (bound line != requested line),
same-line collisions, and path-normalization mismatches (path() vs
toUrlishString()). None of this is necessary for an internal protocol.

Native shape. Incremental requests that mirror the internal model 1:1:

    qtc/insertBreakpoint  { <BreakpointParameters> }
        -> { id, verified, message,
             locations: [ { id, address, file, line, enabled }, ... ] }
    qtc/updateBreakpoint  { id, <changed fields> } -> { ... }
    qtc/removeBreakpoint  { id } -> { ok }
    qtc/enableSubBreakpoint { bpId, locationId, enabled } -> { ok }

Identity is the breakpoint id (== responseId today); sub-breakpoints are
first-class children in the response, not something to reconstruct. Async
changes (pending bp binds on shared-library load, adapter relocates/removes)
arrive as a qtc/breakpointChanged event keyed by the same id.

As built: the C++ side serializes parameters with the shared addToCommand() and
correlates responses by the stable modelid (never file:line). The bridge
returns the breakpoint in the MI 'bkpt' shape (with a sub-location list), which
the C++ side feeds to the shared updateFromGdbOutput()/handleBkpt() - the same
path GdbEngine uses. Verified end-to-end against real gdb (insert -> resolved
bkpt -> stop at breakpoint). Still to do: qtc/enableSubBreakpoint, the async
qtc/breakpointChanged event, and demangling the location function name (gdb's
Python BreakpointLocation.function returns the mangled name).

Parity items carried natively (no DAP equivalent, and no reason to omit them):

- multiple locations per breakpoint with per-location enable;
- thread-/inferior-/queue-specific breakpoints;
- catchpoints: fork/vfork/exec/syscall/signal/load (GdbEngine uses these today,
  gdbengine.cpp insertBreakpoint);
- tracepoints with structured capture expressions (gdbtracepoint.py);
- commands / script-callback on hit, auto-continue (lldbbridge.py
  SetScriptCallbackFunction);
- watchpoints with read/write/access and scope, beyond DAP dataBreakpoints;
- conditions, ignore counts, one-shot -- expressed directly rather than mapped
  onto DAP's loosely-specified hitCondition string.

### 5.5 Variables (native, iname identity)

Do not use variablesReference. It is the worst-fitting DAP concept for Creator:
an ephemeral integer handle invalidated on every resume, which fights the
stable-iname model and forces re-walking the tree each stop.

Native shape. Inames are the identity. A single request returns the dumper tree
for the requested roots (locals, watchers) in one round trip, carrying the full
dumper metadata (iname, encoding, childtype/childnumchild, numchild for lazy
expansion, editability, display-format hints, address). Lazy expansion is a
follow-up request naming the iname to expand. This is the current dumper output,
keyed by iname; WatchHandler / WatchItem consume it directly.

As built (qtc/fetchVariables): the C++ side reuses the shared updateLocalsView()
- the same path GdbEngine uses - and drives the real Qt dumpers over the bridge.
The dumper's structured output is carried in the response verbatim and parsed
with the existing GdbMi reader; transcoding it to a pure JSON tree on the wire
is a deferred cleanup that does not change the architecture (no
variablesReference, iname identity, dumper-aware values). Lazy expansion and
watchers ride along via WatchHandler's existing format/watcher requests (the
'expanded' set among them).

scopes may be retained as a thin DAP-shaped grouping if convenient, but the
variable payload itself is native.

### 5.6 Versioning

Plugin and bridges ship together in one release; for remote debugging the
bridge runs in the host-side gdb, so plugin/bridge skew is effectively nil. A
single protocol-version integer exchanged at initialize is sufficient. No DAP
capability discovery, no graceful degradation against unknown peers.

## 6. One client engine, two backend kinds

Creator remains a DAP client to foreign adapters (gdb -i dap, lldb-dap,
debugpy) via the existing dap/ engine path -- unchanged, at DAP fidelity.

Because our own bridges speak the same control-plane vocabulary, Creator's
engine can be a single implementation whose control plane is identical for both
backend kinds, and whose data plane branches by peer:

- own bridge  -> qtc/ breakpoints + iname variables (rich);
- foreign adapter -> DAP setBreakpoints + variables (thin).

The control-plane state machine -- the part duplicated four times today -- is
written once and serves everything. This is the primary internal payoff and it
survives even though foreign-server interop does not.

## 7. Migration path

1. Introduce the JSON transport and the DAP-shaped control plane for one
   backend (GDB), driving the state machine from events. Keep the existing
   dumper/breakpoint channels behind it initially.
2. Move variables to the native iname payload (dumper tree via updateLocalsView),
   retiring the GdbMi text grammar for that path.
3. Move breakpoints to the native incremental/tree requests, retiring the
   whole-source-replace logic.
4. Repeat for LLDB (shares dumper.py; only framing differs today).
5. CDB and PDB follow; CDB is the outlier with no native DAP adapter, which is
   fine here since we are not using foreign adapters for our own backends.
6. Fold the four engines' control-plane logic into the shared engine.

Each step is independently shippable and testable behind the existing
per-backend selection.

## 8. Risks and open questions

- The data-plane payload (dumper tree) is large and gets no standardization
  benefit; the reuse win is concentrated in the control/lifecycle skeleton.
  Accepted: that skeleton is where the duplicated, error-prone code is.
- Re-rooting the state machine on events instead of MI parsing is the main
  non-mechanical work. De-risked by the existing DapEngine doing exactly this.
- Native-mixed C++/QML debugging (interpreter breakpoints, additional QML
  stack, mixed stepping) needs its own qtc/ requests/events; design TBD.
- Registers and peripheral registers have no DAP request; carried as qtc/
  requests (or modeled as a variables scope). Decide per UI need.
- Exact qtc/ schemas above are illustrative, not final.

## 9. Alternatives considered

- Keep the fully custom protocol. Zero interop, control-plane state machine
  duplicated per backend -- the current pain. Rejected.
- Adopt stock DAP wholesale (delete bridges, use native adapters). Loses the
  dumpers and the rich breakpoint model; regresses display quality; cannot
  cover MSVC/CDB with Qt dumpers. This is what today's dap/ engine does, at that
  cost. Rejected as the primary path (retained only as the foreign-adapter
  client).
- DAP-compatible profile with a full extension namespace and a DAP floor, so
  foreign clients can drive the bridge. Coherent, but requires dual-stacking the
  data plane, a launch-args schema, and capability discovery -- cost with no
  benefit once foreign-client interop is declared a non-goal. Rejected.

The chosen design is the above minus the foreign-client obligations: DAP-shaped
where it fits, native where it does not, internal-only. With no external
consumer there is no second master to serve, so the protocol is optimized
purely for fit.
