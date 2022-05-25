Qt Creator 7.0.2
================

Qt Creator version 7.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v7.0.1..v7.0.2

General
-------

### Locator

* Fixed saving of command history of `Execute` filter

Editing
-------

* Fixed that actions could be applied to wrong editor after switching split
  (QTCREATORBUG-27479)

### C++

* Updated to LLVM 14.0.3
* Fixed wrong `__cplusplus` value for older GCC versions
* ClangFormat
    * Fixed disappearing settings drop down (QTCREATORBUG-26948)

### Language Client

* Fixed crash with function argument and quick fix hints (QTCREATORBUG-27404)
* Fixed selection in `Outline` view
* Fixed `Sort Alphabetically` for outline dropdown

Projects
--------

* Fixed crash with `Recent Projects` (QTCREATORBUG-27399)
* Fixed that `-include` flags were ignored by code model (QTCREATORBUG-27450)

### CMake

* Fixed crash when cancelling progress indicator (QTCREATORBUG-27499)
* Fixed application of build directory after `Browse` (QTCREATORBUG-27407)

Debugging
---------

* Fixed pretty printer for `QFile` in Qt 6.3

Platforms
---------

### macOS

* Fixed that `arm_neon.h` could not be found by code model (QTCREATORBUG-27455)
* Fixed compilier identification of `cc` and `c++` (QTCREATORBUG-27523)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Artem Sokolovskii  
Brook Cronin  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Henning Gruendl  
Jaroslaw Kobus  
Kai Uwe Broulik  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Mats Honkamaa  
Miikka Heikkinen  
Orgad Shaneh  
Robert Löhning  
Thomas Hartmann  
Tim Jenssen  
Vikas Pachdha  
