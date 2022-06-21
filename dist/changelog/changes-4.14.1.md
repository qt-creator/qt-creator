Qt Creator 4.14.1
=================

Qt Creator version 4.14.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.14.0..v4.14.1

General
-------

* Fixed copying to clipboard from JavaScript locator filter

### Building Qt Creator with CMake

* Made it easier to build against separate litehtml (QTCREATORBUG-25144)
* Made it possible to adapt install layout for Linux distributions
  (QTCREATORBUG-25142)
* Fixed building and running against system LLVM (QTCREATORBUG-25147)

Editing
-------

* Fixed search result highlighting for overlapping results (QTCREATORBUG-25237)

### C++

* Added support for `BINDABLE` in `Q_PROPERTY`
* Fixed loading `ClangFormat` plugin on Linux distributions with software
  rendering (QTCREATORBUG-24998)
* Fixed hanging `Follow Symbol` (QTCREATORBUG-25193)
* Fixed freeze in global indexing (QTCREATORBUG-25121)
* Fixed missing completion in `connect` statements (QTCREATORBUG-25153)

### QML

* Fixed reformatter for arrow functions (QTCREATORBUG-23019)
* Fixed reformatter for template strings

### Language Client

* Fixed handling of dynamically registered capabilities

Projects
--------

* Fixed crash in environment settings (QTCREATORBUG-25170)

### CMake

* Fixed that CMake version support was not re-checked when changing its path in
  settings (QTCREATORBUG-25250)

### qmake

* Fixed unnecessary `qmake` run if `separate_debug_info` is force-disabled
  (QTCREATORBUG-25134)
* Fixed wrong messages in `Issues` pane from cumulative parsing
  (QTCREATORBUG-25201)

### Meson

* Fixed crash when switching build type

Debugging
---------

### LLDB

* Fixed that application output could be printed delayed (QTCREATORBUG-24667)
* Fixed performance issue (QTCREATORBUG-25185, QTCREATORBUG-25217)

### CDB

* Fixed `std::map`, `std::set` and `std::list` pretty printers in release builds
  (QTCREATORBUG-24901)

Analyzer
--------

### Clang

* Fixed issue with MSVC and MinGW (QTCREATORBUG-25126)

Platforms
---------

### Remote Linux

* Fixed SSH download operation without session (QTCREATORBUG-25236)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexis Jeandet  
Andre Hartmann  
André Pönitz  
Björn Schäpers  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Denis Shienkov  
Eike Ziller  
Henning Gruendl  
Ivan Komissarov  
Kai Köhne  
Kama Wójcik  
Knud Dollereder  
Leander Schulten  
Leena Miettinen  
Lukasz Ornatek  
Mahmoud Badri  
Marco Bubke  
Michael Winkelmann  
Miikka Heikkinen  
Orgad Shaneh  
Thomas Hartmann  
Tim Jenssen  
Vikas Pachdha  
