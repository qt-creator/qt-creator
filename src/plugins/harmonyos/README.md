# HarmonyOS Plugin for Qt Creator

The plugin builds, deploys, runs and debugs Qt applications on a HarmonyOS
device. It drives the HarmonyOS command-line tools and talks to the device with
`hdc`. The SDK detection also looks where DevEco Studio keeps its copy, but
DevEco Studio is not available for Linux, so none of this was tried with it.

All of it is cross-development: Qt Creator runs on a desktop host, a Linux one
in what follows, and the HarmonyOS machine is the target it builds for and
deploys to. Running Qt Creator on HarmonyOS itself, and developing there for
the machine it runs on, is what this is meant to lead to; nothing below covers
that yet, so do not read a statement about the cross case as one about the
native one.

Everything below was measured on a consumer HAD-W32 (OpenHarmony 6.1.0.115,
API 23) with the command-line tools 6.1.0.105 (API 23) and Qt 6.12 for
HarmonyOS, both the installed one and one built from source, from an x86_64
Linux host.

The three lists that follow are deliberately kept apart: what was watched
happening, what is in place but was never watched, and what cannot be done.
Reading the second list as the first is how one ends up debugging the wrong
thing.

## Seen working

- **SDK, toolchain, Qt versions, kits.** With the SDK location set in
  Preferences > SDKs > HarmonyOS, Clang toolchains appear for aarch64, armv7
  and x86_64, a Qt for HarmonyOS is recognized by its `ohos` mkspec, and a kit
  for it comes out valid and without complaints.
- **Devices.** `hdc list targets` finds the connected device, the device page
  reports it as ready, and its device test finishes successfully.
- **Deploying and installing.** A deploy produced a signed package and put it
  on the device. That the debug session below then ran is also what says the
  debug plugin and the `lldb-server` really were in that package.
- **Running.** The ability is started with `aa start`, the run stays alive for
  as long as the application does by following its `hilog` output, and stopping
  it force-stops the bundle.
- **Debugging.** The application starts the debug server itself, Qt Creator
  attaches through the `remote-ohos` platform, and the debugger stops.
- **Breakpoints.** A breakpoint set by function or by file and line resolves,
  in the application's own library as well as in Qt's, and is hit - watched
  with one in a widget destructor as the application was closed, and with one
  on the first line of `main()`.
- **Stopping before `main()`.** A debug build links against a small library of
  ours, which the loader initializes before the library that names it as a
  dependency - so before any static initializer of the application. It asks
  whether this launch is being debugged, starts the debug server, and then
  spins on an exported byte until the debugger clears it. What it asks is the
  device's own loopback: Qt Creator leaves a listener there through a reverse
  forward for as long as a debug run lasts, because nothing else reaches an
  application that early - the framework passes no environment, the parameter
  store is root-only, and a file the host writes cannot be read from the
  application sandbox. Watched stopping at the first line of `main()`, with
  the frames above it in Qt's HarmonyOS entry point. Nothing in the library
  comes from Qt, so a program that is not a Qt one is held the same way.
- **Symbolised frames**, for the libraries there is a local copy of. The
  platform reports no module list for a process it did not launch, so nothing
  is loaded and, before this was dealt with, no frame could be named and no
  breakpoint could be placed. What fixes it is `placeMappedModules()` in the
  `remote-ohos` branch of `lldbbridge.py`: the memory regions of the process
  do say which file is mapped where, so the lowest mapping of a shared object
  is taken as its load address and the copy on this side is placed there. The
  search paths it looks in are the ones the run already passes as
  `solibSearchPath`.

  The libraries of a HarmonyOS application land at an address that changes
  with every launch, which is why the addresses have to be read at attach time
  rather than assumed. Frames in the system libraries - the musl loader,
  `appspawn`, the OHOS framework - stay nameless, because there is no local
  copy of those to place.

## Implemented but not confirmed

Written and built, with the pieces they rest on checked by hand against the
device, but never yet watched doing their job from within Qt Creator:

- **Building on the device.** The device is offered as a build device and its
  build tools are detected, and the command bridge is signed on its way there
  because the device refuses an unsigned binary. Signing a binary for this
  device was tried by hand; this path through Qt Creator was not.
- **File access.** Reading and writing files on the device goes through `hdc`,
  with the transfer standing in where `hdc shell` cannot help. There is a test
  for it that needs an attached device.

## What it needs

- A provisioning profile for the application, set as "Provisioning profile" in
  Preferences > SDKs > HarmonyOS. The device installs a package only under the
  bundle name the profile is issued for, and only such a package may be
  debugged, so the deploy step writes that bundle name into the package.
- A Java runtime, which the signing tool of the SDK needs.

## Not possible

- **Launching under the debugger.** The device allows `PTRACE_ATTACH` from
  inside an installed application, and refuses `PTRACE_TRACEME` everywhere. So
  the application starts the server and the debugger attaches, rather than the
  server starting the application. This is a property of the device, not
  something left to do. Stopping before the application's own code runs is
  nevertheless possible, by holding it from inside - see "Stopping before
  `main()`" above.

  A release build has no such library and is therefore only ever caught after
  it has started, which is where a breakpoint has to allow for: a location
  reached from `main()` resolves and is never hit, while a destructor or
  anything driven from the event loop works.

## What the SDK makes awkward

Worked around here, but worth knowing when reading the deploy step:

- `hvigor` 6.0.2 deletes the generated project directly after its `PackageHap`
  task, and would take the finished package with it, so the package is copied
  out before the next run replaces the project. The 6.1 tools leave the project
  alone; copying the package out costs nothing there and keeps both usable.
- `hvigor` signs only with material that DevEco Studio manages, so the package
  is signed here instead, with the `hap-sign-tool` of the SDK.
- `harmonydeployqt` stages the third-party runtime dependencies only if it is
  told where they are, and `qt.toolchain.cmake` points it at the directory of
  the machine Qt was built on - which exists for a Qt built here and not for an
  installed one. The kit therefore carries the configured location as
  `QT_ADDITIONAL_PACKAGES_PREFIX_PATH`. The debug server it stages in no case,
  so that one is put into the package afterwards.
- The `ohos` mkspec stops with an error unless `NATIVE_OHOS_SDK` is set in the
  environment, which Qt Creator does not set, so the mkspec cannot be evaluated
  and says nothing about the platform. A Qt version is therefore recognized by
  the name of its mkspec. One that was registered earlier, as a desktop Qt,
  stays that way, and its kit keeps warning that the Qt version does not
  support the device type even though building and running work.
