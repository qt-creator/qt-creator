Qt Creator 10.0.2
=================

Qt Creator version 10.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v10.0.1..v10.0.2

General
-------

* Fixed freezes due to excessive file watching (QTCREATORBUG-28957)

Editing
-------

### C++

* Fixed a crash when following symbols (QTCREATORBUG-28989)
* Fixed the highlighting of raw string literals with empty lines
  (QTCREATORBUG-29200)
* Clang Format
    * Fixed the editing of custom code styles (QTCREATORBUG-29129)
    * Fixed that the wrong code style could be used (QTCREATORBUG-29145)

Projects
--------

* Fixed a crash when triggering a build with unconfigured projects present
  (QTCREATORBUG-29207)

### CMake

* Fixed that the global `Autorun CMake` option could be overridden by old
  settings
* Fixed the `Build CMake Target` locator filter in case a build is already
  running (QTCREATORBUG-26699)
* Presets
    * Added the expansion of `${hostSystemName}` (QTCREATORBUG-28935)
    * Fixed the Qt detection when `CMAKE_TOOLCHAIN_FILE` and `CMAKE_PREFIX_PATH`
      are set

Debugging
---------

* Fixed that debugger tooltips in the editor vanished after expanding
  (QTCREATORBUG-29083)

Test Integration
----------------

* GoogleTest
    * Fixed the reporting of failed tests (QTCREATORBUG-29146)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Artem Sokolovskii  
Björn Schäpers  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Karim Abdelrahman  
Leena Miettinen  
Miikka Heikkinen  
Patrik Teivonen  
Robert Löhning  
Sivert Krøvel  
