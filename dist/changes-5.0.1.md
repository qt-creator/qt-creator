Qt Creator 5.0.1
================

Qt Creator version 5.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v5.0.0..v5.0.1

General
-------

* Fixed saving of MIME type settings

Editing
-------

### QML

* Fixed freeze with imports that don't specify minor version
  (QTCREATORBUG-26178, QTCREATORBUG-26216)
* Fixed crash when using inline components (QTCREATORBUG-26151)

### Language Client

* Fixed working directory for servers that are started per project
  (QTCREATORBUG-26115)
* Fixed sorting of completion items (QTCREATORBUG-26114)
* Fixed that global environment setting was not used for language servers

Projects
--------

### CMake

* Improved handling of issues with `conan` (QTCREATORBUG-25818,
  QTCREATORBUG-25891)
* Fixed endless configuration loop (QTCREATORBUG-26204, QTCREATORBUG-26207,
  QTCREATORBUG-25346, QTCREATORBUG-25995, QTCREATORBUG-25183,
  QTCREATORBUG-25512)
* Fixed crash with "Re-configure with Initial Parameters" (QTCREATORBUG-26220)

### Qbs

* Fixed code model with MSVC and C++20 (QTCREATORBUG-26089)

### qmake

* Fixed that `qmake` was run on every build on macOS (QTCREATORBUG-26212)

### Compilation Database

* Fixed crash when loading project (QTCREATORBUG-26126)

Debugging
---------

### GDB

* Fixed debugging of 32-bit MinGW application with 64-bit debugger
  (QTCREATORBUG-26208)

Analyzer
--------

### Clang

* Fixed that Clazy was asked repeatedly for version and supported checks
  (QTCREATORBUG-26237)

Test Integration
----------------

### Qt Quick

* Fixed unnecessary updates of QML code model

### CTest

* Fixed missing test output

### Google Test

* Fixed wizard for CMake (QTCREATORBUG-26253)

Platforms
---------

### Windows

* Fixed issue with parsing MSVC warnings

### Android

* Fixed cleaning up of old auto-generated Android kits
* Fixed minimum SDK level for CMake projects (QTCREATORBUG-26127)

### iOS

* Fixed initial CMake parameters for iOS device builds

### Web Assembly

* Fixed detection of emscripten compilers (QTCREATORBUG-26199)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alp Öz  
Artem Sokolovskii  
Assam Boudjelthia  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Henning Gruendl  
Ivan Komissarov  
Jaroslaw Kobus  
Johanna Vanhatapio  
Laszlo Agocs  
Leena Miettinen  
Mahmoud Badri  
Miikka Heikkinen  
Orgad Shaneh  
Thomas Hartmann  
Tim Jenssen  
