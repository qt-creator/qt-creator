Limitations:

- Linux, Mac, Windows hosts are supported in principle,
  Linux is recommended.
- Only Kit items in selected directories are auto-detected.
- Kits themselves need to be fixed up manually.
- Shared mounts are restricted to locations on the host system
  that can end up on the same absolute location in the container
- Windows host: Mounted drives cannot be used as shared mounts

What works:

- Qmake in path is found
- CMake in path is found
- Toolchain autodection finds gcc and clang
- Gdb and lldb in path are found

- Building in the container with cmake works

- Running locally or in a compatible docker container works


For testing:

- Optional: Set QT_LOGGING_RULES=qtc.docker.device=true
  This will show a large part of the communication with the docker CLI client.

- Build docker containers from this directory (tests/manual/docker) by
  running ./build.sh. This builds a docker image containing a Desktop Qt
  build setup (including compiler etc) and second docker image container
  containing a run environment without the build tools, but e.g. with gdb
  for debugger testing and a third containing clang and lldb

    - or -

  install similar docker images containing Qt, e.g.  darkmattercoder/qt-build

- Go to Edit -> Preferences -> Devices, 'Add', 'Apply' for both images.
  Note that the Build container alone is sufficient also to run applications,
  but using the Run container gives a more restricted setup closer to a
  real world scenario.

- MAKE SURE there's something sensible in "Paths to Mount".
  These paths are shared between your host system and the docker device.
  These should contain at the very least your sources, otherwise a build
  in the container can't access it.

- Try to auto-detect kit items by pressing "Auto Detect Kit Items" for
  the Build container (only Build, not Run)

- Check whether the auto-detection of kit items works, i.e. this Qt version
  shows up in Kits -> Qt Version, Compilers, CMake, Debugger.

- Fix the Kit setup: There should also be an auto-detected Kit, not
  necessarily with all items in a suitable state.
  Select as Run device the Run container, as Build device the Build container,
  and matching auto-detected compilers, cmake, gdb.

- Create a CMake based Qt (console or widget) application, build / run / debug it.

