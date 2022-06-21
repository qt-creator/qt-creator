Qt Creator 5
===============

Qt Creator version 5 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.15..v5.0.0

General
-------

* Fixed various possible crashes at shutdown

Editing
-------

* Added line ending and indentation to file properties information
* Added menu item and shortcut for editing bookmark comments
  (QTCREATORBUG-25696)
* Fixed folding for Markdown (QTCREATORBUG-25882)
* Fixed completion tooltip on secondary display (QTCREATORBUG-26053)

### C++

* Added experimental support for `clangd` (no code completion yet, requires
  development build of `clangd`)
* Added highlighting option for function parameters (QTCREATORBUG-24880)
* Added template parameters to symbols in Locator
* Fixed that project-unrelated files were selected by default when renaming
  symbols (QTCREATORBUG-8561)
* Fixed highlighting of string literals with multi-byte characters
  (QTCREATORBUG-25715)
* Fixed code model for changed but not yet built `.ui` and `.scxml` files
  (QTCREATORBUG-25937)
* Fixed lookup of toplevel virtual function declarations (QTCREATORBUG-25749)
* Fixed `switch` statement completion for templates (QTCREATORBUG-25998)
* Fixed `Escape String Literal as UTF-8` if literal contains numbers
  (QTCREATORBUG-26003)

### QML

* Updated parser to Qt 6.2
* Fixed handling of multiline template strings (QTCREATORBUG-22766)
* Fixed handling of required and readonly properties (QTCREATORBUG-24144)
* Fixed reformatting of inline components (QTCREATORBUG-24144)
* Fixed reformatting of functions with default values (QTCREATORBUG-23009)
* Fixed wrong warning for types with same name but different namespace
  (QTCREATORBUG-24615)
* Fixed `Tools > External > Qt Quick > QML Utility` (QTCREATORBUG-26137)

### Language Client

* Added support for progress notifications
* Added support for snippets (QTCREATORBUG-22406)
* Fixed completion results for language servers that do not filter results
  themselves
* Fixed that empty responses could be sent (QTCREATORBUG-26116)

### Beautifier

* Fixed issue with `clang-format` and multi-byte characters (QTCREATORBUG-21812,
  QTCREATORBUG-23131)

### Designer

* Fixed `Go to Slot` if UI class is referred to as `UI_<class>`
  (QTCREATORBUG-26013)

Projects
--------

* Added experimental support for building and running on Docker devices
* Added find support to `Issues` pane
* Added option `Show Source and Header Groups` to project tree
  (QTCREATORBUG-25313)
* Fixed crash when closing project while changing current configuration
  (QTCREATORBUG-25655)
* Fixed that output of custom targets was interpreted as errors
  (QTCREATORBUG-25677)
* Fixed missing update of run configuration environment (QTCREATORBUG-25947)
* Fixed that user files were unnecessarily saved with new time stamp
  (QTCREATORBUG-25921)
* Fixed that toolchain setting was fixed for auto-detected kits
  (QTCREATORBUG-25839)
* Reduced UI freeze after loading projects (QTCREATORBUG-25783)

### CMake

* Removed option `Auto-create build directories`, making this the default
  behavior (QTCREATORBUG-25532)
* Added CMake output to right side of `Projects` mode (QTCREATORBUG-25522)
* Added CMake option `QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP` for skipping Qt
  Creator's package manager auto-setup
* Fixed `Jump to File` for file names with special characters
  (QTCREATORBUG-25572)
* Fixed updating of available targets (QTCREATORBUG-24914, QTCREATORBUG-25906)
* Fixed persistence of CMake tool options (QTCREATORBUG-25911)
* Fixed build library search path for CMake 3.20 and later (QTCREATORBUG-26110)
* Fixed code model issues with MSVC and CMake 3.20 and later
  (QTCREATORBUG-26146)

### Qbs

* Improved performance of registering profiles (QTCREATORBUG-25463)

Debugging
---------

* Added `Force logging to console` option (QTCREATORBUG-25421)
* Added context menu for changing variable display style to viewer window
  (QTCREATORBUG-25762)
* Fixed that comments in startup commands resulted in message boxes
  (QTCREATORBUG-25666)
* Removed extra Server Start Script field in Attach to Running Server,
  use a custom deploy step instead.

### GDB

* Added option `Use automatic symbol cache` (QTCREATORBUG-23207)

### QML

* Implemented `Load QML Stack` for LLDB (QTCREATORBUG-25554)

Analyzer
--------

### Clang

* Fixed URL for `clang-tidy` checks (QTCREATORBUG-25902)
* Fixed application of options to checks (QTCREATORBUG-25827)
* Fixed links to clazy documentation (QTCREATORBUG-25869)

FakeVim
-------

* Fixed backspace option
* Fixed updating visual marks when mark settings change

Platforms
---------

### Windows

* Added support for MSVC ARM64 toolchain

### macOS

* Improved experience on ARM based macOS with Intel based Qt Creator build
* Fixed performance issue with registering file watches after loading projects

### Android

* Added detection of Android 12
* Fixed detection of `_prepare_apk_dir` target for CMake projects
  (QTCREATORBUG-25216)

### Remote Linux

* Fixed update of temporary local installation when build path is changed
  (QTCREATORBUG-26103)

### QNX

* Fixed device configuration
* Fixed listing of device processes on Windows
* Fixed issues with CMake and QNX 7.1 and Qt 6

### MCU

* Added tracking of kit dependencies (QTCREATORBUG-25262)
* Added support for module mappings in QML (QTCREATORBUG-25356)
* Fixed update of kit after settings changes (QTCREATORBUG-25488)

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Andy Shaw  
Assam Boudjelthia  
Bernhard Beschow  
Björn Schäpers  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Erik Verbruggen  
Fawzi Mohamed  
Henning Gruendl  
Jaroslaw Kobus  
Jochen Becher  
Johanna Vanhatapio  
Kai Köhne  
Kama Wójcik  
Knud Dollereder  
Leena Miettinen  
Lukas Holecek  
Mahmoud Badri  
Marcel Krems  
Marco Bubke  
Martin Kampas  
Michael Weghorn  
Miikka Heikkinen  
Miina Puuronen  
Miklós Márton  
Nodir Temirkhodjaev  
Oliver Wolff  
Orgad Shaneh  
Pekka Kaikkonen  
Tasuku Suzuki  
Thomas Hartmann  
Tim Blechmann  
Tim Jenssen  
Tom Praschan  
Vikas Pachdha  
Wojciech Smigaj  
Youri Westerman  
