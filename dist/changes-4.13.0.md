Qt Creator 4.13
===============

Qt Creator version 4.13 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.12..v4.13.0

General
-------

* Added experimental support for Meson projects
* Added experimental support for IncrediBuild
* Moved view related menu items to separate toplevel `View` menu (QTCREATORBUG-23610)
* Added basic `Install Plugin` wizard to `About Plugins` dialog
* Added support for multiple shortcuts per action (QTCREATORBUG-72)
* Added sections to marketplace browser (QTCREATORBUG-23808)

Help
----

* Adapted to new filter engine in Qt 5.15

Editing
-------

* Added option for excluding file patterns from whitespace cleanup (QTCREATORBUG-13358)
* Fixed issue with resolving highlighting definitions (QTCREATORBUG-7906)

### C++

* Updated to LLVM 10
* Added editor tool button for `Analyze File` (QTCREATORBUG-23348)
* Added `Add forward declaration` refactoring action (QTCREATORBUG-23444)
* Extended `Add Include` refactoring action to non-Qt classes (QTCREATORBUG-21)
* Fixed MSVC detection with some locale settings (QTCREATORBUG-24311)
* Fixed indentation with C++11 list initialization (QTCREATORBUG-16977, QTCREATORBUG-24035)
* Fixed indentation with trailing return types (QTCREATORBUG-23502)
* Fixed issue with `std::chrono::time_point` (QTCREATORBUG-24067)
* Fixed detection of `noexcept` as change of function signature (QTCREATORBUG-23895)
* Fixed `Extract Function` with namespaces (QTCREATORBUG-23256)
* Fixed `Find Usages` for `shared_ptr` with MSVC (QTCREATORBUG-7866)
* Fixed completion for `std::pair` with MSVC
* Fixed issues with anonymous enums (QTCREATORBUG-7487)
* Fixed that inaccessible members were offered in completion (QTCREATORBUG-1984)
* Fixed refactoring for operators (QTCREATORBUG-6236)
* Fixed issues with resolving overloads in presence of default arguments (QTCREATORBUG-17807)
* Fixed sorting in completion (QTCREATORBUG-6242)
* Fixed that find usages was finding function arguments when searching functions (QTCREATORBUG-2176)
* Fixed crash in lexer (QTCREATORBUG-19525)
* Fixed handling of incomplete macro invocations (QTCREATORBUG-23881)
* Fixed update of function signature tooltip (QTCREATORBUG-24449)
* Fixed that built-in macros were ignored for custom toolchains (QTCREATORBUG-24367)

### Language Client

* Added support for renaming symbols (QTCREATORBUG-21578)
* Added highlighting of code range for diagnostics
* Added tooltips for diagnostics
* Fixed various issues with completion, outline, files with spaces, and synchronization
* Fixed handling of workspace folders (QTCREATORBUG-24452)

### QML

* Added support for moving functions in outline (QTCREATORBUG-21993)
* Added support for required list properties (QTBUG-85716)
* Fixed issues with Qt 5.15 imports
* Fixed updating of outline (QTCREATORBUG-21335)
* Fixed resolution of easing curve (QTCREATORBUG-24142)

### Python

* Added tool button for opening interactive Python, optionally importing current file
* Fixed highlighting of parentheses

### Generic Highlighter

* Updated to KSyntaxHighlighter 5.73.0

### Diff Viewer

* Added option to select encoding (QTCREATORBUG-23835)

### Model Editor

* Improved intersection computation

### QRC

* Added option to sort file list

### Widget Designer

* Fixed that designed widgets were dark in dark theme (QTCREATORBUG-23981)

Projects
--------

* Improved responsiveness of project loading (QTCREATORBUG-18533)
* Improved handling of custom project settings when a kit is removed (QTCREATORBUG-23023)
* Improved editing of path list entries in environment settings (QTCREATORBUG-20965)
* Unified compile and application output parsing (QTCREATORBUG-22665)
* Added option to remove similar files when removing a file (QTCREATORBUG-23869)
* Added support for custom output parsers (QTCREATORBUG-23993)
* Fixed responsiveness with large messages in `Compile Output` (QTCREATORBUG-23944)
* Fixed issue with macros at the start of paths (QTCREATORBUG-24095)

### Wizards

* Added heuristics for finding header to include for non-standard base class (QTCREATORBUG-3855)
* Added information about default suffix for file wizards (QTCREATORBUG-23621)

### CMake

* Removed internal cache of CMake configuration options, which removes conflicts between internal
  cache and existing configuration in build directory (QTCREATORBUG-23218)
* Removed support for server-mode (CMake < 3.14) (QTCREATORBUG-23915)
* Removed defaulting to `CodeBlocks` extra generator
* Added option to provide arguments for initial CMake configuration (QTCREATORBUG-16296,
  QTCREATORBUG-18179)
* Added option to provide arguments to `cmake --build` (QTCREATORBUG-24088)
* Added option to build several targets simultaneously
* Fixed that special build targets were missing from target list (QTCREATORBUG-24064)
* Fixed that triggering a build did not ask for applying changed CMake configuration
  (QTCREATORBUG-18504)

### Compilation Database

* Fixed that `-fpic` was removed from compiler commands (QTCREATORBUG-24106)

Debugging
---------

* Added option to reset all formats for watches to default
* Added option to override sysroot setting when starting or attaching to external application
* Fixed crash when removing all breakpoints of a file (QTCREATORBUG-24306)

### CDB

* Fixed that valid expressions sometimes were `<not accessible>` (QTCREATORBUG-24108)

Analyzer
--------

### Clang

* Re-added editor text marks for diagnostics (QTCREATORBUG-23349)
* Changed to use separate `clazy-standalone` executable
* Fixed issue with `clang-tidy` and `WarningsAsErrors` (QTCREATORBUG-23423)
* Fixed output parsing of `clazy`

Version Control Systems
-----------------------

### Git

* Added option to open `git-bash` (Windows, `Tools` > `Git` > `Git Bash`)
* Added colors to log (QTCREATORBUG-19624)
* Fixed that adding new files was also staging their content (QTCREATORBUG-23441)

Test Integration
----------------

* Added support for Catch2 test framework (QTCREATORBUG-19740)

### Google Test

Code Pasting
------------

* Added option for public or private pastes (QTCREATORBUG-23972)
* Fixed fetching from DPaste

Platforms
---------

### Linux

* Fixed issues with memory not being released (QTCREATORBUG-22832)

### Android

* Removed Ministro deployment option (QTCREATORBUG-23761)
* Added service editor to manifest editor (QTCREATORBUG-23937)
* Added splash screen editor to manifest editor (QTCREATORBUG-24011, QTCREATORBUG-24013)
* Fixed QML debugging and profiling (QTCREATORBUG-24155)
* Fixed debugging on x86 and armv7 architectures (QTCREATORBUG-24191)
* Fixed debugging on emulator (QTCREATORBUG-23291)
* Fixed issue with long kit names (QTBUG-83875)
* Fixed display of logcat (QTCREATORBUG-23177, QTCREATORBUG-23919)
* Fixed detection of Android Studio's JDK on Windows

### iOS

* Fixed slow debugger startup on devices (QTCREATORBUG-21682)

### Bare Metal

* Added support for peripheral registers view using UVSC provider
* Added support for Keil C251 toolchain
* Added support for Keil C166 toolchain
* Added support for RISC-V architecture
* Added support for various Renesas architectures

Credits for these changes go to:
--------------------------------
Aaron Barany  
Aleksei German  
Alessandro Portale  
Alexis Jeandet  
Alexis Murzeau  
Andre Hartmann  
André Pönitz  
Artur Shepilko  
Assam Boudjelthia  
Christian Kamm  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Denis Shienkov  
Eike Ziller  
Fawzi Mohamed  
Federico Guerinoni  
Friedemann Kleint  
Henning Gruendl  
Ivan Komissarov  
Jacek Nijaki  
Jaroslaw Kobus  
Jochen Becher  
Jochen Seemann  
Johanna Vanhatapio  
Joni Poikelin  
Junker, Gregory  
Knud Dollereder  
Kwanghyo Park  
Lars Knoll  
Leena Miettinen  
Lukasz Ornatek  
Mahmoud Badri  
Marco Bubke  
Michael Brüning  
Michael Weghorn  
Michael Winkelmann  
Miikka Heikkinen  
Mitch Curtis  
Nikolai Kosjar  
Oliver Wolff  
Or Kunst  
Orgad Shaneh  
Philip Van Hoof  
Richard Weickelt  
Robert Löhning  
Tarja Sundqvist  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Unseon Ryu  
Venugopal Shivashankar  
Viacheslav Tertychnyi  
Vikas Pachdha  
Ville Nummela  
Ville Voutilainen  
Volodymyr Zibarov  
Yuya Nishihara  
