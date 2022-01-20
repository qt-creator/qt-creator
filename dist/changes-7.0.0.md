Qt Creator 7
============

Qt Creator version 7 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/6.0..v7.0.0

General
-------

* Gave `Welcome` a fresh look
* Split `New File or Project` into `New File` and `New Project`
* Added optional notification of new Qt releases available in the online
  installer (QTCREATORBUG-26708)
* Added `Show in File System View` to more context menus, like `Show in
  Explorer/Finder`
* Added `Tools > Debug Qt Creator > Show Logs` for viewing Qt Creator debug logs
* Moved C++ code model and language client inspectors to `Tools > Debug Qt
  Creator`

Editing
-------

* Added action for selecting all search results in a document
* Added support for choosing external editor as default editor
  (QTCREATORBUG-13880)
* Fixed copy action in text editing macros (QTCREATORBUG-26363)

### C++

* Switched to Clangd by default (QTCREATORBUG-22917)
* Fixed that compilation errors appeared below code model errors in `Issues`
  pane (QTCREATORBUG-23655)
* Fixed that duplication files did not adapt header guard (QTCREATORBUG-26654)
* Fixed highlighting and indentation of raw string literals (QTCREATORBUG-26211)
* Fixed performance issue in global indexer (QTCREATORBUG-26841)
* clang-format
  * Moved settings to `Code Style` editor
  * Added synchronization between `clang-format` settings and custom code style
* Clangd
  * Added memory usage inspector to language client inspector
  * Added highlighting of `Q_PROPERTY` declarations
  * Improved display of diagnostic messages
  * Fixed access type categorization for functions

### QML

* Updated parser to latest Qt version
* Fixed that application directory was not searched for QML modules
  (QTCREATORBUG-24987)

### Python

* Added Python specific language server settings

### Language Server Protocol

* Removed support for outdated semantic highlighting protocol proposal
  (QTCREATORBUG-26624)
* Fixed that outdated diagnostic could be shown (QTCREATORBUG-26585)
* Fixed issue with re-highlighting (QTCREATORBUG-26624)

### FakeVim

* Added support for backslashes in substitute command (QTCREATORBUG-26955)

Projects
--------

* Added option to override GCC target triple (QTCREATORBUG-26913)
* Added multiple selection to `Issues` pane (QTCREATORBUG-25547,
  QTCREATORBUG-26720)
* Improved automatic (re-)detection of toolchains (QTCREATORBUG-26460)
* Fixed unnecessary toolchain calls at startup

### CMake

* Removed grouping of CMake cache variables (QTCREATORBUG-26218)
* Made it possible to stop CMake with button in build configuration
* Renamed `Initial Parameters` to `Initial Configuration` and moved into tabbed
  view with `Current Configuration`
* Added field for passing additional CMake options to kit, initial, and current
  configuration (QTCREATORBUG-26826)
* Added button for editing kit CMake configuration directly from build
  configuration
* Added hint for mismatches between kit, initial, and current configuration
* Added context menu actions for resolving mismatches between kit, initial and
  current configuration
* Added `Help` to context menu for variable names

### Generic

* Added support for precompiled headers (QTCREATORBUG-26532)

### Autotools

* Fixed parsing of `SUBDIRS`

Debugging
---------

* Added debugging helper for `std::variant`, `boost::container::devector`, and
  `boost::small_vector`
* Added debugging helper for `QStringView` (QTCREATORBUG-20918)
* Added `Char Code Integer`, `Hexadecimal Float`, and `Normalized, with
  Power-of-Two Exponent` display formats (QTCREATORBUG-22849,
  QTCREATORBUG-26793)
* Added shortcut for disabling and enabling breakpoints (QTCREATORBUG-26788)

Analyzer
--------

### QML

* Added support for profiling QtQuick3D (QTBUG-98146)

Version Control Systems
-----------------------

### Git

* Added support for filtering log by author
* Added handling of `HOMEDRIVE` and `HOMEPATH` on Windows
* Fixed that conflicts with deleted files could not be resolved
  (QTCREATORBUG-26994)

Test Integration
----------------

### QTest

* Added option for maximum number of warnings (QTCREATORBUG-26637)

### Qt Quick

* Added option for setup code to wizard (QTCREATORBUG-26741)

Platforms
---------

### macOS

* Fixed that macOS dark mode was not used for dark themes (QTCREATORBUG-22477)
* Fixed that user applications inherited access permissions from Qt Creator
  (QTCREATORBUG-26743)

### Android

* Added option for default NDK (QTCREATORBUG-21755, QTCREATORBUG-22389,
  QTCREATORBUG-24248, QTCREATORBUG-26281)
* Fixed that `Include prebuilt OpenSSL libraries` could add it to the wrong
  `.pro` file (QTCREATORBUG-24255)
* Fixed debugging of devices with upper case identifier with LLDB
  (QTCREATORBUG-26709)
* Fixed detection of available NDK platforms for recent NDKs
  (QTCREATORBUG-26772)

### Remote Linux

* Fixed UI state after stopping remote applications (QTCREATORBUG-26848)

### WebAssembly

* Improved browser selection (QTCREATORBUG-25028, QTCREATORBUG-26559)
* Fixed running CMake-based Qt Quick applications with Qt 6.2
  (QTCREATORBUG-26562)

### MCU

* Added support for Renesas Flash Programmer (UL-5082)

### Docker

* Added experimental support for macOS hosts

Credits for these changes go to:
--------------------------------
Aaron Barany  
Alessandro Portale  
Alexander Drozdov  
Allan Sandfeld Jensen  
André Pönitz  
Anton Alimoff  
Artem Sokolovskii  
Assam Boudjelthia  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Cristián Maureira-Fredes  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
Henning Gruendl  
Huixiong Cao  
Janne Koskinen  
Jaroslaw Kobus  
Jean-Michaël Celerier  
Jere Tuliniemi  
Joerg Kreuzberger  
Kai Köhne  
Katarina Behrens  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Maximilian Goldstein  
Miikka Heikkinen  
Morten Johan Sørvig  
Orgad Shaneh  
Petar Perisin  
Piotr Mikolajczyk  
Robert Löhning  
Samuel Ghinet  
Tasuku Suzuki  
Thomas Hartmann  
Tim Jenssen  
Tony Leinonen  
Topi Reinio  
Tor Arne Vestbø  
Ulf Hermann  
Ville Nummela  
XutaxKamay  
