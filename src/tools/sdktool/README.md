The SDK tool can be used to set up Qt versions, tool chains, devices and kits
in Qt Creator.

There still is a lot of knowledge about Qt Creator internals required
to use this tool!

Note that some tool chains/Qt versions/kits may require settings not
available via command line switches. All operations will add any parameter
followed by another parameter in the form `<TYPE>:<VALUE>` to the
configuration that they create.

Currently supported types are int, bool, QString, QByteArray and QVariantList.

Dependencies:
=============

The SDK tool depends only on Qt core library.

Usage:
======

```
./sdktool --help
Qt Creator SDK setup tool.
    Usage: sdktool <ARGS> <OPERATION> <OPERATION_ARGS>

ARGS:
    --help|-h                Print this help text
    --sdkpath=PATH|-s PATH   Set the path to the SDK files

OPERATION:
    One of:
        addKeys         add settings to Qt Creator configuration
        addCMake        add a CMake tool to Qt Creator
        addDebugger     add a debugger to Qt Creator
        addDev          add a device to Qt Creator
        addQt           add a Qt version to Qt Creator
        addTC           add a tool chain to Qt Creator
        addKit          add a kit to Qt Creator
        get             get settings from Qt Creator configuration
        rmCMake         remove a CMake tool from Qt Creator
        rmKit           remove a kit from Qt Creator
        rmDebugger      remove a debugger from Qt Creator
        rmDev           remove a device from Qt Creator
        rmKeys          remove settings from Qt Creator configuration
        rmQt            remove a Qt version from Qt Creator
        rmTC            remove a tool chain from Qt Creator
        findKey         find a key in the settings of Qt Creator
        find            find a value in the settings of Qt Creator

OPERATION_ARGS:
   use "--help <OPERATION>" to get help on the arguments required for an operation.
```

Add a tool chain:
================

```
./sdktool addTC \
    --id "ProjectExplorer.ToolChain.Gcc:company.product.toolchain.g++" \
    --language 2
    --name "GCC (C++, x86_64)" \
    --path /home/code/build/gcc-6.3/usr/bin/g++ \
    --abi x86-linux-generic-elf-64bit \
    --supportedAbis x86-linux-generic-elf-64bit,x86-linux-generic-elf-32bit \
    ADDITIONAL_INTEGER_PARAMETER int:42 \
    ADDITIONAL_STRING_PARAMETER "QString:some string" \
```

Tricky parts:
  - language is one of:
      * C
      * Cxx
  - `id` has to be in the form `ToolChainType:some_unique_part`, where the
    tool chain type can be one of the following, or other toolchains that plugins provide:
      * `ProjectExplorer.ToolChain.Msvc` for Microsoft MSVC compilers
        (Note: This one will be autodetected anyway, so there is little
         need to add it from the sdktool)
      * `ProjectExplorer.ToolChain.Gcc` for a normal GCC (linux/mac)
      * `ProjectExplorer.ToolChain.Clang` for the Clang compiler
      * `ProjectExplorer.ToolChain.LinuxIcc` for the LinuxICC compiler
      * `ProjectExplorer.ToolChain.Mingw` for the Mingw compiler
      * `ProjectExplorer.ToolChain.ClangCl` for the Clang/CL compiler
      * `ProjectExplorer.ToolChain.Custom` for custom toolchain
      * `Qt4ProjectManager.ToolChain.Android` for the Android tool chain
      * `Qnx.QccToolChain` for the Qnx QCC tool chain
      * `WebAssembly.ToolChain.Emscripten` for the Emscripten tool chain

    Check the classes derived from `ProjectExplorer::ToolChain` for their
    Ids.

    The `some_unique_part` can be anything. Qt Creator uses GUIDs by default,
    but any string is fine for the SDK to use.

  - `abi` needs to be in a format that `ProjectExplorer::Abi::fromString(...)`
    can parse.

Add a debugger:
===============


```
./sdktool addDebugger \
    --id "company.product.toolchain.gdb" \
    --name "GDB (company, product)" \
    --engine 1 \
    --binary /home/code/build/gdb-7.12/bin/gdb \
    --abis arm-linux-generic-elf-32 \
```

Tricky parts:
  - `id` can be any unique string. In Qt Creator this is set as the autodetection
    source of the Qt version.
    TODO: is it use in any special way?

  - `engine` is the integer used in the enum `Debugger::DebuggerEngineType`

    Currently these are (Qt Creator 4.6):
      * 1 for gdb
      * 4 for cdb
      * 8 for pdb
      * 256 for lldb

  - `binary` can be a absolute path, the value `auto` or an ABI.
    This is used to find the appropriate debugger for MSVC toolchains
    where Creator does not know the binary path itself.

Add a Qt version:
=================

```
./sdktool addQt \
    --id "company.product.qt" \
    --name "Custom Qt" \
    --qmake /home/code/build/qt-5.9/bin/qmake \
    --type Qt4ProjectManager.QtVersion.Desktop \
```

Tricky parts:
  - `id` can be any unique string. In Qt Creator this is set as the autodetection
    source of the Qt version.

  - `type` must be the string returned by `BaseQtVersion::type()`.

    Currently these are (Qt Creator 4.11):
      * `Qt4ProjectManager.QtVersion.Android` for Android
      * `Qt4ProjectManager.QtVersion.Desktop` for a desktop Qt
      * `Qt4ProjectManager.QtVersion.Ios` for iOS
      * `Qt4ProjectManager.QtVersion.QNX.QNX` for QNX
      * `RemoteLinux.EmbeddedLinuxQt` for Embedded Linux
      * `WinRt.QtVersion.WindowsRuntime` for Windows RT
      * `WinRt.QtVersion.WindowsPhone` for Windows RT phone
      * `Qt4ProjectManager.QtVersion.WebAssembly` for WebAssembly

Add a kit:
==========

Using the newly set up tool chain and Qt version:

```
./sdktool addKit \
    --id "company.product.kit" \
    --name "Qt %{Qt:Version} (company, product)" \
    --debuggerid "company.product.toolchain.gdb" \
    --devicetype GenericLinuxOsType \
    --sysroot /tmp/sysroot \
    --Ctoolchain "ProjectExplorer.ToolChain.Gcc:company.product.toolchain.gcc" \
    --Cxxtoolchain "ProjectExplorer.ToolChain.Gcc:company.product.toolchain.g++" \
    --qt "company.product.qt" \
    --mkspec "devices/linux-mipsel-broadcom-97425-g++" \
```

Tricky parts:
  - `devicetype` is the string returned IDevice::type()

    Currently these are (Qt Creator 4.11):
      * `Android.Device.Type` for Android devices
      * `Desktop` for code running on the local desktop
      * `Ios.Device.Type` for an iOS device
      * `Ios.Simulator.Type` for an iOS simulator
      * `GenericLinuxOsType` for an embedded Linux device
      * `WinRt.Device.Local` for Windows RT (local)
      * `WinRt.Device.Emulator` for a Windows RT emulator
      * `WinRt.Device.Phone` for a Windows RT phone
      * `WebAssemblyDeviceType` for a web browser that supports WebAssembly

  - `debuggerid` is one of the ids used when setting up toolchains with
    `sdktool addDebugger`.

  - `<Lang>toolchain` is one of the ids used when setting up toolchains with
    `sdktool addTC`.

  - `<Lang>` is one of (options can be extended by plugins):
      * C, Cxx, Nim

  - `qt` is one of the ids used when setting up Qt versions with `sdktool addQt`.


Add a device:
=============

TODO

Add a CMake:
============

TODO
