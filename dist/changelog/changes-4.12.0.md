Qt Creator 4.12
===============

Qt Creator version 4.12 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.11..v4.12.0

General
-------

* Added `Restart Now` option when changing settings that require restart
* Added option for linking Qt Creator to a Qt installation, sharing
  auto-detected Qt versions and kits
* Added guard against crashing plugins at startup, providing the option
  to temporarily disable the offending plugin
* Added locator filter for searching in Qt Project bug tracker
* Added option to create custom URL template locator filters
* Added browser for Marketplace items to Welcome mode (QTCREATORBUG-23452)
* Fixed various theming issues

Help
----

* Added tool button for changing target for context help (QTCREATORBUG-17667)
* Added option to register documentation only for highest Qt version, and made that default
  (QTCREATORBUG-21482, QTCREATORBUG-22799, QTCREATORBUG-10004)

Editing
-------

* Added `Go to Last Edit`
* Added option for default line terminator style (QTCREATORBUG-3590)
* Improved behavior when splitting would hide text cursor
* Fixed that wizards ignored default file encoding
* Fixed that only restricted number of sizes were allowed for font size (QTCREATORBUG-22536)
* Fixed completion after undo (QTCREATORBUG-15038)

### C++

* Fixed issue with Clang and precompiled headers (QTCREATORBUG-22897)

### Language Client

* Added support for Markdown in tooltips
* Added support for auto-formatting
* Added outline dropdown (QTCREATORBUG-21916)
* Improved protocol error reporting
* Fixed `Ctrl-click` for `Follow Symbol Under Cursor` (QTCREATORBUG-21848)

### QML

* Updated to Qt 5.15 parser (QTCREATORBUG-23591)
* Improved support for multiple imports into same namespace (QTCREATORBUG-15684)
* Added scanning of `app.qmltypes` and `lib.qmltypes` for type information
* Fixed highlighting for new keywords in Qt 5.15
* Fixed reading of `qmltypes` from Qt 5.15 (QTCREATORBUG-23855)

### Python

* Added wizards for Qt Quick Application and Qt Widgets Application with `.ui` file
  (QTCREATORBUG-21824)

### Diff Viewer

* Added support for staging only selected lines (QTCREATORBUG-19071)

Projects
--------

* Added option to hide "disabled" files in Projects tree (QTCREATORBUG-22821)
* Added option to filter output panes for lines that do not match expression (QTCREATORBUG-19596)
* Added option for default build configuration settings (debug information, QML debugging, Qt Quick
  Compiler) (QTCREATORBUG-16458)
* Added option to only build target for active run configuration (qmake & Qbs)
* Added option to only stop the target of active run configuration on build (QTCREATORBUG-16470)
* Added option for project specific environment (QTCREATORBUG-21862)
* Added option to remove items from `Recent Projects` list in Welcome mode
* Added option to start run configurations directly from target selector (QTCREATORBUG-21799)
* Added option to build project for all configured kits (QTCREATORBUG-16815)
* Added `-ensure-kit-for-binary` command line option that creates a kit for a binary's
  architecture if needed (QTCREATORBUG-8216)
* Added GitHub build workflow to `Qt Creator Plugin` wizard template
* Improved UI responsiveness while parsing projects (QTCREATORBUG-18533)
* Fixed build directory after cloning target (QTCREATORBUG-23462)
* Fixed copying of filtered text from output pane (QTCREATORBUG-23425)

### QMake

* Improved renaming of files (QTCREATORBUG-19257)
* Fixed handling of `object_parallel_to_source` (QTCREATORBUG-18136)
* Fixed crash with circular includes (QTCREATORBUG-23567)
* Fixed issue with renaming files (QTCREATORBUG-23720)

### CMake

* Improved handling of `source_group` (QTCREATORBUG-23372)
* Added support for `Add build library search path to LD_LIBRARY_PATH` (QTCREATORBUG-23464)
* Added automatic registration of CMake documentation, if available (QTCREATORBUG-21338)
* Fixed that `.cmake` directory was created in project source directory (QTCREATORBUG-23816)
* Fixed issues with `snap` on Ubuntu Linux (QTCREATORBUG-23376)
* Fixed handling of `Enable QML` in debugger settings (QTCREATORBUG-23541)
* Fixed unneeded reparsing of files
* Fixed code model issues with precompiled headers (QTCREATORBUG-22888)

### Qbs

* Updated included Qbs version to 1.16.0
* Changed to use separate Qbs executable instead of direcly linking to Qbs (QTCREATORBUG-20622)
* Added option for default install root (QTCREATORBUG-12983)

### Python

* Added option to disable buffered output (QTCREATORBUG-23539)
* Added support for PySide 5.15 to wizards (QTCREATORBUG-23824)

### Generic

* Improved performance for large file trees (QTCREATORBUG-20652)
* Fixed that only first line of `.cflags` and `.cxxflags` was considered

### Compilation Database

* Fixed that project was reparsed if compilation database contents did not change
  (QTCREATORBUG-22574)

### Nim

* Added support for Nimble build system
* Added support for `Follow Symbol Under Cursor`

Debugging
---------

* Added option to hide columns from views (QTCREATORBUG-23342)
* Added option for `init` and `reset` GDB commands when attaching to remote server
* Fixed pretty printer for `std::optional` (QTCREATORBUG-22436)

Analyzer
--------

### Clang

* Improved filtering
* Added `Analyze Current File` to `Tools` menu and editor context menu
* Added context menu item that opens help on diagnostics

### CppCheck

* Added option to trigger Cppcheck manually

### Chrome Traces

* Added more details for counter items
* Added option to restrict view to selected threads
* Added information about percentage of total time for events

### Heob

* Added support for settings profiles (QTCREATORBUG-23209)

Qt Quick Designer
-----------------

* Added locking and pinning of animation curves (QDS-550, QDS-551)
* Added support for annotations (QDS-39)
* Fixed dragging of keyframes in curve editor (QDS-1405)
* Fixed crash when selecting icon (QTCREATORBUG-23773)
* Fixed missing import options (QDS-1592)

Version Control Systems
-----------------------

### Git

* Added option to create branch when trying to push to a non-existing branch (QTCREATORBUG-21154)
* Added option to start interactive rebase from log view (QTCREATORBUG-11200)
* Added information about upstream status to `Git Branches` view
* Added option to `grep` and `pickaxe` git log (QTCREATORBUG-22512)
* Made references in VCS output view clickable and added context menu (QTCREATORBUG-16477)

Test Integration
----------------

* Added support for colored test output (QTCREATORBUG-22297)

### Google Test

* Added support for internal logging (QTCREATORBUG-23354)
* Added support for `GTEST_SKIP` (QTCREATORBUG-23736)

Platforms
---------

### Windows

* Improved behavior with regard to MSVC tool chain matching and compatibility of MSVC 2017 and
  MSVC 2019 (QTCREATORBUG-23653)

### macOS

* Fixed parsing of Apple Clang specific linker message (QTCREATORBUG-19766)
* Fixed `Run in Terminal` and `Open Terminal` when user has different shell configured
  (QTCREATORBUG-21712)

### Android

* Discontinued support for Ministro
* Added auto-detection of Java JDK (QTCREATORBUG-23407)
* Added option to automatically download and install required Android tools (QTCREATORBUG-23285)
* Added option to register multiple NDKs (QTCREATORBUG-23286)
* Added automatic selection of correct NDK for Qt version (QTCREATORBUG-23583)
* Added option to download and use [OpenSSL for Android](https://github.com/KDAB/android_openssl)
  (QTBUG-80625)
* Added support for Android 11 with API level 30
* Improved examples browser to only show items tagged with `android` (QTBUG-80716)
* Improved manifest editor (QTCREATORBUG-23283)
* Fixed issues with latest SDK r29 (QTCREATORBUG-23726)
* Fixed several issues with AVD manager (QTCREATORBUG-23284, QTCREATORBUG-23448)
* Fixed that some essential packages were not installed (QTCREATORBUG-23829)
* Fixed that ABI selection in build configuration did not persist (QTCREATORBUG-23756)

### iOS

* Improved examples browser to only show items tagged with `ios`

### Remote Linux

* Added option to use custom command for install step (QTCREATORBUG-23320)
* Added option to override deployment data (QTCREATORBUG-21854)

### Bare Metal

* Added support for RL78 architecture
* Added support for J-Link and EBlink GDB servers
* Added support for KEIL uVision v5.x debugger

### MCU

* Added auto-registration of documentation and examples (UL-1685, UL-1218)
* Switched to MCUXpresso IDE instead of SEGGER JLink for NXP kits (QTCREATORBUG-23821)
* Fixed issues with desktop kit (QTCREATORBUG-23820)
* Fixed issues with RH850 (QTCREATORBUG-23822)

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexandru Croitor  
Andre Hartmann  
Andrey Sobol  
André Pönitz  
Assam Boudjelthia  
BogDan Vatra  
Camila San  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Denis Shienkov  
Dmitry Kovalev  
Eike Ziller  
Fawzi Mohamed  
Federico Guerinoni  
Filippo Cucchetto  
Friedemann Kleint  
Halfdan Ingvarsson  
Hannes Domani  
Henning Gruendl  
Igor Sidorov  
Jaroslaw Kobus  
Jochen Becher  
Kai Köhne  
Knud Dollereder  
Leander Schulten  
Leena Miettinen  
Lucie Gérard  
Mahmoud Badri  
Mariana Meireles  
Marius Sincovici  
Maximilian Goldstein  
Miikka Heikkinen  
Miklós Márton  
Mitch Curtis  
Mitja Kleider  
Nikolai Kosjar  
Nikolay Panov  
Oliver Wolff  
Orgad Shaneh  
Richard Weickelt  
Robert Löhning  
Sergey Morozov  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Henning  
Tim Jenssen  
Tobias Hunger  
Topi Reinio  
Ulf Hermann  
Unseon Ryu  
Venugopal Shivashankar  
Vikas Pachdha  
Ville Voutilainen  
Volodymyr Samokhatko  
zarelaky  
