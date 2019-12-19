# Qt Creator 4.11

Qt Creator version 4.11 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.10..v4.11.0

## General

* Added option for maximum number of recent files (QTCREATORBUG-21898)
* Added option for showing shortcuts in context menus (QTCREATORBUG-22502)
* Added camel case navigation to many input fields (QTCREATORBUG-21140)
* Improved fuzzy matching in Locator (QTCREATORBUG-19838, QTCREATORBUG-22546)
* Made update notification less intrusive and more informative (QTCREATORBUG-22817)
* Made wizards remember user choices (QTCREATORBUG-16657)

## Help

* Fixed that removing Qt version only unregistered its documentation after
  restart (QTCREATORBUG-16631)

## Editing

* Added option to change line ending style via editor tool bar
* Fixed that explicit colors or styles in KSyntaxHighlighting specifications were ignored
  (QTCREATORBUG-13545, QTCREATORBUG-22229, QTCREATORBUG-22646)
* Fixed behavior of backtab with multi-line selection (QTCREATORBUG-16970)

### Language Client

* Added support for semantic highlighting
  [protocol extension proposal](https://github.com/microsoft/vscode-languageserver-node/pull/367)

### C++

* Added support for single quote digit separator in integer literals (QTCREATORBUG-14939)
* Added option for adding `Q_OBJECT` to `C++ Class` wizard also for custom base class
  (QTCREATORBUG-21810)
* Fixed that build environment was not used for asking compilers about built-in headers
  (QTCREATORBUG-17985)
* Fixed handling of `-mllvm` command line option
* Fixed handling of `noexcept` when refactoring (QTCREATORBUG-11849, QTCREATORBUG-19699)

### QML

* Updated to parser from Qt 5.15
* Fixed pragma reformatting (QTCREATORBUG-22326)

### Python

* Simplified registration of language server
* Added python settings to configure multiple python interpreters
* Simplified switching python interpreters for python projects

## Help

* Added option for switching viewer backend
* Added experimental [litehtml](https://github.com/litehtml/litehtml) based viewer backend
* Added support for multiple pages in external help window (QTCREATORBUG-20558)

## Projects

* Added experimental support for Qt for WebAssembly (QTCREATORBUG-21068)
* Added experimental support for Qt for MCUs
* Added `Build` > `Build for Run Configuration` (QTCREATORBUG-22403)
* Added more space for custom command line arguments (QTCREATORBUG-17890)
* Added option to continue building after a single project failed with `Build` > `Build All`
  (QTCREATORBUG-22140)
* Added option for temporarily disabling individual environment variables (QTCREATORBUG-20984)
* Added option for translation file to wizards (QTCREATORBUG-7453)
* Added option to rename files with same base name when renaming files via project tree
  (QTCREATORBUG-21738)
* Added option for running build process with low priority (QTCREATORBUG-5155)
* Made it possible to schedule running the project while building it (QTCREATORBUG-14297)
* Improved unconfigured project page (QTCREATORBUG-20018)
* Fixed parsing of `lld` output (QTCREATORBUG-22623)
* Fixed issues with expanding macros in environment variables
* Fixed that generated files were appearing in Locator and project wide searches
  (QTCREATORBUG-20176)
* Fixed disabled project context menu item (QTCREATORBUG-22850)

### QMake

* Fixed that automatic changes of `.pro` files could change line ending style (QTCREATORBUG-2196)
* Fixed that `TRANSLATIONS` were not shown in project tree (QTCREATORBUG-7453)

### CMake

* Added support for [file-based API](https://cmake.org/cmake/help/latest/manual/cmake-file-api.7.html),
  CMake's new interface for IDEs since version 3.14
* Added Locator filter `cmo` for location of CMake target definition (QTCREATORBUG-18553)
* Added workaround for CMake issues when CMake executable is non-canonical path
* Fixed handling of boolean semantics (`OFF`, `NO`, `FALSE`, and so on)
* Fixed `Build` > `Run CMake` (QTCREATORBUG-19704)
* Fixed registering `CMake.app` from official installer on macOS
* Fixed code model issues when using precompiled headers (QTCREATORBUG-22888)

### Qbs

* Updated to Qbs version 1.15.0
* Fixed that include paths were not correctly categorized into user and system paths
  for the code model

### Python

* Renamed plugin from `PythonEditor` to `Python`
* Made Python interpreter configurable

### Generic

* Added option to remove directories directly from project tree (QTCREATORBUG-16575)
* Added support for Framework paths (QTCREATORBUG-20099)

### Compilation Database

* Fixed issue with `/imsvc` compiler option (QTCREATORBUG-23146)

## Debugging

### CDB

* Fixed assigning negative values to variables (QTCREATORBUG-17269)

## Analyzer

* Added viewer for Chrome trace report files

### Clang

* Added support for loading results from `clang-tidy` and `clazy` that were
  exported with `-export-fixes`
* Added option for overriding configuration per project
* Added option to analyze only open or edited documents
* Changed to use separate `clang-tidy` executable
* Separated diagnostic configuration settings for code model
  (`C++` > `Code Model`) and analyzer (`Analyzer` > `Clang Tools`)
* Fixed invocation of `clazy` with latest `clazy` versions

## Qt Widget Designer

* Fixed lookup of resources (QTCREATORBUG-22909, QTCREATORBUG-22962)

## Qt Quick Designer

* Added editor for bindings
* Added support for importing 3D assets into Quick3D components
  (QDS-1051, QDS-1053)
* Added option for keyframe time in `Edit` dialog (QDS-1072)
* Added snapping of timeline playhead to keyframes when shift-dragging
  (QDS-1068)
* Fixed issues with dragging timeline items (QDS-1074)
* Fixed various timeline editor issues (QDS-1073)

## Version Control Systems

### Git

* Added `Tools` > `Git` > `Local Repository` > `Archive`, and added archiving to
  `Actions on Commits`
* Made following file renames optional (QTCREATORBUG-22826)

## Test Integration

* Added settings per project (QTCREATORBUG-16704)
* Added option to run tests after successful build

### Boost

* Fixed running multiple tests (QTCREATORBUG-23091)

## Platforms

### Windows

* Added auto-detection for ARM and AVR GNU toolchains

### macOS

* Removed auto-detection of GCC toolchains
* Fixed closing of terminal window after application finishes (QTCREATORBUG-15138)
* Fixed execution of `qtc-askpass` (QTCREATORBUG-23120)
* Fixed environment when opening terminal with `zsh` (QTCREATORBUG-21712)

### Android

* Added warning if `sdkmanager` could not be run, including hint to appropriate Java SDK
  (QTCREATORBUG-22626)
* Added knowledge of Android 10 and API level 29
* Fixed issue with multiple Java versions in `PATH` (QTCREATORBUG-22504)

### Remote Linux

* Moved support for [Qt for Device Creation](https://doc.qt.io/QtForDeviceCreation/qtdc-index.html)
  (Boot2Qt) to open source
* Fixed issues with SFTP on Windows (QTCREATORBUG-22471)

### QNX

* Fixed registration of toolchain as C compiler
* Fixed issue with license checking of QCC
* Fixed deployment of public key (QTCREATORBUG-22591)

### Bare Metal

* Added support for STM8 and MSP430 architectures
* Fixed that it was not possible to add custom deployment steps (QTCREATORBUG-22977)

## Credits for these changes go to:

Aleksei German  
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Antonio Di Monaco  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Christoph Schlosser  
Cristian Adam  
David Schulz  
Denis Shienkov  
Eike Ziller  
Frederik Schwarzer  
Henning Gruendl  
Joel Smith  
Laurent Montel  
Leander Schulten  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marius Sincovici  
Michael Weghorn  
Miikka Heikkinen  
Milian Wolff  
Nikolai Kosjar  
Orgad Shaneh  
Pasi Keränen  
Richard Weickelt  
Robert Löhning  
Sergey Levin  
Sona Kurazyan  
Tasuku Suzuki  
Thomas DeRensis  
Thomas Hartmann  
Tim Henning  
Tim Jenssen  
Tobias Hunger  
Tommi Pekkala  
Uladzislau Paulovich  
Ulf Hermann  
Ville Nummela  
Volker Hilsheimer  
zarelaky  
