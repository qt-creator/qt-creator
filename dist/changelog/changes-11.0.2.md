Qt Creator 11.0.2
=================

Qt Creator version 11.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v11.0.1..v11.0.2

General
-------

* Allow fractional high DPI scaling without modifying the environment
  ([QTCREATORBUG-29461](https://bugreports.qt.io/browse/QTCREATORBUG-29461))

Editing
-------

### General

* Fixed a potential crash when reloading a document
  ([QTCREATORBUG-29432](https://bugreports.qt.io/browse/QTCREATORBUG-29432))

### Copilot

* Fixed a crash when configuring an unusable copilot agent in the settings

Debug
-----

* Fixed a problem where debugging with "Run In Terminal" would fail on Linux
  ([QTCREATORBUG-29463](https://bugreports.qt.io/browse/QTCREATORBUG-29463))

Projects
--------

### CMake

* Fixed code completion for ui file components for CMake based projects
  ([QTCREATORBUG-28787](https://bugreports.qt.io/browse/QTCREATORBUG-28787))
* Fix reading ninjaPath from QtCreator.ini
  ([QTBUG-115754](https://bugreports.qt.io/browse/QTBUG-115754))
* Fixed incorrect device checks when using Boot2Qt
  ([QTCREATORBUG-29474](https://bugreports.qt.io/browse/QTCREATORBUG-29474))

### QMake

* Avoid cleaning the build directory after switching kits
  ([QTCREATORBUG-29451](https://bugreports.qt.io/browse/QTCREATORBUG-29451))
  ([QTCREATORBUG-29481](https://bugreports.qt.io/browse/QTCREATORBUG-29481))

Version Control Systems
-----------------------

### Fossil

* Show the correct dialog when reverting the current file

Credits for these changes go to:
--------------------------------
Aaron Barany  
André Pönitz  
Björn Schäpers  
Christian Kandeler  
Cristian Adam  
David Schulz  
Jaroslaw Kobus  
Leena Miettinen  
Marcus Tillmanns  
Orgad Shaneh  
