# How running and debugging on HarmonyOS works internally

A reading companion to the code, written because the run and debug paths are
not shaped by what an IDE would normally do but by what this device refuses.
Every unusual move below exists because of one of the refusals in the next
section. Source of truth is the code in `src/plugins/harmonyos` and
`share/qtcreator/harmonyos`; the plugin's own `README.md` says what was watched
working and what was not, and is not repeated here.

Two directions are covered, and they are not the same code:

* Qt Creator on a Linux host, the device as a target. Everything goes through
  `hdc`.
* Qt Creator on the device, the device as a build device. No `hdc` is reachable
  from inside an application sandbox, so almost nothing of the first route
  applies.

## What the device refuses, and what it grants

These are the constraints the rest of the document is a consequence of. All of
them were measured on a consumer HAD-W32.

* `PTRACE_TRACEME` is refused everywhere. So nothing can be launched under a
  debugger, and the debug server has to be started from inside the process it
  will debug.
* `PTRACE_ATTACH` is allowed from inside an installed application that was
  signed with a debug profile. That is the one opening the whole debug path
  goes through.
* `execve` of a file an application wrote itself is refused, and so is
  `dlopen` of it. The SELinux type of the application's own storage carries no
  execute bit.
* `PROT_EXEC` on anonymous memory is granted to a debug-signed application, but
  only while the platform's "jit" `prctl` is on. A plain `mprotect` is refused
  with `EINVAL`.
* An application gets no environment from the framework, the parameter store is
  root-only, and a file the host writes is not readable from the application
  sandbox. So there is no ordinary way to tell a launch anything.
* The device's own loopback is reachable from an application, and `hdc` can put
  a listener on it with a reverse forward (`hdc rport`). That is the only
  channel into a launch, and it is used twice, for two different questions.
* From inside an application sandbox the `aa` command is not reachable, and the
  only Want an application may send is an implicit one. So Qt Creator on the
  device starts things by URI scheme, not by name.
* A package may not `dlopen` a library that was not installed with it.

## The moving parts

Ports, from `harmonyosconstants.h`:

| Port | Name | Direction | What it means |
|---|---|---|---|
| 8123 | debug | forward | the debug server inside the application |
| 8124 | gate | reverse | "this launch is being debugged" |
| 8126 | channel | reverse | "here is the application to run" |
| 8022 | ssh | forward | the terminal environment, gone since 2026-09 |

Pieces that travel to the device:

* `qtcwait.cpp` to `libqtcwait.so`. A constructor that holds a debug build at
  startup. Compiled on demand by `Sdk::waitLibrary()` with the SDK's clang,
  cached under `userResourcePath("harmonyos")`, mtime-checked against the
  source in `share/qtcreator/harmonyos`.
* `qtcrunner.cpp` plus `qtcload.cpp` to `libqtcrunner.so`. Takes the
  application's place in the package when running without installing.
  `Sdk::runnerLibrary()`, same mechanism.
* `qtcdebugplugin.cpp` to `libqtcdebug.so`. A `QGenericPlugin` that starts the
  debug server from inside the process.
* `lldb-server` from the SDK, and `qtchost`, both inside the `qtctools` native
  package at `/data/app/qtctools.org/qtctools_1.0/bin`.

## Running from a host

`HarmonyOsRunWorkerFactory` in `harmonyosrunconfiguration.cpp`. The recipe is a
task tree, and its shape is dictated by two facts: `aa start` returns as soon
as the ability was launched, so it cannot stand in for the application's
lifetime, and `hilog` keeps waiting once the process is gone, so it cannot say
when the run ended either.

1. The bundle name is read out of the generated `AppScope/app.json5`, comments
   stripped first because the file is JSON5 and a commented-out entry would
   otherwise be picked up. Without a bundle name there is nothing to launch:
   the device installs only under the name the provisioning profile is for.
2. `aa force-stop` the bundle. A launch reaches a running instance instead of
   starting one, which would silently make the run a no-op.
3. `aa start -a QAbility -b <bundle> -m entry`. Launch arguments come from the
   project's `<target>-harmonyos-extras.json` and are passed as
   `--ps io.qt.appArgsJson '<json>'`, quoted because `hdc` hands the whole line
   to a shell on the device. `--pb io.qt.useUriAsArg false` goes with it, or
   the launch URI arrives as the first argument and an application that opens
   file names tries to open it.
4. `pidof -s <bundle>` until it answers.
5. In parallel: `hilog -P <pid>` feeds the application output pane and keeps
   the run alive, and a one-second `pidof` loop is what ends the run when the
   process is gone.
6. `aa force-stop` again on the way out.

## Running without installing

The "Run without installing" setting. Installing a package costs about a minute
of packaging, signing and installing per run, and only the application library
actually changes between runs.

The package holds `libqtcrunner.so` in the application's place, and the
application library is dropped from it, which is what keeps the package
identical across edits so it is installed once instead of once per run. At
every run:

1. Qt Creator opens a `QTcpServer` on a free host port and `hdc rport`s the
   device's 8126 to it, after removing a stale reverse forward left by a run it
   did not get to clean up after.
2. The runner's `main()` connects to 8126 on the device's loopback and is sent
   a four-byte length and that many bytes, which it writes to
   `/data/storage/el2/base/files/qtcrun/libapp.so`.
3. `QtcLoad::load()` maps it, `QtcLoad::lookup()` finds `main`, and the runner
   calls it as the loader would have. Nothing creates a `QApplication`: the
   Qt main thread, the ability and the platform plugin's surface are already in
   place from the template, and stay valid.
4. The channel stays open for the run. Whatever the runner writes to it lands
   in the application's output, which is the only diagnostic it has before Qt
   is up. Closing it is how the runner says `main()` returned.

Arguments ride ahead of the application on the same channel, announced by
`0xffffffff`, a length no application can have. They are only sent where the
launch cannot carry them.

What this costs in fidelity: the run happens under the runner's identity, so
anything that depends on the application's own manifest has to be checked with
a real installed package.

## Running from Qt Creator on the device

`handoverRecipe()`, selected by a compile-time `Q_OS_OHOS` rather than by
anything the build device says, because a build device reports the Linux its
toolchains target.

The channel is the same, and it is simpler: an application's loopback is the
device's own loopback, so port 8126 is bound directly and no forward is needed.
What is harder is starting the runner at all. `aa` is not reachable and only an
implicit Want may be sent, so the package declares the `qtcrun` scheme and
Creator sends `QDesktopServices::openUrl("qtcrun://run")`.

Two consequences worth knowing when this fails. The runner has 30 seconds to
ask, and if it does not, the likely cause is that it is already running, since
a launch reaches a running instance. And stopping it is not possible at all:
force-stopping an ability is not something an application may do, so cancelling
the run closes the channel and says so, and the application keeps running.

## Debugging an application on the device, from a host

`HarmonyOsDebugWorkerFactory`. Because `PTRACE_TRACEME` is refused, the shape
is inverted from the usual one: the application starts the server and the
debugger attaches.

1. **The gate.** A `QTcpServer` on a free host port, with the device's 8124
   reverse-forwarded to it for as long as the run lasts. A stale forward is
   removed first, as with the channel.
2. **force-stop, then `aa start`** with `--ps io.qt.appArgsJson
   '["-plugin","qtcdebug:8123"]'`. Loading the debug plugin through Qt's
   generic plugin mechanism is what starts the server without the application
   knowing anything about it.
3. **The application decides whether it waits.** A debug build is linked
   against `libqtcwait.so`: the kit inserts `-Wl,--no-as-needed <path>
   -Wl,--as-needed` into the three `*_LINKER_FLAGS_DEBUG` variables, because
   nothing references the library and only being a `DT_NEEDED` entry makes the
   loader initialize it before the library that names it, which is before any
   static initializer of the application and before `main()`. Its constructor
   connects to 8124 on the loopback. A connect that succeeds means the tools
   are waiting; a refused one is an ordinary launch and it returns at once.
4. **If it is waiting**, `libqtcwait.so` forks `lldb-server platform --listen
   *:8123` itself and then spins for up to 60 seconds on an exported
   `volatile bool qtc_waitForDebugger`. It also fills an exported
   `char qtc_launchDump[1024]`, which the debugger reads straight out of the
   process, because nothing else reports from that point: `hilog` carries
   nothing this early.
5. **A platform, not a bare server.** Both the plugin and the wait library
   start `lldb-server platform`, and the run parameters set
   `lldbPlatform("remote-ohos")`. The reason is the next step.
6. **Wait for the port, not the process.** The application starts the server a
   little after its own process appears, so the recipe polls `netstat -ln` for
   8123 in `LISTEN` for up to 30 seconds, then `hdc fport tcp:8123 tcp:8123`.
7. **Attach**, with `AttachToRemoteServer`, `skipDebugServer`,
   `useContinueInsteadOfRun` and the pid the launch produced.
8. **Place the modules.** The platform reports no module list for a process it
   did not launch, so without this nothing is loaded, no frame can be named and
   no breakpoint can be placed. `placeMappedModules()` in the `remote-ohos`
   branch of `lldbbridge.py` reads the process memory regions, which do say
   which file is mapped where, takes the lowest mapping of each shared object
   as its load address, and places the local copy there. The search paths are
   the run's own `solibSearchPath`: the build directory,
   `harmonyos-build/entry/libs/arm64-v8a` and the Qt version's `qtSoPaths()`.
   Addresses change with every launch, which is why they are read at attach
   time rather than assumed. Frames in the musl loader, `appspawn` and the OHOS
   framework stay nameless, because there is no local copy to place.
9. **Release the application.** `releaseApplication()` pokes the
   `qtc_waitForDebugger` byte in `libqtcwait.so` to zero. It is tried after
   every placement rather than once, because that library is mapped together
   with the application's own one, which the framework opens long after the
   process starts.
10. **Keep placing.** `watchLoaderForMappedModules()` sets a breakpoint on
    `_dl_debug_state`, which the loader calls after mapping an object and
    before running its initializers, with a script callback that returns
    `False` so the stop never reaches the engine. That is what lets a library
    loaded later be placed while a breakpoint in it can still resolve.

A release build carries no wait library and is therefore only ever caught after
it has started, which is what a breakpoint has to allow for: a location reached
from `main()` resolves and is never hit, while a destructor or anything driven
from the event loop works.

## Debugging what was built on the device

`HarmonyOsBuildDeviceDebugWorkerFactory`, and the awkward case. A binary the
device just built cannot be debugged where it runs: the environment that
compiled it may not be traced at all, and Qt Creator may not execute a file it
wrote itself. Inside Qt Creator's own application both are allowed, so the
binary is hosted rather than executed.

`qtchost`, from Creator's own package, takes a gate port and a path. It maps
the binary with `QtcLoad::load()`, looks up `main`, reports
`pid <n> slide 0x<n>` over the gate, and then blocks on a `read()` of one byte
before calling it. Creator meanwhile starts `lldb-server platform --listen
*:8123` from the package, polls the port by connecting to it, attaches with
`continueAfterAttach(false)`, and feeds the debugger

    target modules add <path>
    target modules load --file <name> --slide <slide>

because neither the pid nor the slide is known before `qtchost` reports it. The
byte that releases `main()` is written on `RunControl::started`, which is when
breakpoints in the binary can actually be hit. The symbol file is the host path
rather than a device path, because the debugger runs where the build does and
not where the process is. A gate nothing listens on means an ordinary run, and
`qtchost` says so and runs the binary unheld.

## The loader

`qtcload.cpp` is what both the runner and `qtchost` rest on, and it exists
purely because of the exec and `dlopen` refusals. It reads the file, copies the
segments into anonymous writable memory, applies the relocations by hand
(including `DT_RELR` packing, which is newer than the device's musl `elf.h`),
and hands only the text pages to `mprotect` bracketed by the jit `prctl`.

Four things in it are not obvious and each was a failure first:

* **Dependencies are not loaded by it.** Symbols are resolved with `dlsym`
  against what the process already holds, which is the point: the runner
  already has Qt, so only the one library the user just built goes through this
  path. The `DT_NEEDED` entries that the platform will load are loaded by the
  real loader, and the handles are kept, because an application library that
  was itself `dlopen`ed `RTLD_LOCAL` keeps its whole dependency tree out of the
  global scope and then `RTLD_DEFAULT` finds not a single Qt symbol.
* **Exceptions.** The unwinder finds an ordinary library's frames by walking
  the loader's list of images, which this one is not in, so a throw would reach
  no handler and terminate. The frames are handed over one at a time to LLVM's
  unwinder registry, the way a JIT does. `framesRegistered` in the runner's own
  output is the check.
* **`main()` is often not in `.dynsym`.** An executable exports it only when
  linked with `-rdynamic`, but an unstripped build names it in `.symtab`, which
  is in the file and not in any segment, so the local symbols are kept
  separately.
* **Versioned sonames.** A package carries its libraries flat and unversioned,
  because installing a HAP unpacks no symbolic links, so what is there is
  `libQt6Core.so`. A Qt built on the device has an ordinary versioned soname,
  so an application built against it asks for `libQt6Core.so.6`, the same
  library under a name the packaged one does not have. Only the file name
  differs, so the request is answered with the packaged library.

## Where to look

Under `src/plugins/harmonyos/`:

| File | What is in it |
|---|---|
| `harmonyosrunconfiguration.cpp` | both run recipes, the channel, the extras |
| `harmonyosdebugsupport.cpp` | both debug recipes, the gate, the host gate |
| `harmonyosconstants.h` | the ports and the device paths, with the why |
| `harmonyosconfigurations.cpp` | the kit, and the debug link flags |
| `harmonyosdeploystep.cpp` | packaging, signing, installing, what ships when |

Under `share/qtcreator/`:

| File | What is in it |
|---|---|
| `harmonyos/qtcwait.cpp` | holds a debug build at startup |
| `harmonyos/qtcdebugplugin.cpp` | starts the server in-process |
| `harmonyos/qtcrunner.cpp` | takes the application's place |
| `harmonyos/qtchost.cpp` | hosts a binary built on the device |
| `harmonyos/qtcload.cpp` | the hand-rolled ELF loader |
| `debugger/lldbbridge.py` | `remote-ohos`, `placeMappedModules()` |
