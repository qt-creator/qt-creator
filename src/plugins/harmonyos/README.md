# HarmonyOS Plugin for Qt Creator

The plugin builds, deploys, runs and debugs Qt applications on a HarmonyOS
device. It drives the HarmonyOS command-line tools and talks to the device with
`hdc`. The SDK detection also looks where DevEco Studio keeps its copy, but
DevEco Studio is not available for Linux, so none of this was tried with it.

It covers two setups, and a statement about one is not a statement about the
other:

- **From a host.** Qt Creator runs on a desktop machine, a Linux one in what
  follows, and the HarmonyOS device is the target it builds for and deploys
  to.
- **On the device.** Qt Creator itself is a package installed on the device,
  and builds and runs projects there with a toolchain it carries in its own
  package.

Everything below was measured on a consumer HAD-W32 (OpenHarmony 6.1.0.115,
API 23) with the command-line tools 6.1.0.105 (API 23) and Qt 6.12 for
HarmonyOS, both the installed one and one built from source, from an x86_64
Linux host. Older Qt versions are not supported: the Qt online installer
publishes Qt for HarmonyOS from 6.12 on.

The three lists that follow are deliberately kept apart: what was watched
happening, what is in place but was never watched, and what cannot be done.
Reading the second list as the first is how one ends up debugging the wrong
thing.

## Seen working

### From a host

- **SDK, toolchain, Qt versions, kits.** With the SDK location set in
  Preferences > SDKs > HarmonyOS, Clang toolchains appear for aarch64, armv7
  and x86_64, a Qt for HarmonyOS is recognized by its `ohos` mkspec, and a kit
  for it comes out valid and without complaints.
- **Devices.** `hdc list targets` finds the connected device, the device page
  reports it as ready, and its device test finishes successfully.
- **File access.** Reading and writing files on the device goes through `hdc`,
  with the transfer standing in where `hdc shell` cannot help. The test for it
  passes against the tablet, apart from the two cases that need an SSH build
  device.
- **Getting the SDK and Qt from the settings page.** The page downloads and
  unpacks the OpenHarmony SDK (2.5 GB), and installs Qt for HarmonyOS from a Qt
  repository, which is the only route for Qt Creator on the device. The Qt
  comes out registered with its HarmonyOS version. The SDK downloaded this way
  has no `hvigor`, so packaging still needs the one from DevEco Studio or the
  command line tools.
- **Deploying and installing.** A deploy produced a signed package and put it
  on the device. That the debug session below then ran is also what says the
  debug plugin and the `lldb-server` really were in that package. A deploy
  whose package content did not change skips the packaging and the install,
  which took a deploy from 21 seconds to 9.
- **Running.** The ability is started with `aa start`, the run stays alive for
  as long as the application does by following its `hilog` output, and stopping
  it force-stops the bundle.
- **Running without installing.** With "Run without installing" set on the
  deploy step, the package holds a runner instead of the application. It is
  installed once, carries no Qt, and stays the same whatever the application
  and whatever Qt it is built with. Each run hands the application library and
  the Qt it needs over a channel on the device, and the runner maps them
  itself. Watched with a Qt Widgets project and a Qt 6.12 kit: build, package,
  sign, install and run, with the application's window on the tablet. The
  first run of a Qt copies it to the device before the start (19 files, 512 MB
  in about 10 seconds), and a later run of the same Qt skips that.
- **Debugging through the runner.** The runner starts the debug server, reports
  where it mapped each library, and holds `main()` back. The platform lets
  nobody write into text it made executable, so the libraries are placed at
  those addresses, and the runner is let go from a stop in `qtcHostWaiting`,
  which it calls while it holds. Watched stopping in a `MainWindow`
  constructor, with `main()` of the application and of the runner above it.
  It is slow: the stop came 3.5 minutes after the start, most of it spent at
  the loader's stops, where the memory map is read again each time.
- **Debugging.** The application starts the debug server itself, Qt Creator
  attaches through the `remote-ohos` platform, and the debugger stops.
- **Breakpoints.** A breakpoint set by function or by file and line resolves,
  in the application's own library as well as in Qt's, and is hit. It was
  watched with one in a widget destructor as the application was closed, and
  with one on the first line of `main()`.
- **Stopping before `main()`.** A debug build links against a small library of
  ours, which the loader initializes before the library that names it as a
  dependency, so before any static initializer of the application. It asks
  whether this launch is being debugged, starts the debug server, and then
  spins on an exported byte until the debugger clears it. What it asks is the
  device's own loopback: Qt Creator leaves a listener there through a reverse
  forward for as long as a debug run lasts, because nothing else reaches an
  application that early. The framework passes no environment, the parameter
  store is root-only, and a file the host writes cannot be read from the
  application sandbox. Watched stopping at the first line of `main()`, with
  the frames above it in Qt's HarmonyOS entry point. Nothing in the library
  comes from Qt, so a program that is not a Qt one is held the same way.
- **Symbolized frames**, for the libraries there is a local copy of. The
  platform reports no module list for a process it did not launch, so nothing
  would be loaded, and no frame could be named and no breakpoint placed. What
  fixes it is `placeMappedModules()` in the `remote-ohos` branch of
  `lldbbridge.py`: the memory regions of the process say which file is mapped
  where, so the lowest mapping of a shared object is taken as its load address
  and the copy on this side is placed there. The search paths it looks in are
  the ones the run already passes as `solibSearchPath`.

  The libraries of a HarmonyOS application land at an address that changes
  with every launch, which is why the addresses have to be read at attach time
  rather than assumed. Frames in the system libraries (the musl loader,
  `appspawn`, the OHOS framework) stay nameless, because there is no local
  copy of those to place.
- **Packaging Qt Creator itself.** Qt Creator's own sources, opened with a
  HarmonyOS kit, build, deploy and run on the device. A project declares what
  else belongs in its package in a `<target>-harmonyos-extras.json` file its
  build writes: resource directories, launch arguments, files that have to be
  executable. With `"device-toolchain": true` in it, the deploy step also packs
  the compiler, CMake, ninja and lldb from the device-hosted SDK into a native
  package (about 145 MB), and skips that again while nothing changed.

### On the device

- **Kit.** Qt Creator detects the device it runs on as a build device, finds
  the toolchain, CMake, ninja and lldb its package carries, and creates a kit
  with that device as build and run device. Nothing has to be configured by
  hand apart from registering a Qt.
- **Building.** A CMake Qt Widgets project in the user's own storage
  configures and builds with that kit.
- **Running.** A run goes through the same runner as "Running without
  installing" above, which has to be installed on the device. There is no
  `hdc` on the device, so the first run of a Qt receives it over the channel
  (512 MB in about 1 second), ends, and is started again. The application's
  `main()` was reached under a second after that, and its window shown.
- **Debugging.** Qt Creator may not execute a file it wrote, so `qtchost` in
  its package maps the binary, reports where, and waits in `qtcHostWaiting`
  until the debugger has placed it and lets it go. Watched stopping at a
  breakpoint in `main()` of a project built on the device, 4 seconds after the
  start, with `main()` of `qtchost` above it.

## Implemented but not confirmed

Written and built, but never yet watched doing its job from within Qt Creator:

- **Installing Qt through the maintenance tool.** Where there is one, the
  settings page installs Qt for HarmonyOS through it rather than from a Qt
  repository. Not run, because it changes the installation on the machine.

## What it needs

- A provisioning profile for the application, set as "Provisioning profile" in
  Preferences > SDKs > HarmonyOS. The device installs a package only under the
  bundle name the profile is issued for, and only such a package may be
  debugged, so the deploy step writes that bundle name into the package. The
  restricted permissions the application asks for have to be allowed by the
  profile, and Qt already asks for `ohos.permission.SYSTEM_FLOAT_WINDOW`. The
  deploy step lists in the Issues pane what the profile lacks.
- A Java runtime, which the signing tool and hvigor need, at most Java 23 (see
  below). Qt Creator puts the one `JAVA_HOME` names first.
- For Qt Creator on the device: the SDK Huawei publishes as "native-ohos-x64"
  (its payload is for arm64 despite the name) set as the device-hosted SDK
  when Qt Creator's own package is built.

## Not possible

- **Launching under the debugger.** The device allows `PTRACE_ATTACH` from
  inside an installed application, and refuses `PTRACE_TRACEME` everywhere. So
  the application starts the server and the debugger attaches, rather than the
  server starting the application. This is a property of the device, not
  something left to do. Stopping before the application's own code runs is
  nevertheless possible, by holding it from inside (see "Stopping before
  `main()`" above).

  A release build has no such library and is therefore only ever caught after
  it has started, which is where a breakpoint has to allow for: a location
  reached from `main()` resolves and is never hit, while a destructor or
  anything driven from the event loop works.
- **Loading what an application wrote.** The device refuses to `dlopen` or
  `execve` a file that was not installed with a package, whatever its location,
  mode or signature. That is why the runner maps the application and its Qt
  itself, and registers Qt's platform plugin by hand instead of letting Qt load
  it.
- **The runner's own limits.** It runs the application under the runner's
  identity, so anything that depends on the application's own manifest still
  has to be checked with an installed package. From the device a run cannot
  stop the application (Stop closes the channel and leaves it running), a run
  while an earlier one is still up reaches that one instead of starting anew,
  and no exit status comes back. A run from a host and one from the device
  cannot use the same device at the same time, because both want the same
  port on it.

## What the SDK makes awkward

Worked around here, but worth knowing when reading the deploy step:

- `hvigor` 6.0.2 deletes the generated project directly after its `PackageHap`
  task, and would take the finished package with it, so the package is copied
  out before the next run replaces the project. The 6.1 tools leave the project
  alone. Copying the package out costs nothing there and keeps both usable.
- The packing tool deletes the whole generated project as well when `hvigor`
  runs it on Java 24 or later, where `new File("")` came to mean the working
  directory. The next task then fails with "File is not exist" on
  `entry/oh-package.json5`. An `hvigor` daemon keeps the Java it was started
  with.
- `hvigor` signs only with material that DevEco Studio manages, so the package
  is signed here instead, with the `hap-sign-tool` of the SDK.
- `harmonydeployqt` stages the third-party runtime dependencies only if it is
  told where they are, and `qt.toolchain.cmake` points it at the directory of
  the machine Qt was built on, which exists for a Qt built here and not for an
  installed one. The kit therefore carries the configured location as
  `QT_ADDITIONAL_PACKAGES_PREFIX_PATH`. The debug server it stages in no case,
  so that one is put into the package afterwards. Nor does it follow what a
  plugin it stages needs in turn (the JPEG one wants `libjpeg.so`), so the
  deploy step reports by name what no directory supplies.
- The `ohos` mkspec stops with an error unless `NATIVE_OHOS_SDK` is set in the
  environment, which Qt Creator does not set, so the mkspec cannot be evaluated
  and says nothing about the platform. A Qt version is therefore recognized by
  the name of its mkspec. The Qt online installer registers Qt for HarmonyOS
  as a desktop Qt (QTCREATORBUG-34793), and with no ABI, so such a Qt gets its
  HarmonyOS kit by its mkspec too, with the ABI read off `libQt6Core.so`.
- `hdc` writes a log file for every invocation into `TMPDIR`, and the device
  detection runs it every five seconds, so Qt Creator points it at a directory
  that goes away with the session.
- A native package declared "public" shares one namespace on the device with
  every other one, so two applications that each carry an `lldb-server` could
  not both be installed. The packages built here are "private".
