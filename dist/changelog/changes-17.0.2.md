Qt Creator 17.0.2
=================

Qt Creator version 17.0.2 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v17.0.1..v17.0.2>

Editing
-------

* Fixed an issue with camel case navigation
  ([QTCREATORBUG-33154](https://bugreports.qt.io/browse/QTCREATORBUG-33154),
   [QTCREATORBUG-33428](https://bugreports.qt.io/browse/QTCREATORBUG-33428))
* Fixed the cursor position after deleting selected text and moving up or down
  ([QTCREATORBUG-33106](https://bugreports.qt.io/browse/QTCREATORBUG-33106))

### Language Server Protocol

* Fixed a crash when completing code
  ([QTCREATORBUG-33258](https://bugreports.qt.io/browse/QTCREATORBUG-33258))

### Markdown

* Fixed a crash
  ([QTAIASSIST-295](https://bugreports.qt.io/browse/QTAIASSIST-295))

Projects
--------

* Fixed the `Qt Quick Test` wizard template when using `#pragma once`
  ([QTCREATORBUG-33502](https://bugreports.qt.io/browse/QTCREATORBUG-33502))

### CMake

* Fixed that presets were ignored if there was only a `CMakeUserPresets.json`
  file but no `CMakePresets.json` file
  ([QTCREATORBUG-33509](https://bugreports.qt.io/browse/QTCREATORBUG-33509))

### Qbs

* Fixed the Qt configuration for remote projects

### Compilation Database

* Fixed the parsing of the project when switching build configurations

Analyzer
--------

### Axivion

* Fixed an issue with authorization

Platforms
---------

### Android

* Worked around a CMake issue
  ([QTBUG-139439](https://bugreports.qt.io/browse/QTBUG-139439))
  ([CMake Bugtracker](https://gitlab.kitware.com/cmake/cmake/-/issues/27169))

### MCU

* Fixed that non-MCU kits were offered for MCU projects
  ([QTCREATORBUG-33352](https://bugreports.qt.io/browse/QTCREATORBUG-33352))

Credits for these changes go to:
--------------------------------
Alexandru Croitor  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Lukasz Papierkowski  
Marcus Tillmanns  
Teea Poldsam  
Yasser Grimes  
