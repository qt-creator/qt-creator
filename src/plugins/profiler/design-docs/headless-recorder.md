# Headless recorder (`qtprofiler-cli`)

A console tool that records traces and has no UI, so that an editor
extension (VS Code) can start and control recordings and hand the traces to
the WebAssembly viewer. It is meant to be small: it must not link the Qt
Creator plugin stack, Qt Widgets or the Utils library as they are today.

## Protocol

JSON-RPC 2.0 on stdin/stdout, one message per line (newline-delimited,
compact JSON, no embedded newlines). stderr is for diagnostics only.
`--rpc` selects it; without it the tool runs one recording from the command
line (see `--help`) and prints the trace path.

Requests (client to tool):

| Method            | Params                                               | Result |
|-------------------|------------------------------------------------------|--------|
| `initialize`      | none                                                 | `{name, version, protocolVersion, backends:[{id, name}]}` |
| `startRecording`  | `{backend, executable, arguments?, workingDirectory?, environment?, options?, paused?, durationMs?}` | `{}` |
| `pauseRecording`  | none                                                 | `{}` |
| `resumeRecording` | none                                                 | `{}` |
| `stopRecording`   | none                                                 | `{}` |
| `getStatus`       | none                                                 | `{state, paused, recordedMs}` |
| `shutdown`        | none                                                 | `{}`, then the tool exits |

`backend` is an id from `initialize`, or a case-insensitive part of a name.
`options` maps a backend setting's key (for example `IntervalUs`) to its value.
`state` is `idle`, `starting`, `capturing` or `processing`.
End of input stops a running recording and exits.

Errors use the JSON-RPC 2.0 codes, and two of the tool's own: `-32001` for
a request the tool is in the wrong state for, and `-32002` for a recording
that could not be started, with the `fix?` of `recordingFailed` in its
`data`. A recording that fails to start sends no notifications.

Notifications (tool to client): `recordingStarted {target}`,
`captureStarted`, `pausedChanged {paused}`, `processingStarted`,
`progress {percent}`, `status {text, toolTip}`,
`recordingFinished {tracePath, format}` and
`recordingFailed {message, fix?}`.

## Layering

The recording engine -- `Sampler`, `RecordingSession`, the backends, the
sample trace model and writer -- is already separate from the UI: only the
per-backend settings classes carry widget layouts. The headless tool and the
viewer/Creator are meant to share it, so it has to stop depending on the
`Utils` library, which pulls in Widgets, XML, PrintSupport and more.

What it needs from `Utils` is small, and is moved into libraries of their own
that depend on QtCore only:

* **A file path library.** The path type the engine uses: join, parent, file
  name and suffix, exists, read and write files, make directories, with the
  semantics the engine already relies on.
* **A process library.** Launching and watching a target, and running helper
  tools: command line, environment, working directory, output and finished
  signals, terminate and kill, on top of `QProcess`.

`Utils` itself is meant to build on these rather than keep copies, so the two
have one implementation of what they share; the parts only Creator needs
(remote devices, shell handling, Creator's environment model) stay in `Utils`
as extensions of the small types. The engine, `CommonTraceFormat` and
`QmlDebug` depend on the small libraries instead of `Utils`.

## Steps

1. The tool and the protocol, driving the existing recorder (this stage).
   Gives a working vertical slice and a size baseline.
2. The small path and process libraries, with `Utils` building on them.
3. Port `CommonTraceFormat`, `QmlDebug` and the engine to them; move the
   engine into a library that needs only QtCore (and QtNetwork for QML), and
   link the tool against that.
4. Size work: no Widgets/Gui, LTO, `-Os`, dead stripping, no iostream.
