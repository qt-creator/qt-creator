Qt Creator 7.0.1
================

Qt Creator version 7.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v7.0.0..v7.0.1

General
-------

* Fixed update notification

Editing
-------

* Fixed that double-clicking in navigation view no longer opened file in
  separate editor window (QTCREATORBUG-26773)
* Fixed selection after indenting multiple lines (QTCREATORBUG-27365)
* Fixed issues with `Ctrl+Click` for following symbol (QTCREATORBUG-26595)

### C++

* Fixed crash with Doxygen comment (QTCREATORBUG-27207)
* Fixed cursor visibility after refactoring (QTCREATORBUG-27210)
* Fixed that `Auto-Indent Selection` indented raw unicode string literals
  (QTCREATORBUG-27244)
* Fixed indentation after structured binding (QTCREATORBUG-27183)
* Fixed moving function definition with exception specification, reference
  qualifiers, or trailing return type (QTCREATORBUG-27132)
* Fixed `Generate Getter/Setter` with function types (QTCREATORBUG-27133)
* Clangd
    * Fixed local symbol renaming (QTCREATORBUG-27249)
    * Fixed more output parameter syntax highlighting issues
      (QTCREATORBUG-27352, QTCREATORBUG-27367, QTCREATORBUG-27368)
    * Fixed crash when navigating (QTCREATORBUG-27323)
    * Fixed semantic highlighting in some cases (QTCREATORBUG-27384)

### QML

* Fixed handling of JavaScript string templates (QTCREATORBUG-21869)
* Fixed wrong M325 warnings (QTCREATORBUG-27380)

Projects
--------

* Fixed default build device (QTCREATORBUG-27242)

### CMake

* Fixed empty `-D` parameter being passed to CMake (QTCREATORBUG-27237)
* Fixed that configuration errors could lead to hidden configuration widget
* Fixed sysroot configuration (QTCREATORBUG-27280)

Platforms
---------

### macOS

* Fixed `Open Terminal` and `Run in Terminal` on macOS 12.3 (QTCREATORBUG-27337)

### Android

* Fixed `Application Output` for Android 6 and earlier (QTCREATORBUG-26732)
* Fixed debugging on Linux with NDK 23 (QTCREATORBUG-27297)

### QNX

* Fixed progress bar for deploying libraries (QTCREATORBUG-27274)
* Fixed wrong `LD_LIBRARY_PATH` (QTCREATORBUG-27287)

### WebAssembly

* Fixed running `emrun --browser` with latest emsdk (QTCREATORBUG-27239)

Credits for these changes go to:
--------------------------------
Aaron Barany  
Aleksei German  
Alesandro Portale  
Alessandro Portale  
André Pönitz  
Assam Boudjelthia  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
GPery  
Henning Gruendl  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Marco Bubke  
Mats Honkamaa  
Miikka Heikkinen  
Orgad Shaneh  
Rafael Roquetto  
Robert Löhning  
Samuel Ghinet  
Samuli Piippo  
Tapani Mattila  
Tasuku Suzuki  
Thomas Hartmann  
Tim Jenssen  
Vikas Pachdha  
