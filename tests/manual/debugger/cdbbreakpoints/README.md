# cdb breakpoint scoping

`cdbbreakpoints.py` measures what an unresolved file-and-line breakpoint
costs while an application loads its libraries.

cdb keeps a `bu` breakpoint unresolved until a module turns up that
contains the source file, and re-checks the pending expression on every
module load. Naming the module in the expression - ``bu `module!file:line` ``
- reduces that check to a name comparison for every module the file cannot
be in.

## Running

From a Visual Studio command prompt, with `ninja` on PATH:

    python cdbbreakpoints.py --dlls 200 --breakpoints 1,10,36

It generates an application that loads `--dlls` libraries, builds it, and
then starts it under cdb with the given numbers of breakpoints, once with
plain expressions and once scoped to the module. `resolved` and `hit` in
the output confirm that both forms produce working breakpoints; a fast run
with `resolved=0` would only mean the expression was never bound.

`--keep` reuses an already generated fixture, `--cdb` points at another
cdb.exe.

## Measured

Both setups ran on Windows 10.0.26200 with cdb 10.0.26100, every row with
all breakpoints resolved.

### The generated fixture, 200 modules

| breakpoints | plain             | scoped          |
|-------------|-------------------|-----------------|
| 0           | 0.28 - 0.34 s     | -               |
| 1           | 9.46 - 10.17 s    | 0.32 - 0.34 s   |
| 10          | 46.79 - 47.62 s   | 0.33 - 0.35 s   |
| 36          | 156.46 - 172.07 s | 0.34 - 0.37 s   |

That is about 48 ms per module load for the first unresolved breakpoint and
22 ms for each further one, so the cost is the product of breakpoint count
and module count. `--dlls 40 --breakpoints 1,5` shows the same shape in a
few seconds.

### Qt Creator itself, 269 modules

A RelWithDebInfo Qt Creator started as

    qtcreator.exe -platform offscreen -noload Welcome -settingspath <fresh> -test all

loads 269 modules, 120 of them its own, and exits once every plugin is up,
so the run measures startup. One breakpoint per plugin, spread over 72
plugin DLLs:

| breakpoints | plain      | scoped   |
|-------------|------------|----------|
| 0           | 7.8 - 9.8 s| -        |
| 1           | 13.8 s     | 9.4 s    |
| 10          | 23.1 s     | 9.5 s    |
| 36          | 39.4 s     | 10.9 s   |

The 36 rows hit three of their breakpoints in either form. Scoping holds
startup at the no-breakpoint cost, unscoped 36 breakpoints cost four times
that.

An expression that never resolves is re-checked for the whole run and is
much more expensive: a single one took 198 s on the first run of a session
and 6 s on later ones, with the plugin PDBs in the file cache by then. It
can also end the session outright, because a file name alone can match
ambiguously in a module that has nothing to do with the breakpoint:

    Matched: Help!Help::Internal::createBookmarkManagerTest+0x6
    Matched: Help!Help::Internal::createBookmarkManagerTest+0x3b
    Breakpoint 0's offset expression evaluation failed.
    WaitForEvent failed, Win32 error 0n22

Startup ended there, 89 modules short. Scoped to `Core!`, the same
breakpoint stays unresolved, no module load can match it by accident, and
startup completes.

A source file that is compiled into a static library needs the link graph
to be scoped: its module is whatever links the library, and the path does
not tell. Eight such breakpoints, all resolved, cost 15.0 s against 9.0 s
scoped:

| source                         | static library    | module          |
|--------------------------------|-------------------|-----------------|
| libvterm/src/screen.c          | libvterm          | TerminalLib     |
| libptyqt/conptyprocess.cpp     | ptyqt             | Utils           |
| qmldesignerutils/asset.cpp     | QmlDesignerUtils  | QmlDesigner     |
| texteditor/tabsettingsdata.cpp | TextEditorSupport | QmlDesignerCore |

Three of thirteen static library sources cannot be set by file and line at
all, scoped or not, because the line has several addresses within one
module:

    Matched: Utils!PtyQt::createPtyProcess+0x34 (00007ff8`7834d734)
    Matched: Utils!PtyQt::createPtyProcess+0x56 (00007ff8`7834d756)
    Ambiguous symbol error at '`ptyqt.cpp:52 `'

The engine answers that with one address breakpoint per match.

A source that several binaries link becomes one scoped breakpoint per
binary, eight for `asset.cpp`. Ten of them, the eight plus two for
`tabsettingsdata.cpp`, are free: a `-test Core` startup that loads 100
modules takes 4.12 s without breakpoints and 4.16 s with the ten, all
still unresolved because none of their modules is among the 100. The two
unscoped ones instead end the session after 96 modules.

The debugger derives the modules from the project (`modulesForBreakpoint()`
in `cdbengine.cpp`, `binariesForSourceFile()` in `cmakebuildsystem.cpp`),
which is why a breakpoint set in the editor now carries one, and one
breakpoint per binary when the source ends up in several.
QTCREATORBUG-27058 and QTCREATORBUG-30265 have the reports this came from,
including 42 s against 20 s application startup on a real project.
