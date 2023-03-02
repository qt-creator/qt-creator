Qt Creator 9.0.2
================

Qt Creator version 9.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v9.0.1..v9.0.2

General
-------

* Fixed that the UI language selection dropdown could be empty
  (QTCREATORBUG-28591)

Editing
-------

* Fixed that the `delete` key on number blocks did not work with multiple
  cursors (QTCREATORBUG-28584)
* Fixed a crash with snippets (QTCREATORBUG-28631)
* Fixed a freeze when pressing `Ctrl+D` (QTCREATORBUG-28709)

### C++

* Fixed the update of the code style preview (QTCREATORBUG-28621)
* Fixed some selection rendering issues in code that is not recognized by the
  code model (QTCREATORBUG-28637, QTCREATORBUG-28639)
* ClangFormat
    * Fixed a crash when indenting (QTCREATORBUG-28600)

### Language Server Protocol

* Fixed a wrong response that can crash language servers (QTCREATORBUG-27856,
  QTCREATORBUG-28598)

Projects
--------

### CMake

* Fixed that the path to Ninja from the online installer was not passed to CMake
  when using a MSVC toolchain (QTCREATORBUG-28685)
* Fixed the editing of `CMAKE_PREFIX_PATH` in the `Initial Configuration`
  (QTCREATORBUG-28779)
* Presets
    * Fixed that relative compiler paths in presets were not resolved correctly
      (QTCREATORBUG-28602)
    * Fixed that changes were not reflected in the kit (QTCREATORBUG-28609)

### Qmake

* Fixed a crash when connecting or disconnecting Android devices
  (QTCREATORBUG-28370)

Test Integration
----------------

### QtTest

* Fixed the checked state in the tree
* Fixed the handling of data tags with spaces

Platforms
---------

### macOS

* Fixed that macOS could ask over and over again for permissions
  (QTCREATORBUG-26705)
* Fixed that opening terminals failed on macOS 13 (QTCREATORBUG-28683)
* Fixed the detection of CMake from Homebrew on ARM Macs

### Android

* Fixed that `ANDROID_PLATFORM` was missing for NDK 23b and later
  (QTCREATORBUG-28624)

### Remote Linux

* Fixed that opening a file dialog unnecessarily asked for passwords for
  remote devices

Credits for these changes go to:
--------------------------------
Alexey Edelev  
André Pönitz  
Artem Sokolovskii  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Leena Miettinen  
Marco Bubke  
Marcus Tillmanns  
Patrik Teivonen  
Robert Löhning  
Tim Jenssen  
