# Qt Creator

Qt Creator is a cross-platform, integrated development environment (IDE) for
application developers to create applications for multiple desktop, embedded,
and mobile device platforms.

The Qt Creator Manual is available at:

https://doc.qt.io/qtcreator/index.html

For an overview of the Qt Creator IDE, see:

https://doc.qt.io/qtcreator/creator-overview.html

## Supported Platforms

The standalone binary packages support the following platforms:

* Windows 10 (x86_64) or later
* Windows 11 (ARM64) or later
* (K)Ubuntu Linux 22.04 (x86_64) or later
* (K)Ubuntu Linux 24.04 (arm64) or later
* macOS 12 or later

When you compile Qt Creator yourself, the Qt version that you build with
determines the supported platforms.

## Contributing

For instructions on how to set up the Qt Creator repository to contribute
patches back to Qt Creator, please check:

https://wiki.qt.io/Setting_up_Gerrit

See the following page for information about our coding standard:

https://doc.qt.io/qtcreator-extending/coding-style.html

## Compiling Qt Creator

Prerequisites:

* Qt 6.5.3 or later. The Qt version that you use to build Qt Creator defines the
  minimum platform versions that the result supports
  (Windows 10, RHEL/CentOS 8.8, Ubuntu 22.04, macOS 11 for Qt 6.5.3).
* Qt WebEngine module for QtWebEngine based help viewer
* On Windows:
    * MinGW with GCC 11.2 or Visual Studio 2019 or later
    * Python 3.8 or later (optional, needed for the python enabled debug helper)
    * Debugging Tools for Windows (optional, for MSVC debugging support with CDB)
* On macOS: latest Xcode
* On Linux: GCC 10 or later
* LLVM/Clang 14 or later (optional, LLVM/Clang 17 is recommended.
  See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
  get LLVM.
  The ClangFormat plugin uses the LLVM C++ API.
  Since the LLVM C++ API provides no compatibility guarantee,
  if later versions don't compile we don't support that version.)
* CMake
* Ninja (recommended)

The used toolchain has to be compatible with the one Qt was compiled with.

### Getting Qt Creator from Git

The official mirror of the Qt Creator repository is located at
https://code.qt.io/cgit/qt-creator/qt-creator.git/. Run

    git clone https://code.qt.io/qt-creator/qt-creator.git

to clone the Qt Creator sources from there. This creates a checkout of the
Qt Creator sources in the `qt-creator/` directory of your current working
directory.

Qt Creator relies on some submodules, like
[litehtml](https://github.com/litehtml) for displaying documentation. Get these
submodules with

    cd qt-creator  # switch to the sources, if you just ran git clone
    git submodule update --init --recursive

Note the `--recursive` in this command, which fetches also submodules within
submodules, and is necessary to get all the sources.

The git history contains some coding style cleanup commits, which you might
want to exclude for example when running `git blame`. Do this by running

    git config blame.ignoreRevsFile .gitignore-blame

### Linux and macOS

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `/path/to/qtcreator_sources`, Qt is installed in
`/path/to/Qt`, and LLVM is installed in `/path/to/llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

Note that `/path/to/Qt` doesn't imply the full path depth like:
`$USER/Qt/6.4.3/gcc_64/lib/cmake/Qt6`, but only `$USER/Qt/6.4.3/gcc_64`.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

    mkdir qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" /path/to/qtcreator_sources
    cmake --build .

#### Troubleshooting: libxcb plugin not found while using Qt libraries built locally from source

Ensure all prerequisites for building Qt are installed:
https://doc.qt.io/qt-6/linux.html
https://doc.qt.io/qt-6/linux-requirements.html

If they were installed before building Qt and xcb plugin is missing try reinstall them with

```sh
    sudo apt-get --reinstall <package_name>
```

Reset building configuration for Qt libraries at '/path/to/qt_sources'

```sh
    cmake --build . --target=clean
```

and remove CMakeCache.txt

```sh
    rm CMakeCache.txt
```

Try building Qt source again.


### Windows

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `\path\to\qtcreator_sources`, Qt is installed in
`\path\to\Qt`, and LLVM is installed in `\path\to\llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

Note that `\path\to\Qt` doesn't imply the full path depth like:
`c:\Qt\6.4.3\msvc2019_64\lib\cmake\Qt6`, but only `c:/Qt/6.4.3/msvc2019_64`.
The usage of slashes `/` is intentional, since CMake has issues with backslashes `\`
in `CMAKE_PREFX_PATH`, they are interpreted as escape codes.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

Decide which compiler to use: MinGW or Microsoft Visual Studio.

MinGW is available via the Qt online installer, for other options see
<https://wiki.qt.io/MinGW>. Run the commands below in a shell prompt that has
`<path_to_mingw>\bin` in the `PATH`.

For Microsoft Visual C++ you can use the "Build Tools for Visual Studio". Also
install the "Debugging Tools for Windows" from the Windows SDK installer. We
strongly recommend using the 64-bit version and 64-bit compilers on 64-bit
systems. Open the `x64 Native Tools Command Prompt for VS <version>` from the
start menu items that were created for Visual Studio, and run the commands
below in it.

    md qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" \path\to\qtcreator_sources
    cmake --build .

Qt Creator can be registered as a post-mortem debugger. This can be done in the
options page or by running the tool qtcdebugger with administrative privileges
passing the command line options -register/unregister, respectively.
Alternatively, the required registry entries

    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AeDebug
    HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\AeDebug

can be modified using the registry editor regedt32 to contain

    qtcreator_build\bin\qtcdebugger %ld %ld

When using a self-built version of Qt Creator as post-mortem debugger, it needs
to be able to find all dependent Qt-libraries and plugins when being launched
by the system. The easiest way to do this is to create a self-contained Qt
Creator by installing it and installing its dependencies. See "Options" below
for details.

Note that unlike on Unix, you cannot overwrite executables that are running.
Thus, if you want to work on Qt Creator using Qt Creator, you need a separate
installation of it. We recommend using a separate, release-built version of Qt
Creator to work on a debug-built version of Qt Creator.

Alternatively, take the following template of `CMakeUserPresets.json` for
reference. Write your own configurePreset inheriting `cmake-plugin-minimal` in
`CMakeUserPresets.json` to build with IDEs (such as QtCreator, VSCode,
CLion...etc) locally:

```json
{
  "version": 4,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 23,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "custom",
      "displayName": "custom",
      "description": "custom",
      "inherits": "cmake-plugin-minimal",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "toolset": {
        "value": "v142,host=x64",
        "strategy": "external"
      },
      "architecture": {
        "value": "x64",
        "strategy": "external"
      },
      "cacheVariables": {
        "CMAKE_CXX_COMPILER": "cl.exe",
        "CMAKE_C_COMPILER": "cl.exe",
        "CMAKE_PREFIX_PATH": "c:/Qt/6.4.3/msvc2019_64"
      }
    }
  ]
}
```

### Options

If you do not have Ninja installed and in the `PATH`, remove `-G Ninja` from
the first `cmake` call. If you want to build in release mode, change the build
type to `-DCMAKE_BUILD_TYPE=Release`. You can also build with release
optimizations but debug information with `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

You can find more options in the generated CMakeCache.txt file. For instance,
building of Qbs together with Qt Creator can be enabled with `-DBUILD_QBS=ON`.

Installation is not needed. You can run Qt Creator directly from the build
directory. On Windows, make sure that your `PATH` environment variable points to
all required DLLs, like Qt and LLVM. On Linux and macOS, the build already
contains the necessary `RPATH`s for the dependencies.

If you want to install Qt Creator anyway, that is however possible using

    cmake --install . --prefix /path/to/qtcreator_install

To create a self-contained Qt Creator installation, including all dependencies
like Qt and LLVM, additionally run

    cmake --install . --prefix /path/to/qtcreator_install --component Dependencies

To install development files like headers, CMake files, and `.lib` files on
Windows, run

    cmake --install . --prefix /path/to/qtcreator_install --component Devel

If you used the `RelWithDebInfo` configuration and want debug information to be
available to the installed Qt Creator, run

    cmake --install . --prefix /path/to/qtcreator_install --component DebugInfo

### QML Designer

To disable the build of the experimental QML Designer plugins and their dependencies,
use the CMake option `-DWITH_QMLDESIGNER=OFF`. The QML Designer plugin requires additional
testing for specific Qt versions. If that can not be provided we suggest to disable it.

### Perf Profiler Support

Support for the [perf](https://perf.wiki.kernel.org/index.php/Main_Page) profiler
requires the `perfparser` tool that is part of the Qt Creator source package, and also
part of the Qt Creator Git repository in form of a submodule in `src/tools/perfparser`.

Compilation of `perfparser` requires ELF and DWARF development packages.
You can either download and extract a prebuilt package from
https://download.qt.io/development_releases/prebuilt/elfutils/ and add the
directory to the `CMAKE_PREFIX_PATH` when configuring Qt Creator,
or install the `libdw-dev` package on Debian-style Linux systems.

You can also point Qt Creator to a separate installation of `perfparser` by
setting the `PERFPROFILER_PARSER_FILEPATH` environment variable to the full
path to the executable.

### Partial building of executables and plugins

Set the following CMake definitions in order to configure and build only
parts of Qt Creator. Note that dependencies are not automatically handled.
```
-DBUILD_EXECUTABLES:STRING=QtCreator;ClangBackend;qtcreator_processlauncher
```
```
-DBUILD_PLUGINS:STRING=Core;TextEditor;ProjectExplorer;CppTools;CppEditor;QmakeProjectManager;CMakeProjectManager;Debugger;ResourceEditor;QtSupport;LanguageClient
```
## Getting LLVM/Clang for the Clang Code Model

The Clang code model uses `Clangd` and the ClangFormat plugin depends on the
LLVM/Clang libraries. The currently recommended LLVM/Clang version is 14.0.

### Prebuilt LLVM/Clang packages

Prebuilt packages of LLVM/Clang can be downloaded from
https://download.qt.io/development_releases/prebuilt/libclang/

This should be your preferred option because you will use the version that is
shipped together with Qt Creator (with backported/additional patches). In
addition, MinGW packages for Windows are faster due to profile-guided
optimization. If the prebuilt packages do not match your configuration, you
need to build LLVM/Clang manually.

If you use the MSVC compiler to build Qt Creator the suggested way is:
    1. Download both MSVC and MinGW packages of libclang.
    2. Use the MSVC version of libclang during the Qt Creator build.
    3. Prepend PATH variable used for the run time with the location of MinGW version of libclang.dll.
    4. Launch Qt Creator.

### Building LLVM/Clang manually

You need to install CMake in order to build LLVM/Clang.

Build LLVM/Clang by roughly following the instructions at
http://llvm.org/docs/GettingStarted.html#git-mirror:

   1. Clone LLVM/Clang and checkout a suitable branch

          git clone -b release_17.0.6-based --recursive https://code.qt.io/clang/llvm-project.git

   2. Build and install LLVM/Clang

          mkdir build
          cd build

      For Linux/macOS:

          cmake \
            -D CMAKE_BUILD_TYPE=Release \
            -D LLVM_ENABLE_RTTI=ON \
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
            -D CMAKE_INSTALL_PREFIX=<installation location> \
            ../llvm-project/llvm
          cmake --build . --target install

      For Windows:

          cmake ^
            -G Ninja ^
            -D CMAKE_BUILD_TYPE=Release ^
            -D LLVM_ENABLE_RTTI=ON ^
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ^
            -D CMAKE_INSTALL_PREFIX=<installation location> ^
            ..\llvm-project\llvm
          cmake --build . --target install

# Licenses and Attributions

Qt Creator is available under commercial licenses from The Qt Company,
and under the GNU General Public License version 3,
annotated with The Qt Company GPL Exception 1.0.
See [LICENSE.GPL3-EXCEPT](LICENSES/LICENSE.GPL3-EXCEPT) for the details.

For more information about the third-party components that Qt Creator
includes, see the
[Acknowledgements section in the documentation](https://doc.qt.io/qtcreator/creator-acknowledgements.html).

