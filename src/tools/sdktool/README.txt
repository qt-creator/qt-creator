The SDK tool can be used to set up Qt versions, tool chains and kits
in Qt Creator.

There still is a lot of knowledge about Qt Creator internals required
to use this tool!

Note that some tool chains/Qt versions/kits may require settings not
available via command line switches. All operations will add any parameter
followed by another parameter in the form "<TYPE>:<VALUE>" to the
configuration that they create.

Currently supported types are int, bool, QString, QByteArray and QVariantList.

Dependencies:
The SDK tool depends on the Utils library of Qt Creator and Qt itself.
Please make sure both libraries are found by setting PATH/LD_LIBRARY_PATH.

Usage:
Qt Creator SDK setup tool.
    Usage:./sdktool <ARGS> <OPERATION> <OPERATION_ARGS>

ARGS:
    --help|-h                Print this help text
    --sdkpath=PATH|-s PATH   Set the path to the SDK files

OPERATION:
    One of:
        addKeys         add settings to Qt Creator configuration
        addKit          add a Kit to Qt Creator
        addQt           add a Qt version to Qt Creator
        addTC           add a tool chain to Qt Creator
        findKey         find a key in the settings of Qt Creator
        find            find a value in the settings of Qt Creator
        get             get settings from Qt Creator configuration
        rmKeys          remove settings from Qt Creator configuration
        rmKit           remove a Kit from Qt Creator
        rmQt            remove a Qt version from Qt Creator
        rmTC            remove a tool chain from Qt Creator

OPERATION_ARGS:
   use "--help <OPERATION>" to get help on the arguments required for an operation.


Help for individual operations can be retrieved using

    ./sdktool --help addKit

Examples:

Add a tool chain:
./sdktool addTC \
    --id "ProjectExplorer.ToolChain.Gcc:my_test_TC" \
    --name "Test TC" \
    --path /usr/bin/g++ \
    --abi x86-linux-generic-elf-64bit \
    --supportedAbis x86-linux-generic-elf-64bit,x86-linux-generic-elf-32bit \
    ADDITIONAL_INTEGER_PARAMETER int:42 \
    ADDITIONAL_STRING_PARAMETER "QString:some string" \

Tricky parts:
  - The id has to be in the form "ToolChainType:some_unique_part", where the
    tool chain type must be one of (as of Qt Creator 2.6):
      * ProjectExplorer.ToolChain.Msvc for Microsoft MSVC compilers
        (Note: This one will be autodetected anyway, so there is little
         need to add it from the sdktool)
      * ProjectExplorer.ToolChain.WinCE for Windows CE tool chain by
        Microsoft (Note: This one will be autodetected anyway, so there
        is little need to add it from the sdktool)
      * ProjectExplorer.ToolChain.Gcc for a normal GCC (linux/mac)
      * Qt4ProjectManager.ToolChain.Android for the Android tool chain
      * ProjectExplorer.ToolChain.Clang for the Clang compiler
      * ProjectExplorer.ToolChain.LinuxIcc for the LinuxICC compiler
      * ProjectExplorer.ToolChain.Mingw for the Mingw compiler

    Check the classes derived from ProjectExplorer::ToolChain for their
    Ids.

    The some_unique_part can be anything. Creator uses GUIDs by default,
    but any string is fine for the SDK to use.

  - The ABI needs to be in a format that ProjectExplorer::Abi::fromString(...)
    can parse.

Add a Qt version:
./sdktool addQt \
    --id "my_test_Qt" \
    --name "Test Qt" \
    --qmake /home/code/build/qt4-4/bin/qmake \
    --type Qt4ProjectManager.QtVersion.Desktop \

Tricky parts:
  - The id can be any unique string. In creator this is set as the autodetection
    source of the Qt version.

  - type must be the string returned by BaseQtVersion::type().

    Currently these are (Qt Creator 2.6):
      * Qt4ProjectManager.QtVersion.Android for Android
      * Qt4ProjectManager.QtVersion.Desktop for a desktop Qt
      * RemoteLinux.EmbeddedLinuxQt for an embedded linux Qt
      * Qt4ProjectManager.QtVersion.Maemo for an Maemo Qt
      * Qt4ProjectManager.QtVersion.QNX.BlackBerry for Qt on BlackBerry
      * Qt4ProjectManager.QtVersion.QNX.QNX for Qt on QNX
      * Qt4ProjectManager.QtVersion.Simulator for Qt running in the Qt simulator
      * Qt4ProjectManager.QtVersion.WinCE for Qt on WinCE

Add a Kit using the newly set up tool chain and Qt version:
./sdktool addKit \
    --id "my_test_kit" \
    --name "Test Kit" \
    --debuggerengine 1 \
    --debugger /tmp/gdb-test \
    --devicetype Desktop \
    --sysroot /tmp/sysroot \
    --toolchain "ProjectExplorer.ToolChain.Gcc:my_test_TC" \
    --qt "my_test_Qt" \
    --mkspec "testspec" \

Tricky parts:
  - debuggerengine is the integer used in the enum Debugger::DebuggerEngineType
    The most important type is 1 for GDB.
  - debugger can be a absolute path or the value: 'auto'

  - devicetype is the string returned IDevice::type()

    Currently these are (Qt Creator 2.6):
      * Android.Device.Type for Android devices
      * Desktop for code running on the local desktop
      * BBOsType for BlackBerry devices
      * HarmattanOsType for N9/N950 devices based on Harmattan
      * Maemo5OsType for N900 devices based on Maemo

  - toolchain is one of the ids used when setting up toolchains with sdktool addTC.

  - qt is one of the ids used when setting up Qt versions with sdktool addQt.

