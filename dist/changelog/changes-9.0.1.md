Qt Creator 9.0.1
================

Qt Creator version 9.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v9.0.0..v9.0.1

General
-------

* Improved performance in the context of file path handling
* Fixed missing `No updates found.` message after looking for updates
* Fixed loading of custom external tools definitions

Editing
-------

* Fixed double `*` sign at end of long document names in dropdown

### C++

* Fixed jumping to wrong symbol with `Follow Symbol` (QTCREATORBUG-28452)
* Fixed display of tab size in code style settings (QTCREATORBUG-28450)
* Fixed crash after closing settings when opened from indexing progress
  (QTCREATORBUG-28566)
* Fixed crash when opening type hierarchy (QTCREATORBUG-28529)
* Fixed code style settings being saved even when canceling
* Fixed checkbox state in Beautifier settings (QTCREATORBUG-28525)

Projects
--------

### CMake

* Fixed that build environment was not migrated to the new configuration
  environment (QTCREATORBUG-28372)
* Fixed handling of `inherits` for deeper hierarchies (QTCREATORBUG-28498)

Debugging
---------

* Fixed handling of macros in source path mapping (QTCREATORBUG-28484)

### GDB

* Fixed pretty printer of `std::string` from `libc++` (QTCREATORBUG-28511)

### CDB

* Fixed source path mapping (QTCREATORBUG-28521)

Analyzer
--------

### Clang

* Fixed crash when clearing selection in settings (QTCREATORBUG-28524)

Test Integration
----------------

### Google Test

* Fixed debugging (QTCREATORBUG-28504)

Platforms
---------

### Linux

* Fixed wrong colors with GTK3 platform theme (QTCREATORBUG-28497)

### Docker

* Fixed that working directory for remote processes was not made reachable

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Artem Sokolovskii  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Kai Köhne  
Kwangsub Kim  
Leena Miettinen  
Marcus Tillmanns  
Orgad Shaneh  
Riitta-Leena Miettinen  
Thomas Hartmann  
Tim Jenssen  
Ulf Hermann  
